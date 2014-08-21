/* Copyright (C) 2006-2011 G.P. Halkes
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

#include <ctype.h>
#include <unistd.h>

#include "definitions.h"
#include "stream.h"
#include "unicode.h"
#include "optionMacros.h"
#include "option.h"
#include "util.h"
#include "buffer.h"
#include "dispatch.h"

#define DWDIFF_COMPILE
#include "optionDescriptions.h"

typedef struct {
	const char *name;
	const char *description;
	const char *sequence;
	const char *backgroundSequence;
} Color;

Color colors[] = {
	{ "black", N_("Black"), "30", "40" },
	{ "red", N_("Red"), "31", "41" },
	{ "green", N_("Green"), "32", "42" },
	{ "brown", N_("Brown"), "33", "43" },
	{ "blue", N_("Blue"), "34", "44" },
	{ "magenta", N_("Magenta"), "35", "45" },
	{ "cyan", N_("Cyan"), "36", "46" },
	{ "gray", N_("Gray"), "37", "47" },
	{ "dgray", N_("Dark gray"), "30;1", NULL },
	{ "bred", N_("Bright red"), "31;1", NULL },
	{ "bgreen", N_("Bright green"), "32;1", NULL },
	{ "yellow", N_("Yellow"), "33;1", NULL },
	{ "bblue", N_("Bright blue"), "34;1", NULL },
	{ "bmagenta", N_("Bright magenta"), "35;1", NULL },
	{ "bcyan", N_("Bright cyan"), "36;1", NULL },
	{ "white", N_("White"), "37;1", NULL },
	{ NULL, NULL, NULL, NULL }
};

/** Compare two strings, ignoring case.
	@param a The first string to compare.
	@param b The first string to compare.
	@return an integer smaller, equal, or larger than zero to indicate that @a a
		sorts before, the same, or after @a b.
*/
int strCaseCmp(const char *a, const char *b) {
	int ca, cb;
	while ((ca = tolower(*a++)) == (cb = tolower(*b++)) && ca != 0) {}

	return ca == cb ? 0 : (ca < cb ? -1 : 1);
}

