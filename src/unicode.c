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
#ifdef USE_UNICODE

#include <stdlib.h>
#include <string.h>
#include <unicode/unorm.h>
#include <unicode/ustring.h>

#include "definitions.h"
#include "unicode.h"
#include "static_assert.h"
#include "option.h"

/*****************************************************************************
Input and output of UTF-8 streams
*****************************************************************************/

/** Read in one UTF-8 character.
    @param stream The @a Stream to read.
	@retval The first UCS4 character found in the file or an error code.

	This function does not try to decode sequences that should not occur in
	valid UTF-8. Because it is more likely that the input is non-UTF-8 than
	that some program created invalid UTF-8 this is the more sane option.
*/
static UChar32 getuc(Stream *stream) {
	int c, bytesLeft;
	UChar32 retval, least;

	if ((c = stream->vtable->getChar(stream)) == EOF)
		return EOF;

	switch (c) {
		case  0: case  1: case  2: case  3: case  4: case  5: case  6: case  7:
		case  8: case  9: case 10: case 11: case 12: case 13: case 14: case 15:
		case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
		case 24: case 25: case 26: case 27: case 28: case 29: case 30: case 31:
		case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
		case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47:
		case 48: case 49: case 50: case 51: case 52: case 53: case 54: case 55:
		case 56: case 57: case 58: case 59: case 60: case 61: case 62: case 63:
		case 64: case 65: case 66: case 67: case 68: case 69: case 70: case 71:
		case 72: case 73: case 74: case 75: case 76: case 77: case 78: case 79:
		case 80: case 81: case 82: case 83: case 84: case 85: case 86: case 87:
		case 88: case 89: case 90: case 91: case 92: case 93: case 94: case 95:
		case  96: case  97: case  98: case  99: case 100: case 101: case 102: case 103:
		case 104: case 105: case 106: case 107: case 108: case 109: case 110: case 111:
		case 112: case 113: case 114: case 115: case 116: case 117: case 118: case 119:
		case 120: case 121: case 122: case 123: case 124: case 125: case 126: case 127:
			return c;
		case 128: case 129: case 130: case 131: case 132: case 133: case 134: case 135:
		case 136: case 137: case 138: case 139: case 140: case 141: case 142: case 143:
		case 144: case 145: case 146: case 147: case 148: case 149: case 150: case 151:
		case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159:
		case 160: case 161: case 162: case 163: case 164: case 165: case 166: case 167:
		case 168: case 169: case 170: case 171: case 172: case 173: case 174: case 175:
		case 176: case 177: case 178: case 179: case 180: case 181: case 182: case 183:
		case 184: case 185: case 186: case 187: case 188: case 189: case 190: case 191:
			return UEINVALID;
		case 192: case 193:
			return UEOVERLONG;
		case 194: case 195: case 196: case 197: case 198: case 199: case 200: case 201:
		case 202: case 203: case 204: case 205: case 206: case 207: case 208: case 209:
		case 210: case 211: case 212: case 213: case 214: case 215: case 216: case 217:
		case 218: case 219: case 220: case 221: case 222: case 223:
			least = 0x80;
			bytesLeft = 1;
			retval = c & 0x1F;
			break;
		case 224: case 225: case 226: case 227: case 228: case 229: case 230: case 231:
		case 232: case 233: case 234: case 235: case 236: case 237: case 238: case 239:
			least = 0x800;
			bytesLeft = 2;
			retval = c & 0x0F;
			break;
		case 240: case 241: case 242: case 243: case 244:
			least = 0x10000L;
			bytesLeft = 3;
			retval = c & 0x07;
			break;
		case 245: case 246: case 247: case 248: case 249: case 250: case 251: case 252:
		case 253: case 254: case 255:
			return UETOOLARGE;
		default:
			PANIC();
	}

	for (; bytesLeft > 0; bytesLeft--) {
		if ((c = stream->vtable->getChar(stream)) == EOF)
			return UEINVALID;
		if ((c & 0xC0) != 0x80) {
			stream->vtable->ungetChar(stream, c);
			return UEINVALID;
		}
		retval = (retval << 6) | (c & 0x3F);
	}

	if (retval < least)
		return UEOVERLONG;
	if (retval > 0x10FFFFL)
		return UETOOLARGE;
	return retval;
}

