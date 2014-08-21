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

#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "definitions.h"

/** Alert the user of a fatal error and quit.
    @param fmt The format string for the message. See fprintf(3) for details.
    @param ... The arguments for printing.
*/
void fatal(const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(2);
}

/** Print an out of memory message and exit. */
void outOfMemory(void) {
	fatal("%s", _("Out of memory"));
}

/** Perform tolower for the ASCII character set. */
int ASCIItolower(int c) {
	return (c >= 0x41 && c <= 0x5A) ? c | 0x20 : c;
}

#ifdef NO_STRDUP
char *strdupA(const char *orig) {
	char *result;
	if ((result = malloc(strlen(orig) * sizeof(char))) == NULL)
		return NULL;
	return strcpy(result, orig);
}
#endif

char *safe_strdup(const char *orig) {
	char *result = strdupA(orig);
	if (result == NULL)
		outOfMemory();
	return result;
}

void *safe_malloc(size_t size) {
	void *result = malloc(size);

	if (result == NULL)
		outOfMemory();
	return result;
}

void *safe_calloc(size_t size) {
	void *result = calloc(1, size);

	if (result == NULL)
		outOfMemory();
	return result;
}

void *safe_realloc(void *ptr, size_t size) {
	if ((ptr = realloc(ptr, size)) == NULL)
		outOfMemory();
	return ptr;
}