/** Convert a string from the input format to an internally usable string.
	@param string A @a Token with the string to be converted.
	@param descr A description of the string to be included in error messages.
	@return The length of the resulting string.

	The use of this function processes escape characters. The converted
	characters are written in the original string.
*/
static size_t parseEscapes(char *string, const char *descr) {
	size_t maxReadPosition = strlen(string);
	size_t readPosition = 0, writePosition = 0;
	size_t i;

	while (readPosition < maxReadPosition) {
		if (string[readPosition] == '\\') {
			readPosition++;

			if (readPosition == maxReadPosition) {
				fatal(_("Single backslash at end of %s argument\n"), descr);
			}
			switch(string[readPosition++]) {
				case 'n':
					string[writePosition++] = '\n';
					break;
				case 'r':
					string[writePosition++] = '\r';
					break;
				case '\'':
					string[writePosition++] = '\'';
					break;
				case '\\':
					string[writePosition++] = '\\';
					break;
				case 't':
					string[writePosition++] = '\t';
					break;
				case 'b':
					string[writePosition++] = '\b';
					break;
				case 'f':
					string[writePosition++] = '\f';
					break;
				case 'a':
					string[writePosition++] = '\a';
					break;
				case 'v':
					string[writePosition++] = '\v';
					break;
				case '?':
					string[writePosition++] = '\?';
					break;
				case '"':
					string[writePosition++] = '"';
					break;
				case 'e':
					string[writePosition++] = '\033';
					break;
				case 'x': {
					/* Hexadecimal escapes */
					unsigned int value = 0;
					/* Read at most two characters, or as many as are valid. */
					for (i = 0; i < 2 && (readPosition + i) < maxReadPosition && isxdigit(string[readPosition + i]); i++) {
						value <<= 4;
						if (isdigit(string[readPosition + i]))
							value += (int) (string[readPosition + i] - '0');
						else
							value += (int) (tolower(string[readPosition + i]) - 'a') + 10;
						if (value > UCHAR_MAX)
							/* TRANSLATORS:
							   The %s argument is a long option name without preceding dashes. */
							fatal(_("Invalid hexadecimal escape sequence in %s argument\n"), descr);
					}
					readPosition += i;

					if (i == 0)
						/* TRANSLATORS:
						   The %s argument is a long option name without preceding dashes. */
						fatal(_("Invalid hexadecimal escape sequence in %s argument\n"), descr);

					string[writePosition++] = (char) value;
					break;
				}
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7': {
					/* Octal escapes */
					int value = (int)(string[readPosition - 1] - '0');
					size_t maxIdx = string[readPosition - 1] < '4' ? 2 : 1;
					for (i = 0; i < maxIdx && readPosition + i < maxReadPosition && string[readPosition + i] >= '0' && string[readPosition + i] <= '7'; i++)
						value = value * 8 + (int)(string[readPosition + i] - '0');

					readPosition += i;

					string[writePosition++] = (char) value;
					break;
				}
#ifdef USE_UNICODE
				case 'u':
				case 'U': {
					UChar32 value = 0;
					size_t chars = string[readPosition - 1] == 'U' ? 8 : 4;

					if (maxReadPosition < readPosition + chars)
						/* TRANSLATORS:
						   The %c argument will be either 'u' or 'U'. The %s argument is a long
					       option name without preceding dashes. */
						fatal(_("Too short \\%c escape in %s argument\n"), string[readPosition - 1], descr);
					for (i = 0; i < chars; i++) {
						if (!isxdigit(string[readPosition + i]))
							/* TRANSLATORS:
							   The %c argument will be either 'u' or 'U'. The %s argument is a long
							   option name without preceding dashes. */
							fatal(_("Too short \\%c escape in %s argument\n"), string[readPosition - 1], descr);
						value <<= 4;
						if (isdigit(string[readPosition + i]))
							value += (int) (string[readPosition + i] - '0');
						else
							value += (int) (tolower(string[readPosition + i]) - 'a') + 10;
					}

					if (value > 0x10FFFFL)
						/* TRANSLATORS:
						   The %s argument is a long option name without preceding dashes. */
						fatal(_("\\U escape out of range in %s argument\n"), descr);

					if ((value & 0xF800L) == 0xD800L)
						/* TRANSLATORS:
						   The %s argument is a long option name without preceding dashes. */
						fatal(_("\\%c escapes for surrogate codepoints are not allowed in %s argument\n"), string[readPosition - 1], descr);

					/* The conversion won't overwrite subsequent characters because
					   \uxxxx is already the as long as the max utf-8 length */
					writePosition += convertToUTF8(value, string + writePosition);
					readPosition += chars;
					break;
				}
#endif
				default:
					string[writePosition++] = string[readPosition - 1];
					break;
			}
		} else {
			string[writePosition++] = string[readPosition++];
		}
	}
	/* Terminate string. */
	string[writePosition] = 0;
	return writePosition;
}

static void noDefaultMarkers(void) {
	/* Note: then lenghts are 0 by default. */
	if (option.addStart == NULL)
		option.addStart = "";
	if (option.addStop == NULL)
		option.addStop = "";
	if (option.delStart == NULL)
		option.delStart = "";
	if (option.delStop == NULL)
		option.delStop = "";
}


/*===============================================================*/
/* Single character (SC) versions of the addCharacters and checkOverlap routines.
   Descriptions can be found in the definition of the DispatchTable struct. */

/** Add all characters to the specified list or bitmap.
	@param chars The string with characters to add to the list or bitmap.
	@param list The list to add to.
	@param bitmap. The bitmap to add to.

	If in UTF-8 mode, the characters will be added to the list. Otherwise to
	the bitmap.
*/
void addCharactersSC(const char *chars, size_t length, CHARLIST *list, char bitmap[BITMASK_SIZE]) {
	size_t i;

	/* Suppress unused parameter warning in semi-portable way. */
	(void) list;

	for (i = 0; i < length; i++) {
		SET_BIT(bitmap, chars[i]);
	}
}

/** Check for overlap in whitespace and delimiter sets. */
void checkOverlapSC(void) {
	int i;

	if (!option.whitespaceSet)
		return;

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (!TEST_BIT(option.delimiters, i))
			continue;

		if (TEST_BIT(option.whitespace, i))
			fatal(_("Whitespace and delimiter sets overlap\n"));
	}
}

void setPunctuationSC(void) {
	int i;
	for (i = 0; i <= UCHAR_MAX; i++) {
		if (!ispunct(i))
			continue;
		SET_BIT(option.delimiters, i);
	}
}

