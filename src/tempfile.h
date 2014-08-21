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

#ifndef TEMPFILE_H
#define TEMPFILE_H

#include "stream.h"

#define TEMPLATE "/tmp/dwdiffXXXXXXXX"
#define TEMPLATE_LENGTH sizeof(TEMPLATE)

typedef struct {
	Stream *stream;
	char name[TEMPLATE_LENGTH];
	bool closed;
} TempFile;

TempFile *tempFile(void);
void closeTempFile(TempFile *file);
void resetTempFiles(void);

#endif
