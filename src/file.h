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

#ifndef FILE_H
#define FILE_H

#include "buffer.h"

/* Note: files opened for write can also be read. However, not vice versa. */
typedef enum {
	FILE_READ,
	FILE_WRITE
} FileMode;

typedef enum {
	EOF_NO,
	EOF_COMING,
	EOF_HIT
} EOFState;

#ifdef PAGE_SIZE
#define FILE_BUFFER_SIZE PAGE_SIZE
#else
#define FILE_BUFFER_SIZE 4096
#endif

struct FileVtable;

typedef struct File {
	/* FD this struct is buffering */
	int fd;
	/* 0, or the error code of the failed operation. Further operations will
	   fail immediately. */
	int errNo;
	/* Current mode associated with the File. */
	FileMode mode;

	/* Buffer containing data from the file. */
	char buffer[FILE_BUFFER_SIZE];
	int bufferFill;
	/* Current index in the buffer. */
	int bufferIndex;

	/* Flag to indicate whether filling the buffer hit end of file. */
	EOFState eof;

	struct FileVtable *vtable;
} File;

typedef struct FileVtable {
	/* Functions to call for fileWrite and filePutc. Different implementations
	   are provided such that diffing with context can be easily done. */
	int (*fileWrite)(struct File *file, const char *buffer, int bytes);
	int (*filePutc)(struct File *file, int c);
	int (*fileFlush)(struct File *file);
} FileVtable;

#define fileFlush(file) ((file)->vtable->fileFlush(file))
#define filePutc(file, c) ((file)->vtable->filePutc((file), (c)))
#define fileWrite(file, buffer, bytes) ((file)->vtable->fileWrite((file), (buffer), (bytes)))


File *fileWrapFD(int fd, FileMode mode);
File *fileOpen(const char *name, FileMode mode);
int fileGetc(File *file);
int fileUngetc(File *file, int c);
int fileClose(File *file);
int filePuts(File *file, const char *string);
int fileRewind(File *file, FileMode mode);
int fileError(File *file);
int fileGetErrno(File *file);
int fileEof(File *file);
void fileClearEof(File *file);

#endif