void initOptionsSC(void) {}

void postProcessOptionsSC(void) {
	if (!option.whitespaceSet) {
		int i;
		for (i = 0; i <= UCHAR_MAX; i++) {
			if (!TEST_BIT(option.delimiters, i) && isspace(i))
				SET_BIT(option.whitespace, i);
		}
	}

	if (!TEST_BIT(option.whitespace, ' '))
		option.wdiffOutput = true;
}

#ifdef USE_UNICODE
/*===============================================================*/
/* UTF-8 versions of the addCharacters and checkOverlap routines.
   Descriptions can be found in the definition of the DispatchTable struct. */

/** Add all characters to the specified list.
	@param chars The string with characters to add to the list or bitmap.
	@param list The list to add to.
	@param bitmap. The bitmap to add to (unused).
*/
void addCharactersUTF8(const char *chars, size_t length, CHARLIST *list, char bitmap[BITMASK_SIZE]) {
	Stream *charStream;
	CharData cluster;

	/* Suppress unused parameter warning in semi-portable way. */
	(void) bitmap;

	VECTOR_INIT_ALLOCATED(cluster.UTF8Char.original);
	VECTOR_INIT_ALLOCATED(cluster.UTF8Char.converted);
	charStream = newStringStream(chars, length);

	while (getCluster(charStream, &cluster.UTF8Char.original)) {
		UTF16Buffer tmp;
		size_t i;

		decomposeChar(&cluster);

		/* Check for duplicates */
		for (i = 0; i < list->used; i++)
			if (compareUTF16Buffer(&cluster.UTF8Char.converted, &list->data[i]) == 0)
				break; /* continue won't work because we're in a for-loop */
		if (i != list->used)
			continue;

		tmp.used = cluster.UTF8Char.converted.used;
		tmp.data = safe_malloc(cluster.UTF8Char.converted.used * sizeof(UChar));
		tmp.allocated = cluster.UTF8Char.converted.used;
		memcpy(tmp.data, cluster.UTF8Char.converted.data, cluster.UTF8Char.converted.used * sizeof(UChar));
		VECTOR_APPEND(*list, tmp);
	}
	VECTOR_FREE(cluster.UTF8Char.original);
	VECTOR_FREE(cluster.UTF8Char.converted);
	free(charStream);
}

/** Check for overlap in whitespace and delimiter sets. */
void checkOverlapUTF8(void) {
	size_t i, j;

	if (!option.whitespaceSet)
		return;

	/* Check for overlap can be done in O(N) because the lists have already been sorted in postProcessOptionsUTF8. */
	for (i = 0, j = 0; i < option.delimiterList.used; i++) {
		for (; j < option.whitespaceList.used && compareUTF16Buffer(&option.delimiterList.data[i],
				&option.whitespaceList.data[j]) > 0; j++) /* NO-OP */;

		if (j == option.whitespaceList.used)
			break;

		if (compareUTF16Buffer(&option.delimiterList.data[i], &option.whitespaceList.data[j]) == 0)
			fatal(_("Whitespace and delimiter sets overlap\n"));
	}

	if (option.punctuationMask == 0)
		return;

	for (i = 0; i < option.whitespaceList.used; i++) {
		if (isUTF16Punct(&option.whitespaceList.data[i]))
			fatal(_("Whitespace and delimiter sets overlap\n"));
	}
	return;
}

void setPunctuationUTF8(void) {
	option.punctuationMask = U_GC_P_MASK | U_GC_S_MASK;
}

void initOptionsUTF8(void) {
	VECTOR_INIT_ALLOCATED(option.whitespaceList);
	VECTOR_INIT_ALLOCATED(option.delimiterList);
}

void postProcessOptionsUTF8(void) {
	static_assert(CRLF_GRAPHEME_CLUSTER_BREAK == 0);

	qsort(option.delimiterList.data, option.delimiterList.used,	sizeof(UTF16Buffer),
		(int (*)(const void *, const void *)) compareUTF16Buffer);

	qsort(option.whitespaceList.data, option.whitespaceList.used, sizeof(UTF16Buffer),
		(int (*)(const void *, const void *)) compareUTF16Buffer);

	VECTOR_APPEND(charData.UTF8Char.converted, ' ');
	if (classifyChar() != CAT_WHITESPACE)
		option.wdiffOutput = true;
	charData.UTF8Char.converted.used = 0;
}

