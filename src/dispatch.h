/* Copyright (C) 2008 G.P. Halkes
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

#ifndef DISPATH_H
#define DISPATH_H

#include "definitions.h"

#define CHARLIST SWITCH_UNICODE(CharList, void)
/* The FPTR macro serves only as a placeholder to allow easy generation of the
   code in dispatch_autogen.h. */
#define FPTR

typedef struct {
	/** Get the next character from the input.
		@param file The file to read.
		@return True if the end of the file has not been reached.
	*/
	FPTR bool (*getNextCharDT)(Stream *file);

	/** Test if a character is a whitespace character. */
	FPTR bool (*isWhitespaceDT)(void);

	/** Test if a character is a delimiter character. */
	FPTR bool (*isDelimiterDT)(void);

	/** Write a character to a token file.
		@param file The file to write to.
	*/
	FPTR void (*writeTokenCharDT)(InputFile *file);

	/** Write a character to a token file.
		@param file The file to write to.
	*/
	FPTR void (*writeWhitespaceCharDT)(InputFile *file);

	/** Write a delimiter to the whitespace file.
		@param file The file to write to.
	*/
	FPTR void (*writeWhitespaceDelimiterDT)(InputFile *file);

	/** Add all characters to the specified list or bitmap.
		@param chars The string with characters to add to the list or bitmap.
		@param list The list to add to.
		@param bitmap. The bitmap to add to.

		If in UTF-8 mode, the characters will be added to the list. Otherwise to
		the bitmap.
	*/
	FPTR void (*addCharactersDT)(const char *chars, size_t length, CHARLIST *list, char bitmap[BITMASK_SIZE]);

	/** Check for overlap in whitespace and delimiter sets. */
	FPTR void (*checkOverlapDT)(void);

	FPTR void (*setPunctuationDT)(void);
	FPTR void (*initOptionsDT)(void);
	FPTR void (*postProcessOptionsDT)(void);
} DispatchTable;

extern DispatchTable *dispatch;

#include "dispatch_autogen.h"
#endif
