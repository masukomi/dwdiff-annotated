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

#ifndef STREAM_H
#define STREAM_H

typedef struct Stream Stream;

#include "definitions.h"
#include "file.h"
#include "util.h"
#include "unicode.h"

typedef struct StreamVtable {
	int (*getChar)(struct Stream *);
	int (*ungetChar)(struct Stream *, int c);
} StreamVtable;

struct Stream {
	StreamVtable *vtable;
	union {
		File *file;
		struct {
			const char *string;
			size_t length;
			size_t index;
		} string;
	} data;
#ifdef USE_UNICODE
	/* Buffered character for grapheme cluster breaking. */
	UChar32 bufferedChar;
	int32_t lastClusterCategory;

	/* High surrogate buffer for converting from UTF-16 to UTF-8. */
	UChar32 highSurrogate;

	/* Character already read when checking for low surrogate. */
	UChar32 nextChar;
#endif
};

Stream *newFileStream(File *file);
Stream *newStringStream(const char *string, size_t length);
bool isFileStream(const Stream *stream);

#define sferror(s) (fileError((s)->data.file))
#define sfflush(s) (fileFlush((s)->data.file))
#define srewind(s) (fileRewind((s)->data.file, FILE_READ))
#define sfeof(s) (fileEof((s)->data.file))

// Note: this gets a single byte character, rather than a UTF-8 character
#define sgetc(s) (fileGetc((s)->data.file))
#define sputc(s, c) (filePutc((s)->data.file, c))
#define swrite(s, buf, bytes) (fileWrite((s)->data.file, buf, bytes))
#define sgeterrno(s) (fileGetErrno((s)->data.file))

void sfclose(Stream *stream);

#endif