/*===============================================================*/
#endif

#define SEQUENCE_BUFFER_LEN 20
static char *parseColor(const char *_color) {
	char sequenceBuffer[SEQUENCE_BUFFER_LEN];
	char *colon, *color;
	int i, fg = -1, bg = -1;

	color = strdupA(_color);

	if (strncmp("e:", color, 2) == 0) {
		/* Custom color string */
		parseEscapes(color + 2, "color");
		memmove(color, color + 2, strlen(color) + 1);
		return color;
	}

	colon = strchr(color, ':');

	if (colon != NULL && strrchr(color, ':') != colon)
		fatal(_("Invalid color specification %s\n"), color);

	if (colon != NULL)
		*colon++ = 0;

	if (*color != 0) {
		for (i = 0; colors[i].name != NULL; i++) {
			if (strCaseCmp(color, colors[i].name) == 0) {
				fg = i;
				break;
			}
		}
		if (colors[i].name == NULL)
			fatal(_("Invalid color %s\n"), color);
	}

	if (colon != NULL) {
		if (*colon != 0) {
			for (i = 0; colors[i].backgroundSequence != NULL; i++) {
				if (strCaseCmp(colon, colors[i].name) == 0) {
					bg = i;
					break;
				}
			}
			if (colors[i].backgroundSequence == NULL)
				fatal(_("Invalid background color %s\n"), colon);
		}
	}

	if (fg >= 0) {
		if (bg >= 0)
			snprintf(sequenceBuffer, SEQUENCE_BUFFER_LEN, "\033[0;%s;%sm", colors[fg].sequence, colors[bg].backgroundSequence);
		else
			snprintf(sequenceBuffer, SEQUENCE_BUFFER_LEN, "\033[0;%sm", colors[fg].sequence);
	} else {
		if (bg >= 0)
			snprintf(sequenceBuffer, SEQUENCE_BUFFER_LEN, "\033[0%sm", colors[bg].backgroundSequence);
	}
	free(color);
	return strdupA(sequenceBuffer);
}

PARSE_FUNCTION(parseCmdLine)
	char *comma;
	int noOptionCount = 0;

	/* Initialise options to correct values */
	memset(&option, 0, sizeof(option));
	option.printDeleted = option.printAdded = option.printCommon = true;
	option.matchContext = 2;
	option.output = stdout;

	initOptions();
	ONLY_UNICODE(option.decomposition = UNORM_NFD;)

	option.needStartStop = true;

	option.paraDelimMarker = "<-->";
	option.paraDelimMarkerLength = strlen(option.paraDelimMarker);

	OPTIONS
		OPTION('d', "delimiters", REQUIRED_ARG)
			size_t length = parseEscapes(optArg, "delimiters");
			addCharacters(optArg, length, SWITCH_UNICODE(&option.delimiterList, NULL), option.delimiters);
		END_OPTION
		OPTION('P', "punctuation", NO_ARG)
			setPunctuation();
		END_OPTION
		OPTION('W', "white-space", REQUIRED_ARG)
			size_t length = parseEscapes(optArg, "whitespace");
			option.whitespaceSet = true;
			addCharacters(optArg, length, SWITCH_UNICODE(&option.whitespaceList, NULL), option.whitespace);
		END_OPTION
		OPTION('h', "help", NO_ARG)
				int i = 0;
				printf(_("Usage: dwdiff [OPTIONS] <OLD FILE> <NEW FILE>\n"));
				while (descriptions[i] != NULL)
					fputs(_(descriptions[i++]), stdout);
				exit(EXIT_SUCCESS);
		END_OPTION
		OPTION('v', "version", NO_ARG)
			fputs("dwdiff " VERSION_STRING "\n", stdout);
			fputs(
				/* TRANSLATORS:
				   - If the (C) symbol (that is the c in a circle) is not available,
					 leave as it as is. (Unicode code point 0x00A9)
				   - G.P. Halkes is name and should be left as is. */
				_("Copyright (C) 2006-2011 G.P. Halkes and others\nLicensed under the GNU General Public License version 3\n"), stdout);
			exit(EXIT_SUCCESS);
		END_OPTION
		OPTION('1', "no-deleted", NO_ARG)
			option.printDeleted = false;
			if (!option.printAdded)
				option.needMarkers = true;
			if (!option.printCommon || !option.printAdded)
				option.needStartStop = false;
		END_OPTION
		OPTION('2', "no-inserted", NO_ARG)
			option.printAdded = false;
			if (!option.printDeleted)
				option.needMarkers = true;
			if (!option.printCommon || !option.printDeleted)
				option.needStartStop = false;
		END_OPTION
		OPTION('3', "no-common", NO_ARG)
			option.printCommon = false;
			option.needMarkers = true;
		END_OPTION
		OPTION('i', "ignore-case", NO_ARG)
			option.ignoreCase = true;
		END_OPTION
