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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "definitions.h"
#include "stream.h"

static TempFile files[6];
static unsigned openIndex = 0;

#ifndef LEAVE_FILES
static bool inited;

#endif

/** Create a temporary file. */
TempFile *tempFile(void) {
#ifndef LEAVE_FILES
	int fd;
#endif

	ASSERT(openIndex < sizeof(files) / sizeof(files[0]));

#ifdef LEAVE_FILES
	strcpy(files[openIndex].name, "dwdiffTemp");
	sprintf(files[openIndex].name + strlen(files[openIndex].name), "%d", openIndex);
	if ((files[openIndex].stream = newFileStream(fileOpen(files[openIndex].name, FILE_WRITE))) == NULL)
		return NULL;
#else
	/* Create temporary file. */
	if (!inited) {
		/* Make sure the umask is set so that we don't introduce a security risk. */
		umask(~S_IRWXU);
		/* Make sure we will remove temporary files on exit. */
		atexit(resetTempFiles);
		inited = true;
	}

	strcpy(files[openIndex].name, TEMPLATE);
	if ((fd = mkstemp(files[openIndex].name)) < 0)
		return NULL;
	if ((files[openIndex].stream = newFileStream(fileWrapFD(fd, FILE_WRITE))) == NULL)
		return NULL;
#endif

	return files + openIndex++;
}

/** Closes a temporary file by closing its stream.
    @param file The ::TempFile to close.

    This function will close the stream, but it does not remove the temporary
    file. This will be done by the ::cleanup function at exit. It is
    permissible to call this function with the same argument more than once.
*/
void closeTempFile(TempFile *file) {
	if (!file->closed) {
		sfclose(file->stream);
		file->closed = true;
	}
}

/** Remove all the created temporary files and reset the data structures. */
void resetTempFiles(void) {
	unsigned i;
	for (i = 0; i < openIndex; i++) {
		closeTempFile(&files[i]);
		remove(files[i].name);
		files[i].closed = false;
		files[i].stream = NULL;
		memset(files[i].name, 0, sizeof(files[i].name));
	}
	openIndex = 0;
}