#define REPLACEMENT_CHARACTER 0xFFFD;

/** Get a character from a @a Stream, converting strange characters to REPLACEMENT CHARACTER.
    @param stream The @a Stream to read from.
    @return a UCS-4 character or EOF on end-of-file or error.
*/
UChar32 getucFiltered(Stream *stream) {
	UChar32 c;

	if (stream->nextChar >= 0) {
		c = stream->nextChar;
		stream->nextChar = -1;
	} else {
		c = getuc(stream);
	}

	if (c < 0) {
		return c == EOF ? EOF : REPLACEMENT_CHARACTER;
	} else if ((c & 0xF800L) == 0xD800L) {
		UChar32 clow;

		/* If we just encountered a low surrogate, just replace. */
		if ((c & 0xDC00) == 0xDC00)
			return REPLACEMENT_CHARACTER;

		clow = getuc(stream);

		if ((clow & 0xFC00) != 0xDC00) {
			/* Buffer last read char so we can return next time. Return
			   replacement for high surrogate encountered earlier. */
			stream->nextChar = clow;
			return REPLACEMENT_CHARACTER;
		}

		c = clow + (c << 10) + 0x10000 - (0xD800 << 10) - 0xDC00;
	}

	return c;
}

/** Convert one UCS-4 character to UTF-8.
	@param c The character to convert.
	@param buffer The buffer to write the result.
	@return the number of @a char's written to @a buffer.

	This function does not check for high/low surrogates.
*/
int convertToUTF8(UChar32 c, char *buffer) {
	int bytes, i;

	if (c > 0x10000L)
		bytes = 4;
	else if (c > 0x800)
		bytes = 3;
	else if (c > 0x80)
		bytes = 2;
	else {
		buffer[0] = c;
		return 1;
	}

	for (i = bytes - 1; i >= 0; i--) {
		buffer[i] = (c & 0x3F) | 0x80;
		c >>= 6;
	}
	buffer[0] |= 0xF0 << (4 - bytes);
	return bytes;
}

/** Convert one UCS-4 character to UTF-8, filtering surrogates.
	@param c The character to convert.
	@param buffer The buffer to write the result.
	@param highSurrogate The location where highSurrogates are stored for this conversion.
	@return the number of @a char's written to @a buffer.

	This function also translates high/low surrogate pairs to a single UCS-4
	character. Therefore it can also be used to convert UTF-16 to UTF-8. Note
	that the characters written are supposed to form a valid UTF-16 string, so
	every high surrogate has to be followed by a low surrogate, and every low
	surrogate has to be preceeded by a high surrogate.
*/
int filteredConvertToUTF8(UChar32 c, char *buffer, UChar32 *highSurrogate) {
	if ((c & 0xFC00) == 0xD800) {
		*highSurrogate = c;
		return 0;
	}

	if ((c & 0xFC00) == 0xDC00) {
		ASSERT(*highSurrogate != 0);
		c = c + ((*highSurrogate) << 10) + 0x10000 - (0xD800 << 10) - 0xDC00;
		*highSurrogate = 0;
	}

	return convertToUTF8(c, buffer);
}

/** Write out one UCS-4 character as UTF-8.
    @param stream The @a Stream to write.
	@param c The character to write.
	@return 0 for succes, EOF for failure.

	See @a filteredConvertToUTF8 for more details on the conversion.
*/
int putuc(Stream *stream, UChar32 c) {
	char encoded[4];
	int bytes = filteredConvertToUTF8(c, encoded, &stream->highSurrogate);

	if (bytes == 0)
		return 0;
	return fileWrite(stream->data.file, encoded, bytes) < bytes ? EOF : 0;
}


#define MAX_GCB_CLASS 13
/* This will warn us if we compile against a new library with more cluster
   break classes. */
#ifdef DEBUG
static_assert(U_GCB_COUNT <= MAX_GCB_CLASS);
#endif

/* Latest version of the algorithm, with all its classes, can be found at
   http://www.unicode.org/reports/tr29/
*/

/* The values of the constants _should_ not change from version to version,
   but better safe than sorry. */