#ifdef USE_UNICODE
		OPTION('I', "ignore-formatting", NO_ARG)
			if (UTF8Mode)
				option.decomposition = UNORM_NFKD;
			else
				fatal(_("Option %.*s is only supported for UTF-8 mode\n"), OPTPRARG);
#else
		OPTION('I', "ignore-formatting", NO_ARG)
			fatal(_("Support for option %.*s is not compiled into this version of dwdiff\n"), OPTPRARG);
#endif
		END_OPTION
		OPTION('s', "statistics", NO_ARG)
			option.statistics = true;
		END_OPTION
		OPTION('a', "autopager", NO_ARG)
			fatal(_("Option %.*s is not supported\n"), OPTPRARG);
		END_OPTION
		OPTION('p', "printer", NO_ARG)
			option.printer = true;
			noDefaultMarkers();
		END_OPTION
		OPTION('l', "less", NO_ARG)
			option.less = true;
			noDefaultMarkers();
		END_OPTION
		OPTION('t', "terminal", NO_ARG)
			fatal(_("Option %.*s is not supported\n"), OPTPRARG);
		END_OPTION
		OPTION('w', "start-delete", REQUIRED_ARG)
			option.delStartLen = parseEscapes(optArg, "start-delete");
			option.delStart = optArg;
		END_OPTION
		OPTION('x', "stop-delete", REQUIRED_ARG)
			option.delStopLen = parseEscapes(optArg, "stop-delete");
			option.delStop = optArg;
		END_OPTION
		OPTION('y', "start-insert", REQUIRED_ARG)
			option.addStartLen = parseEscapes(optArg, "start-insert");
			option.addStart = optArg;
		END_OPTION
		OPTION('z', "stop-insert", REQUIRED_ARG)
			option.addStopLen = parseEscapes(optArg, "stop-insert");
			option.addStop = optArg;
		END_OPTION
		OPTION('n', "avoid-wraps", NO_ARG)
			fatal(_("Option %.*s is not supported\n"), OPTPRARG);
		END_OPTION
		SINGLE_DASH
			switch (noOptionCount++) {
				case 0:
					option.oldFile.input = newFileStream(fileWrapFD(STDIN_FILENO, FILE_READ));
					break;
				case 1:
					if (option.oldFile.name == NULL)
						fatal(_("Can't read both files from standard input\n"));
					option.newFile.input = newFileStream(fileWrapFD(STDIN_FILENO, FILE_READ));
					break;
				default:
					fatal(_("Too many files to compare\n"));
			}
		END_OPTION
		DOUBLE_DASH
			NO_MORE_OPTIONS;
		END_OPTION
		OPTION('c', "color", OPTIONAL_ARG)
			option.colorMode = true;
			if (optArg != NULL) {
				int i;

				if (strCaseCmp(optArg, "list") == 0) {
					fputs(
						/* TRANSLATORS:
						   "Name" and "Description" are table headings for the color name list.
						   Make sure you keep the alignment of the headings over the text. */
						_("Name            Description\n"), stdout);
					fputs("--              --\n", stdout);
					for (i = 0; colors[i].name != NULL; i++)
						printf("%-15s %s\n", colors[i].name, _(colors[i].description));
					fputc('\n', stdout);
					fputs(_("The colors black through gray are also usable as background color\n"), stdout);
					exit(EXIT_SUCCESS);
				}

				comma = strchr(optArg, ',');
				if (comma != NULL && strrchr(optArg, ',') != comma)
					fatal(_("Invalid color specification %s\n"), optArg);

				if (comma != NULL)
					*comma++ = 0;

				option.delColor = parseColor(optArg[0] == 0 ? "bred" : optArg);
				option.addColor = parseColor(comma == NULL ? "bgreen" : comma);
			} else {
				option.delColor = parseColor("bred");
				option.addColor = parseColor("bgreen");
			}
			option.delColorLen = strlen(option.delColor);
			option.addColorLen = strlen(option.addColor);
			noDefaultMarkers();
		END_OPTION
		OPTION('L', "line-numbers", OPTIONAL_ARG)
			if (optArg != NULL)
				PARSE_INT(option.lineNumbers, 1, INT_MAX);
			else
				option.lineNumbers = DEFAULT_LINENUMBER_WIDTH;
		END_OPTION
		OPTION('C', "context", REQUIRED_ARG)
			PARSE_INT(option.contextLines, 0, INT_MAX);
			option.context = true;
			initContextBuffers();
		END_OPTION
		OPTION('m', "match-context", REQUIRED_ARG)
			PARSE_INT(option.matchContext, 0, (INT_MAX - 1) / 2);
			option.matchContext *= 2;
		END_OPTION
		BOOLEAN_LONG_OPTION("aggregate-changes", option.aggregateChanges)
		OPTION('S', "paragraph-separator", OPTIONAL_ARG)
			option.paraDelim = true;
			if (optArg != NULL) {
				option.paraDelimMarker = optArg;
				option.paraDelimMarkerLength = parseEscapes(optArg, "paragraph-separator");
			}
		END_OPTION
		BOOLEAN_LONG_OPTION("wdiff-output", option.wdiffOutput)
		LONG_OPTION("dwfilter", REQUIRED_ARG)
			option.dwfilterMode = true;
			if ((option.output = fopen(optArg, "r+")) == NULL)
				fatal(_("Error opening temporary output file: %s\n"), strerror(errno));
		END_OPTION
		OPTION('r', "reverse", NO_ARG)
			if (!option.dwfilterMode)
				fatal(_("Option %.*s does not exist\n"), OPTPRARG);
		END_OPTION
		OPTION('R', "repeat-markers", NO_ARG)
			option.repeatMarkers = true;
		END_OPTION
		LONG_OPTION("diff-input", NO_ARG)
			option.diffInput = true;
		END_OPTION
		OPTION('A', "algorithm", REQUIRED_ARG)
			if (strcmp(optArg, "best") == 0) {
				minimal = true;
				speed_large_files = false;
			} else if (strcmp(optArg, "normal") == 0) {
				minimal = false;
				speed_large_files = false;
			} else if (strcmp(optArg, "fast") == 0) {
				minimal = false;
				speed_large_files = true;
			} else {
				fatal(_("Invalid algorithm name\n"));
			}
		END_OPTION

		fatal(_("Option %.*s does not exist\n"), OPTPRARG);
	NO_OPTION
		switch (noOptionCount++) {
			case 0:
				option.oldFile.name = optcurrent;
				break;
			case 1:
				option.newFile.name = optcurrent;
				break;
			default:
				fatal(_("Too many files to compare\n"));
		}
	END_OPTIONS
	/* Check that we have something to work with. */
	if (option.diffInput) {
		if (noOptionCount > 1)
			fatal(_("Only one input file accepted with --diff-input\n"));
		if (noOptionCount == 0)
			option.oldFile.input = newFileStream(fileWrapFD(STDIN_FILENO, FILE_READ));
	} else {
		if (noOptionCount != 2)
			fatal(_("Need two files to compare\n"));
	}

	/* Check and set some values */
	if (option.delStart == NULL) {
		option.delStart = "[-";
		option.delStartLen = 2;
	}
	if (option.delStop == NULL) {
		option.delStop = "-]";
		option.delStopLen = 2;
	}
	if (option.addStart == NULL) {
		option.addStart = "{+";
		option.addStartLen = 2;
	}
	if (option.addStop == NULL) {
		option.addStop = "+}";
		option.addStopLen = 2;
	}

	if (!option.printCommon && !option.printAdded && !option.printDeleted)
		option.needMarkers = false;

	if ((!option.printAdded + !option.printDeleted + !option.printCommon) == 2)
		option.needStartStop = false;

	postProcessOptions();

	checkOverlap();
END_FUNCTION
