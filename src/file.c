/* Copyright (C) 2008-2011 G.P. Halkes
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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "definitions.h"
#include "file.h"
#include "util.h"
#include "option.h"

static int filePutcReal(File *file, int c);
static int fileWriteReal(File *file, const char *buffer, int bytes);
static int fileFlushReal(File *file);

static FileVtable vtableReal = { fileWriteReal, filePutcReal, fileFlushReal };

/** Wrap a file descriptor in a @a File struct. */
File *fileWrapFD(int fd, FileMode mode) {
	File *retval = malloc(sizeof(File));

	if (retval == NULL)
		return NULL;

	retval->fd = fd;
	retval->errNo = 0;
	retval->bufferFill = 0;
	retval->bufferIndex = 0;
	retval->eof = EOF_NO;
	retval->mode = mode;
	retval->vtable = &vtableReal;
	return retval;
}

/** Open a file.
	@param name The name of the file to open.
	@param mode The mode of the file to open.

	@a mode must be one of @a FILE_READ or @a FILE_WRITE. @a FILE_WRITE can
	only be used if compiled with LEAVE_FILES.
*/
File *fileOpen(const char *name, FileMode mode) {
	int fd, openMode;

	switch (mode) {
		case FILE_READ:
			openMode = O_RDONLY;
			break;
#ifdef LEAVE_FILES
		case FILE_WRITE:
			openMode = O_RDWR | O_CREAT | O_TRUNC;
			break;
#endif
		default:
			PANIC();
	}

	if ((fd = open(name, openMode, 0600)) < 0)
		return NULL;

	return fileWrapFD(fd, mode);
}

/** Get the next character from a @a File. */
int fileGetc(File *file) {
	ASSERT(file->mode == FILE_READ);
	if (file->errNo != 0)
		return EOF;

	if (file->bufferIndex >= file->bufferFill) {
		ssize_t bytesRead = 0;

		if (file->eof != EOF_NO) {
			file->eof = EOF_HIT;
			return EOF;
		}

		/* Use while loop to allow interrupted reads */
		while (1) {
			ssize_t retval = read(file->fd, file->buffer + bytesRead, FILE_BUFFER_SIZE - bytesRead);
			if (retval == 0) {
				file->eof = EOF_COMING;
				break;
			} else if (retval < 0) {
				if (errno == EINTR)
					continue;
				file->errNo = errno;
				break;
			} else {
				bytesRead += retval;
				if (bytesRead == FILE_BUFFER_SIZE)
					break;
			}
		}
		if (file->errNo != 0)
			return EOF;
		if (bytesRead == 0) {
			file->eof = EOF_HIT;
			return EOF;
		}
		file->bufferFill = bytesRead;
		file->bufferIndex = 0;
	}

	return (unsigned char) file->buffer[file->bufferIndex++];
}

/** Push a character back into the buffer for a @a File. */
int fileUngetc(File *file, int c) {
	ASSERT(file->mode == FILE_READ);
	if (file->errNo != 0)
		return EOF;

	ASSERT(file->bufferIndex > 0);
	return file->buffer[--file->bufferIndex] = (unsigned char) c;
}

/** Flush the buffer associated with a @a File to disk. */
static int flushBuffer(File *file) {
	ssize_t bytesWritten = 0;

	if (file->mode == FILE_READ)
		return 0;

	if (file->errNo != 0)
		return EOF;

	if (file->bufferFill == 0)
		return 0;

	/* Use while loop to allow interrupted reads */
	while (1) {
		ssize_t retval = write(file->fd, file->buffer + bytesWritten, file->bufferFill - bytesWritten);
		if (retval == 0) {
			PANIC();
		} else if (retval < 0) {
			if (errno == EINTR)
				continue;
			file->errNo = errno;
			return EOF;
		} else {
			bytesWritten += retval;
			if (bytesWritten == file->bufferFill)
				break;
		}
	}

	file->bufferFill = 0;
	return bytesWritten;
}

/** Close a @a File. */
int fileClose(File *file) {
	int retval = flushBuffer(file);

	if (close(file->fd) < 0 && retval == 0) {
		retval = errno;
	} else {
		retval = 0;
	}

	free(file);
	return retval;
}

static int fileFlushReal(File *file) {
	return flushBuffer(file) == EOF ? -1 : 0;

	/* The code below also fsync's the data to disk. However, this should not
	   be necessary to allow another program to read the entire file. It does
	   however slow the program down, so we skip it. */

/*	if (flushBuffer(file) == EOF)
		return -1;
	fsync(file->fd);
	return 0; */
}

/** Write a character to a @a File. */
static int filePutcReal(File *file, int c) {
	ASSERT(file->mode == FILE_WRITE);
	if (file->errNo != 0)
		return EOF;

	if (file->bufferFill >= FILE_BUFFER_SIZE) {
		if (flushBuffer(file) == EOF)
			return EOF;
	}

	file->buffer[file->bufferFill++] = (unsigned char) c;
	return 0;
}

/** Write a buffer to a @a File. */
static int fileWriteReal(File *file, const char *buffer, int bytes) {
	ASSERT(file->mode == FILE_WRITE);
	if (file->errNo != 0)
		return EOF;

	while (1) {
		size_t minLength = FILE_BUFFER_SIZE - file->bufferFill < bytes ? FILE_BUFFER_SIZE - file->bufferFill : bytes;

		memcpy(file->buffer + file->bufferFill, buffer, minLength);
		file->bufferFill += minLength;
		bytes -= minLength;
		if (bytes == 0)
			return 0;

		if (flushBuffer(file) == EOF)
			return EOF;
		buffer += minLength;
	}
}

/** Write a string to a @a File. */
int filePuts(File *file, const char *string) {
	size_t length;

	ASSERT(file->mode == FILE_WRITE);
	if (file->errNo != 0)
		return EOF;

	length = strlen(string);

	return fileWrite(file, string, length);
}

/** Rewind a @a File, changing its mode. */
int fileRewind(File *file, FileMode mode) {
	ASSERT(mode == FILE_READ || (file->mode == FILE_WRITE && mode == FILE_WRITE));

	if (flushBuffer(file) != 0)
		return -1;

	if (lseek(file->fd, 0, SEEK_SET) < 0) {
		file->errNo = errno;
		return -1;
	}
	file->eof = EOF_NO;
	file->mode = mode;
	return 0;
}

/** Return if a @a File is in error state. */
int fileError(File *file) {
	return file->errNo != 0;
}

/** Return if a @a File is at the end of file. */
int fileEof(File *file) {
	return file->eof == EOF_HIT;
}

/** Get the errno for the failing action on a @a File. */
int fileGetErrno(File *file) {
	return file->errNo;
}

void fileClearEof(File *file) {
	file->eof = EOF_COMING;
}
