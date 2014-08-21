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

#ifndef BUFFER_H
#define BUFFER_H

#include "vector.h"

typedef VECTOR(char, CharBuffer);

void initContextBuffers(void);
void addchar(char c, bool common);
void printLineNumbers(int oldLineNumber, int newLineNumber);
void writeString(const char *string, size_t bytes);

#define INITIAL_BUFFER_SIZE 80
#define INITIAL_WORD_BUFFER_SIZE 32

#endif