static_assert(U_GCB_OTHER == 0);
static_assert(U_GCB_CONTROL == 1);
static_assert(U_GCB_CR == 2);
static_assert(U_GCB_EXTEND == 3);
static_assert(U_GCB_L == 4);
static_assert(U_GCB_LF == 5);
static_assert(U_GCB_LV == 6);
static_assert(U_GCB_LVT == 7);
static_assert(U_GCB_T == 8);
static_assert(U_GCB_V == 9);
#if ICU_VERSION_MAJOR_NUM > 3
static_assert(U_GCB_SPACING_MARK == 10);
static_assert(U_GCB_PREPEND == 11);
#if ICU_VERSION_MAJOR_NUM > 4
static_assert(U_GCB_REGIONAL_INDICATOR == 12);
#endif
#endif

/** Table used for determining wheter a grapheme break exists between two characters. */
static char clusterContinuationTable[MAX_GCB_CLASS][MAX_GCB_CLASS] = {
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, CRLF_GRAPHEME_CLUSTER_BREAK, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
	{0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
	{1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1}
};

/** Table used for determining wheter a break exists between two characters when using backspace to overstrike characters. */
static char backspaceContinuationTable[MAX_GCB_CLASS][MAX_GCB_CLASS] = {
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static void addUCS4ToUTF16Buffer(UTF16Buffer *buffer, UChar32 c);

/** Retrieve the Grapheme cluster break category for a character.

	This ensures that cluster categories above the compiled-in maximum are
	folded to the "other" category. Should a newer version of the ICU library
	return values higher than the expected value, this will not pose
	significant problems.
*/
static int getClusterCategory(UChar32 c) {
	int category = u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK);

	return category >= MAX_GCB_CLASS ? U_GCB_OTHER : category;
}


/** Get the next Cluster from a stream.
    @param stream The @a Stream to read.
    @param buffer The @a UTF16Buffer to store the Cluster.
    @param continuationTable The table to use for looking up continuations.
    @return a boolean indicating whether the reading was successful.
*/
static bool getClusterInternal(Stream *stream, UTF16Buffer *buffer, char continuationTable[MAX_GCB_CLASS][MAX_GCB_CLASS]) {
	int32_t newClusterCategory;

	/* Empty buffer. */
	buffer->used = 0;

	/* Check if we have already read a character on a previous occasion. */
	if (stream->bufferedChar < 0) {
		if ((stream->bufferedChar = getucFiltered(stream)) < 0)
			return false;
		newClusterCategory = getClusterCategory(stream->bufferedChar);
	} else {
		newClusterCategory = stream->lastClusterCategory;
	}

	/* Append characters as long as the cluster categories dictate. */
	do {
		stream->lastClusterCategory = newClusterCategory;
		addUCS4ToUTF16Buffer(buffer, stream->bufferedChar);
		if ((stream->bufferedChar = getucFiltered(stream)) < 0) {
			if (isFileStream(stream))
				fileClearEof(stream->data.file);
			newClusterCategory = 0;
			break;
		}
		newClusterCategory = getClusterCategory(stream->bufferedChar);
	} while (continuationTable[stream->lastClusterCategory][newClusterCategory]);

	/* Save grapheme category for next call. */
	stream->lastClusterCategory = newClusterCategory;
	return true;
}

/** Get the next Grapheme Cluster from a stream.
    @param stream The @a Stream to read.
    @param buffer The @a UTF16Buffer to store the Grapheme Cluster.
    @return a boolean indicating whether the reading was successful.
*/
bool getCluster(Stream *stream, UTF16Buffer *buffer) {
	return getClusterInternal(stream, buffer, clusterContinuationTable);
}

/** Get the next Backspace Cluster from a stream.
    @param stream The @a Stream to read.
    @param buffer The @a UTF16Buffer to store the Backspace Cluster.
    @return a boolean indicating whether the reading was successful.
*/
bool getBackspaceCluster(Stream *stream, UTF16Buffer *buffer) {
	return getClusterInternal(stream, buffer, backspaceContinuationTable);
}


/*****************************************************************************
UTF16Buffer operations
*****************************************************************************/

/** Append a 24 bit UCS-4 character to a UTF-16 encoded buffer.
    @param buffer The UTF16Buffer structure to append to.
    @param c The character to append.
*/
static void addUCS4ToUTF16Buffer(UTF16Buffer *buffer, UChar32 c) {
	if (c > 0x10000) {
		VECTOR_APPEND(*buffer, 0xD800 - (0x10000 >> 10) + (c >> 10));
		VECTOR_APPEND(*buffer, 0xDC00 | (c & 0x3FF));
	} else {
		VECTOR_APPEND(*buffer, (UChar) c);
	}
}

/** Decompose a Grapheme Cluster according to the standard decomposition.
    @param c The Grapheme Cluster to decompose.
*/
void decomposeChar(CharData *c) {
	UErrorCode error = U_ZERO_ERROR;
	size_t requiredLength;
	requiredLength = unorm_normalize(c->UTF8Char.original.data, c->UTF8Char.original.used, option.decomposition, 0,
		c->UTF8Char.converted.data, c->UTF8Char.converted.allocated, &error);

	if (requiredLength > c->UTF8Char.converted.allocated) {
		ASSERT(error == U_BUFFER_OVERFLOW_ERROR);
		error = U_ZERO_ERROR;

		VECTOR_ALLOCATE(c->UTF8Char.converted, requiredLength * sizeof(UChar));
		requiredLength = unorm_normalize(c->UTF8Char.original.data, c->UTF8Char.original.used, option.decomposition, 0,
			c->UTF8Char.converted.data, c->UTF8Char.converted.allocated, &error);
	}
	ASSERT(U_SUCCESS(error));
	c->UTF8Char.converted.used = requiredLength;
}

/** Fold the case of a Grapheme Cluster.
    @param c The Grapheme Cluster to case-fold.
*/
void casefoldChar(CharData *c) {
	UErrorCode error = U_ZERO_ERROR;
	size_t requiredLength;

	requiredLength = u_strFoldCase(c->UTF8Char.casefolded.data, c->UTF8Char.casefolded.allocated, c->UTF8Char.converted.data,
		c->UTF8Char.converted.used, U_FOLD_CASE_DEFAULT, &error);

	if (requiredLength > c->UTF8Char.casefolded.allocated) {
		ASSERT(error == U_BUFFER_OVERFLOW_ERROR);
		error = U_ZERO_ERROR;

		c->UTF8Char.casefolded.data = realloc(c->UTF8Char.casefolded.data, requiredLength * sizeof(UChar));
		c->UTF8Char.casefolded.allocated = requiredLength;
		requiredLength = u_strFoldCase(c->UTF8Char.casefolded.data, c->UTF8Char.casefolded.allocated, c->UTF8Char.converted.data,
			c->UTF8Char.converted.used, U_FOLD_CASE_DEFAULT, &error);
	}
	ASSERT(U_SUCCESS(error));
	c->UTF8Char.casefolded.used = requiredLength;
}

/* An initial size of 4 will provide sufficient space for most graphmes. There
   may be some cases in which this is not enough, but the buffer will be grown
   if this is the case. The reason for the ifndef is that for testing of the
   code to grow the buffers, we need to be able to set the size to 1.
*/
#ifndef UTF16BUFFER_INITIAL_SIZE
#define UTF16BUFFER_INITIAL_SIZE 4
#endif

/** Compare two UTF16Buffer's for sorting purposes */
int compareUTF16Buffer(const UTF16Buffer *a, const UTF16Buffer *b) {
	size_t minlen = a->used < b->used ? a->used : b->used;
	int result;

	result = memcmp(a->data, b->data, minlen * sizeof(UChar));

	if (result)
		return result;

	if (b->used > minlen)
		return -1;
	if (a->used > minlen)
		return 1;
	return 0;
}

#define UTF16CharCondition(name, condition) bool isUTF16##name(UTF16Buffer *buffer) { \
	UChar32 c; size_t i; \
	for (i = 0; i < buffer->used; i++) { \
\
		if ((buffer->data[i] & 0xFC00) == 0xD800) { \
			ASSERT(i + 1 < buffer->used); \
			c = ((((UChar32) buffer->data[i]) & 0x3FF) << 10) + 0x10000 + (buffer->data[i + 1] & 0x3FF); \
			i++; \
		} else { \
			c = buffer->data[i]; \
		} \
		if (!(condition)) \
			return false; \
	} \
	return true; \
} \

/** Check if a @a UTF16Buffer contains a punctuation character. */
UTF16CharCondition(Punct, U_GET_GC_MASK(c) & option.punctuationMask)
/** Check if a @a UTF16Buffer contains a punctuation character. */
UTF16CharCondition(Whitespace, u_isUWhiteSpace(c))

#endif
