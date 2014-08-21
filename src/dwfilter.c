/* Copyright (C) 2010-2011 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#include "definitions.h"
#include "util.h"
#include "optionMacros.h"
#define DWFILTER_COMPILE
#include "optionDescriptions.h"

#define TEMPLATE "/tmp/dwdiffXXXXXXXX"
#define TEMPLATE_LENGTH sizeof(TEMPLATE)

#ifndef DWDIFF
#	define DWDIFF "dwdiff"
#endif


struct {
	int oldFile, newFile;
	bool reverse;
	int postProcessorStart;
} option;

static PARSE_FUNCTION(parseCmdLine)
	int noOptionCount = 0;
	bool discard;

	/* Initialise options to correct values */
	memset(&option, 0, sizeof(option));

	OPTIONS
		OPTION('d', "delimiters", REQUIRED_ARG)
		END_OPTION
		OPTION('P', "punctuation", NO_ARG)
		END_OPTION
		OPTION('W', "white-space", REQUIRED_ARG)
		END_OPTION
		OPTION('h', "help", NO_ARG)
				int i = 0;
				printf(_("Usage: dwfilter [OPTIONS] <OLD FILE> <NEW FILE> <POST PROCESSOR> [POST PROCESSOR OPTIONS]\n"));
				while (descriptions[i] != NULL)
					fputs(_(descriptions[i++]), stdout);
				exit(EXIT_SUCCESS);
		END_OPTION
		OPTION('v', "version", NO_ARG)
			fputs("dwfilter " VERSION_STRING "\n", stdout);
			fputs(
				/* TRANSLATORS:
				   - If the (C) symbol (that is the c in a circle) is not available,
					 leave as it as is. (Unicode code point 0x00A9)
				   - G.P. Halkes is name and should be left as is. */
				_("Copyright (C) 2006-2011 G.P. Halkes\nLicensed under the GNU General Public License version 3\n"), stdout);
			exit(EXIT_SUCCESS);
		END_OPTION
		OPTION('i', "ignore-case", NO_ARG)
		END_OPTION
		OPTION('I', "ignore-formatting", NO_ARG)
		END_OPTION
		SINGLE_DASH
		switch (noOptionCount++) {
			case 0:
				option.oldFile = optargind;
				break;
			case 1:
				option.newFile = optargind;
				break;
			default:
				option.postProcessorStart = optargind;
				STOP_OPTION_PROCESSING;
				break;
		}
		END_OPTION
		DOUBLE_DASH
			NO_MORE_OPTIONS;
		END_OPTION
		OPTION('C', "context", REQUIRED_ARG)
		END_OPTION
		OPTION('m', "match-context", REQUIRED_ARG)
		END_OPTION
		BOOLEAN_LONG_OPTION("aggregate-changes", discard)
		OPTION('A', "algorithm", REQUIRED_ARG)
		END_OPTION
		BOOLEAN_LONG_OPTION("wdiff-output", discard)
		/* FIXME: make this work again, after fixing dwdiff */
/* 		OPTION('S', "paragraph-separator", OPTIONAL_ARG)
		END_OPTION */

		BOOLEAN_OPTION('r', "reverse", option.reverse)

		fatal(_("Option %.*s does not exist\n"), OPTPRARG);
	NO_OPTION
		switch (noOptionCount++) {
			case 0:
				option.oldFile = optargind;
				break;
			case 1:
				option.newFile = optargind;
				break;
			default:
				option.postProcessorStart = optargind;
				STOP_OPTION_PROCESSING;
				break;
		}
	END_OPTIONS
	/* Check that we have something to work with. */
	if (noOptionCount < 2)
		fatal(_("Need two files to compare\n"));
	if (noOptionCount < 3)
		fatal(_("No post processor specified\n"));

END_FUNCTION

static char tempfile[TEMPLATE_LENGTH];

static void cleanup(void) {
	unlink(tempfile);
}

static void createTempfile(void) {
	int fd;
	/* Create temporary file. */
	/* Make sure the umask is set so that we don't introduce a security risk. */
	umask(~S_IRWXU);
	/* Make sure we will remove temporary files on exit. */
	atexit(cleanup);

	strcpy(tempfile, TEMPLATE);
	if ((fd = mkstemp(tempfile)) < 0)
		fatal(_("Could not create temporary file: %s\n"), strerror(errno));
	close(fd);
}

static int execute(char * const args[]) {
	pid_t pid;
	int status;

	if ((pid = fork()) == 0) {
		execvp(args[0], args);
		exit(127);
	} else if (pid < 0) {
		fatal(_("Could not execute %s: %s\n"), args[0], strerror(errno));
	}
	pid = wait(&status);
	if (pid < 0)
		fatal(_("Error waiting for child process to terminate: %s\n"), strerror(errno));
	return WEXITSTATUS(status);
}

#define OUTPUT_OPTION "--dwfilter="

int main(int argc, char *argv[]) {
	char **cmdArgs;
	char *outputArg;
	int i, j;

#if defined(USE_GETTEXT) || defined(USE_UNICODE)
	setlocale(LC_ALL, "");
#endif

#ifdef USE_GETTEXT
	bindtextdomain("dwdiff", LOCALEDIR);
	textdomain("dwdiff");
#endif

	parseCmdLine(argc, argv);
	createTempfile();
	cmdArgs = safe_malloc(sizeof(char *) * (option.postProcessorStart + 7));
	outputArg = safe_malloc(sizeof(char) * (strlen(tempfile) + sizeof(OUTPUT_OPTION)));

	cmdArgs[0] = safe_strdup(DWDIFF);
	/* argument has been allocated to correct size above, so we can safely use sprintf */
	sprintf(outputArg, OUTPUT_OPTION "%s", tempfile);
	cmdArgs[1] = outputArg;

	for (i = 1, j = 2; i < option.postProcessorStart; i++)
		if (i != option.oldFile && i != option.newFile)
			cmdArgs[j++] = argv[i];

	cmdArgs[j++] = safe_strdup("-w");
	cmdArgs[j++] = safe_strdup("");
	cmdArgs[j++] = safe_strdup("-x");
	cmdArgs[j++] = safe_strdup("");
	cmdArgs[j++] = safe_strdup("-2");

	cmdArgs[j++] = argv[option.reverse ? option.newFile : option.oldFile];
	cmdArgs[j++] = argv[option.reverse ? option.oldFile : option.newFile];
	cmdArgs[j++] = NULL;

	if (execute(cmdArgs) > 2)
		fatal(_("dwdiff returned an error\n"));

	free(outputArg);
	free(cmdArgs);

	cmdArgs = safe_malloc(sizeof(char *) * (argc - option.postProcessorStart + 3));

	for (i = option.postProcessorStart, j = 0; i < argc; i++, j++)
		cmdArgs[j] = argv[i];

	cmdArgs[j++] = option.reverse ? argv[option.oldFile] : tempfile;
	cmdArgs[j++] = option.reverse ? tempfile : argv[option.newFile];
	cmdArgs[j++] = NULL;

	return execute(cmdArgs);
}


