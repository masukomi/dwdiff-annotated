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
#include <ctype.h>
#include <errno.h>
#include "definitions.h"
#include "option.h"
#include "util.h"
#include "stream.h"
#include "buffer.h"
#include "unicode.h"
#include "diff/diff.h"
#include "hashtable.h"

static const char resetColor[] = "\033[0m";
static const char eraseLine[] = "\033[K";
static unsigned int oldLineNumber = 1, newLineNumber = 1;
static bool lastWasLinefeed = true, lastWasDelete = false, lastWasCarriageReturn = false;

/** Check whether the last-read character equals a certain value.

    This only works correctly if @p c is a character which will always be put
    into its own grapheme cluster, like control characters.
*/
static bool charDataEquals(int c) {
#ifdef USE_UNICODE
	if (UTF8Mode)
		return charData.UTF8Char.original.data[0] == c;
#endif
	return charData.singleChar == c;
}

static bool readNextChar(Stream *stream) {
#ifdef USE_UNICODE
	if (UTF8Mode)
		return getBackspaceCluster(stream, &charData.UTF8Char.original);
#endif
	charData.singleChar = stream->vtable->getChar(stream);
	return charData.singleChar != EOF;
}

static void addCharData(bool common) {
#ifdef USE_UNICODE
	if (UTF8Mode) {
		size_t i;
		char encoded[4];
		int bytes, j;
		UChar32 highSurrogate = 0;

		for (i = 0; i < charData.UTF8Char.original.used; i++) {
			bytes = filteredConvertToUTF8(charData.UTF8Char.original.data[i], encoded, &highSurrogate);

			for (j = 0; j < bytes; j++)
				addchar(encoded[j], common);
		}
		return;
	}
#endif
	addchar(charData.singleChar, common);
}

/* Note: ADD should be 0, OLD_COMMON should be DEL + COMMON. */
typedef enum {ADD, DEL, COMMON, OLD_COMMON} Mode;

/** If the last character printed was a newline, do some special handling.
	@param mode What kind of output is generated next.
*/
static void doPostLinefeed(Mode mode) {
	if (lastWasLinefeed) {
		if (mode & COMMON)
			mode = COMMON;

		lastWasLinefeed = false;
		if (option.lineNumbers) {
			if (option.colorMode && mode != COMMON)
				writeString(resetColor, sizeof(resetColor) - 1);
			printLineNumbers(oldLineNumber, newLineNumber);
		}
		if (option.needStartStop && mode != COMMON) {
			if (option.colorMode) {
				if (mode == ADD)
					writeString(option.addColor, option.addColorLen);
				else
					writeString(option.delColor, option.delColorLen);
			}
			if (option.repeatMarkers) {
				if (mode == ADD)
					writeString(option.addStart, option.addStartLen);
				else
					writeString(option.delStart, option.delStartLen);
			}
		}
	}
}


/** Handle a single whitespace character.
	@param print Skip or print.
	@param mode What type of output to generate.
*/
static void handleWhitespaceChar(bool print, Mode mode) {
	if (print) {
		doPostLinefeed(mode);

		/* Less mode also over-strikes whitespace */
		if (option.less && !charDataEquals('\n') && !charDataEquals('\r') && mode == DEL) {
			addchar('_', mode & COMMON);
			addchar('\010', mode & COMMON);
		} else if (option.needStartStop && (mode & COMMON) != COMMON && (charDataEquals('\r')  || (!lastWasCarriageReturn && charDataEquals('\n')))) {
			if (option.repeatMarkers) {
				if (mode == ADD)
					writeString(option.addStop, option.addStopLen);
				else
					writeString(option.delStop, option.delStopLen);
			}

			if (option.colorMode)
				/* Erase rest of line so it will use the correct background color */
				writeString(eraseLine, sizeof(eraseLine) - 1);
		}
		addCharData(mode & COMMON);

		if (charDataEquals('\n'))
			lastWasLinefeed = true;

		lastWasCarriageReturn = charDataEquals('\r');
	}

	if (charDataEquals('\n')) {
		switch (mode) {
			case COMMON:
			case ADD:
				newLineNumber++;
				break;
			case OLD_COMMON:
			case DEL:
				oldLineNumber++;
				break;
			default:
				PANIC();
		}
	}
}

/** Skip or print the next bit of whitespace from @a file.
	@param file The file with whitespace.
	@param print Skip or print.
	@param mode What type of output to generate.
*/
static void handleNextWhitespace(InputFile *file, bool print, Mode mode) {
	if (file->whitespaceBufferUsed) {
		Stream *stream = newStringStream(file->whitespaceBuffer.data, file->whitespaceBuffer.used);
		while (readNextChar(stream))
			handleWhitespaceChar(print, mode);
		free(stream);
		file->whitespaceBuffer.used = 0;
		file->whitespaceBufferUsed = false;
	} else {
		while (readNextChar(file->whitespace->stream)) {
			if (charDataEquals(0))
				return;

			if (charDataEquals(1)) {
				if (!readNextChar(file->whitespace->stream))
					fatal(_("Error reading back input\n"));
			}
			handleWhitespaceChar(print, mode);
		}
	}
}

/** Skip or print the next bit of whitespace from the new or old file, keeping
	the other file synchronized as far as line numbers are concerned.
	@param printNew Use the new file for printing instead of the old file.

	This is only for printing common text, and will skip over the old
	whitespace.
*/
static void handleSynchronizedNextWhitespace(bool printNew) {
#ifdef USE_UNICODE
	static_assert(CRLF_GRAPHEME_CLUSTER_BREAK == 0);
#endif

	bool BValid = true;
	unsigned int *lineNumberA, *lineNumberB;
	Stream *whitespaceA, *whitespaceB;

	Stream *oldFileWhitespaceStream = option.oldFile.whitespaceBufferUsed ?
			newStringStream(option.oldFile.whitespaceBuffer.data, option.oldFile.whitespaceBuffer.used) :
			option.oldFile.whitespace->stream;
	Stream *newFileWhitespaceStream = option.newFile.whitespaceBufferUsed ?
			newStringStream(option.newFile.whitespaceBuffer.data, option.newFile.whitespaceBuffer.used) :
			option.newFile.whitespace->stream;

	if (printNew) {
		whitespaceA = newFileWhitespaceStream;
		whitespaceB = oldFileWhitespaceStream;
		lineNumberA = &newLineNumber;
		lineNumberB = &oldLineNumber;
	} else {
		whitespaceA = oldFileWhitespaceStream;
		whitespaceB = newFileWhitespaceStream;
		lineNumberA = &oldLineNumber;
		lineNumberB = &newLineNumber;
	}

	while (readNextChar(whitespaceA)) {
		if (charDataEquals(0))
			break;

		if (charDataEquals(1)) {
			if (!readNextChar(whitespaceA))
				fatal(_("Error reading back input\n"));
		}

		if (option.printCommon) {
			doPostLinefeed(COMMON);

			/* Note that we don't have to check less mode here as we only print
			   common whitespace. */
			addCharData(true);

			if (charDataEquals('\n'))
				lastWasLinefeed = true;

			lastWasCarriageReturn = charDataEquals('\r');
		}

		/* If a newline was found, see if the B file also has a newline. */
		if (charDataEquals('\n')) {
			(*lineNumberA)++;
			/* Only process the B file if it has not reached then end of the token yet. */
			if (BValid) {
				bool result;
				while ((result = readNextChar(whitespaceB))) {
					if (charDataEquals(0))
						break;

					if (charDataEquals(1)) {
						if (!readNextChar(whitespaceB))
							fatal(_("Error reading back input\n"));
					}

					if (charDataEquals('\n')) {
						(*lineNumberB)++;
						break;
					}
				}
				if (charDataEquals(0) || !result)
					BValid = false;
			}
		}
	}

	/* Process any remaining whitespace from the BS file. */
	if (BValid) {
		while (readNextChar(whitespaceB)) {
			if (charDataEquals(0))
				break;

			if (charDataEquals(1)) {
				if (!readNextChar(whitespaceB))
					fatal(_("Error reading back input\n"));
			}

			if (charDataEquals('\n'))
				(*lineNumberB)++;
		}
	}

	if (option.oldFile.whitespaceBufferUsed)
		free(oldFileWhitespaceStream);
	if (option.newFile.whitespaceBufferUsed)
		free(newFileWhitespaceStream);
}

/** Wrapper for addchar which takes printer and less mode into account
	@param mode What type of output to generate.
*/
void addTokenChar(Mode mode) {
	/* Printer mode and less mode do special stuff, per character. */
	if ((option.printer || option.less) && !charDataEquals('\n') && !charDataEquals('\r')) {
		if (mode == DEL) {
			addchar('_', mode & COMMON);
			addchar('\010', mode & COMMON);
		} else if (mode == ADD) {
			addCharData(mode & COMMON);
			addchar('\010', mode & COMMON);
		}
	}
	addCharData(mode & COMMON);
}

/** Skip or print the next token from @a file.
	@param file The file with tokens.
	@param print Skip or print.
	@param mode What type of output to generate.
*/
static void handleNextToken(TempFile *file, bool print, Mode mode) {
	bool empty = true;
	while (readNextChar(file->stream)) {
		if (charDataEquals(0)) {
			/* Check for option.paraDelim _should_ be superfluous, unless there is a bug elsewhere. */
			if (option.paraDelim && print && empty && mode != COMMON) {
				Stream *stream = newStringStream(option.paraDelimMarker, option.paraDelimMarkerLength);
				while (readNextChar(stream)) {
					/* doPostLinefeed only does something if the last character was a line feed. However,
					   the paragraph delimiter may contain line feeds as well, so call doPostLinefeed
					   every time a character was printed. */
					doPostLinefeed(mode);
					addCharData(mode);
				}
				free(stream);
			}
			return;
		}
		empty = false;

		/* Unescape the characters, if necessary. */
		if (charDataEquals(1)) {
			if (!readNextChar(file->stream))
				fatal(_("Error reading back input\n"));
		}

		if (print) {
			doPostLinefeed(mode);

			if (option.needStartStop && (mode & COMMON) != COMMON && (charDataEquals('\r')  || (!lastWasCarriageReturn && charDataEquals('\n')))) {
				if (option.repeatMarkers) {
					if (mode == ADD)
						writeString(option.addStop, option.addStopLen);
					else
						writeString(option.delStop, option.delStopLen);
				}

				if (option.colorMode)
					/* Erase rest of line so it will use the correct background color */
					writeString(eraseLine, sizeof(eraseLine) - 1);
			}

			addTokenChar(mode);

			if (charDataEquals('\n'))
				lastWasLinefeed = true;

			lastWasCarriageReturn = charDataEquals('\r');
		}

		if (charDataEquals('\n')) {
			switch (mode) {
				/* When the newline is a word character rather than a whitespace
				   character, we can safely count old and new lines together
				   for common words. This will keep the line numbers in synch
				   for these cases. Note that this also means that for
				   OLD_COMMON the line counter is not incremented. */
				case COMMON:
					oldLineNumber++;
				case ADD:
					newLineNumber++;
					break;
				case DEL:
					oldLineNumber++;
					break;
				case OLD_COMMON:
					break;
				default:
					PANIC();
			}
		}
	}
}

/** Skip or print the next whitespace and tokens from @a file.
	@param file The @a InputFile to use.
	@param idx The last word to print or skip.
	@param print Skip or print.
	@param mode What type of output to generate.
*/
static void handleWord(InputFile *file, int idx, bool print, Mode mode) {
	while (file->lastPrinted < idx) {
		handleNextWhitespace(file, print, mode);
		handleNextToken(file->tokens, print, mode);
		file->lastPrinted++;
	}
}

/** Print (or skip if the user doesn't want to see) the common words.
	@param idx The last word to print (or skip).
*/
void printToCommonWord(int idx) {
	while (option.newFile.lastPrinted < idx) {
		handleSynchronizedNextWhitespace(!lastWasDelete);
		lastWasDelete = false;
		handleNextToken(option.newFile.tokens, option.printCommon, COMMON);
		handleNextToken(option.oldFile.tokens, false, OLD_COMMON);
		option.newFile.lastPrinted++;
		option.oldFile.lastPrinted++;
	}
}

/** Print (or skip if the user doesn't want to see) words from @a file.
	@param range The range of words to print (or skip).
	@param file The @a InputFile to print from.
	@param mode Either ADD or DEL, used for printing of start/stop markers.
*/
static void printWords(lin start, lin count, InputFile *file, bool print, Mode mode) {
	ASSERT(file->lastPrinted == start);

	/* Print the first word. As we need to add the markers AFTER the first bit of
	   white space, we can't just use handleWord */

	/* Print preceding whitespace. Should not be overstriken, so print as common */
	handleNextWhitespace(file, print, mode + COMMON);
	/* Ensure that the the line numbers etc. get printed before the markers */
	if (print)
		doPostLinefeed(COMMON);

	/* Print start marker */
	if (print && option.needStartStop) {
		if (mode == ADD) {
			if (option.colorMode)
				writeString(option.addColor, option.addColorLen);
			writeString(option.addStart, option.addStartLen);
		} else {
			if (option.colorMode)
				writeString(option.delColor, option.delColorLen);
			writeString(option.delStart, option.delStartLen);
		}
	}

	/* Print first word */
	handleNextToken(file->tokens, print, mode);
	file->lastPrinted++;
	/* Print following words */
	handleWord(file, start + count, print, mode);

	if (print)
		doPostLinefeed(mode);

	/* Print stop marker */
	if (print && option.needStartStop) {
		if (mode == ADD)
			writeString(option.addStop, option.addStopLen);
		else
			writeString(option.delStop, option.delStopLen);

		if (option.colorMode)
			writeString(resetColor, sizeof(resetColor) - 1);
	}
}

/** Print (or skip if the user doesn't want to see) deleted words.
	@param range The range of words to print (or skip).
*/
void printDeletedWords(struct change *script) {
	printWords(script->line0, script->deleted, &option.oldFile, option.printDeleted, DEL);
}

/** Print (or skip if the user doesn't want to see) inserted words.
	@param range The range of words to print (or skip).
*/
void printAddedWords(struct change *script) {
	printWords(script->line1, script->inserted, &option.newFile, option.printAdded, ADD);
}

/** Print (or skip if the user doesn't want to see) the last (common) words of both files. */
void printEnd(void) {
	if (!option.printCommon)
		return;
	while(!sfeof(option.newFile.tokens->stream)) {
		handleSynchronizedNextWhitespace(!lastWasDelete);
		lastWasDelete = false;
		handleNextToken(option.newFile.tokens, true, COMMON);
		handleNextToken(option.oldFile.tokens, false, OLD_COMMON);
	}
}

/** Load a piece of whitespace from file.
	@param file The @a InputFile to read from and store the result.
	@return a boolean indicating whether a newline was found within the whitespace.
*/
static bool loadNextWhitespace(InputFile *file) {
	bool newlineFound = false;

	file->whitespaceBufferUsed = true;
	file->whitespaceBuffer.used = 0;
	while (readNextChar(file->whitespace->stream)) {
		if (charDataEquals(0))
			return newlineFound;

		if (charDataEquals(1)) {
			if (!readNextChar(file->whitespace->stream))
				fatal(_("Error reading back input\n"));
		}

		if (charDataEquals('\n'))
			newlineFound = true;

#ifdef USE_UNICODE
		if (UTF8Mode) {
			size_t i;
			char encoded[4];
			int bytes, j;
			UChar32 highSurrogate = 0;

			for (i = 0; i < charData.UTF8Char.original.used; i++) {
				bytes = filteredConvertToUTF8(charData.UTF8Char.original.data[i], encoded, &highSurrogate);

				for (j = 0; j < bytes; j++)
					VECTOR_APPEND(file->whitespaceBuffer, encoded[j]);
			}
		} else
#endif
		{
			VECTOR_APPEND(file->whitespaceBuffer, charData.singleChar);
		}
	}
	return newlineFound;
}

/** Create an array of integers to represent the aggregation of several tokens to a token with context.
    @param diffTokens The array of integers representing the words in the input.
    @param range An array of two integers representing the start and length in
        @p diffTokens (or @c NULL for complete array).
    @param context The number of context tokens to use (half on either side).
    @param file The ::file_data struct to fill.
*/

static ValueType *initializeContextDiffTokens(ValueTypeVector *diffTokens, lin *range, unsigned context, struct file_data *file) {
	ValueType *contextDiffTokens, *dataBase;
	size_t dataRange;
	size_t i, idx = 0;
	ValueType edgeArray[context + 1];

	if (range == NULL)
		dataRange = diffTokens->used;
	else
		dataRange = range[1];

	contextDiffTokens = safe_malloc((dataRange + context) * sizeof(ValueType));

	dataBase = diffTokens->data;
	if (range != NULL)
		dataBase += range[0];

	memcpy(edgeArray + 1, dataBase, context * sizeof(ValueType));
	edgeArray[0] = -1;
	for (i = 1; i <= context; i++)
		contextDiffTokens[idx++] = getValue(edgeArray, (i + 1) * sizeof(ValueType));

	for (i = 0; i < dataRange - context; i++)
		contextDiffTokens[idx++] = getValue(dataBase + i, (context + 1) * sizeof(ValueType));

	memcpy(edgeArray, dataBase + dataRange - context, context * sizeof(ValueType));
	edgeArray[context] = -1;
	for (i = 0; i < context; i++)
		contextDiffTokens[idx++] = getValue(edgeArray + i, (context - i + 1) * sizeof(ValueType));

	ASSERT(idx == dataRange + context);

	file->equivs = contextDiffTokens;
	file->buffered_lines = dataRange + context;
	return contextDiffTokens;
}

/** Read the output of the diff command, and call the appropriate print routines.
	@param baseRange The range associated with the diff-token files, or NULL if the whole file.
	@param context The size of the context used.
*/
static void doDiffInternal(lin *baseRange, unsigned context) {
	enum { C_ADD, C_DEL, C_CHANGE } command;
	bool reverseDeleteAdd = false;
	struct change *script = NULL, *ptr;
	struct comparison cmp;

	if (context == 0) {
		if (baseRange == NULL) {
			cmp.file[0].equivs = option.oldFile.diffTokens.data;
			cmp.file[0].buffered_lines = option.oldFile.diffTokens.used;
			cmp.file[1].equivs = option.newFile.diffTokens.data;
			cmp.file[1].buffered_lines = option.newFile.diffTokens.used;
		} else {
			cmp.file[0].equivs = option.oldFile.diffTokens.data + baseRange[0];
			cmp.file[0].buffered_lines = baseRange[1];
			cmp.file[1].equivs = option.newFile.diffTokens.data + baseRange[2];
			cmp.file[1].buffered_lines = baseRange[3];
		}
		cmp.file[0].equiv_max = cmp.file[1].equiv_max = baseHashMax;

		script = diff_2_files(&cmp);
	} else {
		ValueType *oldDiffTokens, *newDiffTokens;

		/* Because we don't actually produce the empty tokens at the start and
		   end of the file or range, we can't produce a set of context tokens
		   when the context is larger than the range we try to cover. Therefore
		   we trim the context to the smallest range. This may move some
		   changes forward a little. */
		if (baseRange != 0) {
			if ((lin) context > baseRange[1])
				context = baseRange[1];
			if ((lin) context > baseRange[3])
				context = baseRange[3];
		} else {
			if (context > option.oldFile.diffTokens.used)
				context = option.oldFile.diffTokens.used;
			if (context > option.newFile.diffTokens.used)
				context = option.newFile.diffTokens.used;
		}
		/* Make sure the context is a multiple of 2. */
		context &= ~1;
		if (context == 0) {
			doDiffInternal(baseRange, context);
			return;
		}

		oldDiffTokens = initializeContextDiffTokens(&option.oldFile.diffTokens, baseRange, context, cmp.file);
		newDiffTokens = initializeContextDiffTokens(&option.newFile.diffTokens,
			baseRange == NULL ? NULL : baseRange + 2, context, cmp.file + 1);
		cmp.file[0].equiv_max = getHashMax();

		script = diff_2_files(&cmp);

		free(oldDiffTokens);
		free(newDiffTokens);
	}

	if (option.needMarkers && baseRange == NULL)
		puts("======================================================================");

	while (script != NULL) {
		if (baseRange != NULL) {
			script->line0 += baseRange[0];
			script->line1 += baseRange[2];
		}

		command = script->inserted == 0 ? C_DEL : (script->deleted == 0 ? C_ADD : C_CHANGE);
		differences = 1;

		if (option.matchContext && context != 0) {
			/* If the match-context option was specified, the diff output will
			   generally be too long. Here we trim the ranges to the minimum
			   range required to show the diff. However, the ranges are only
			   too long when the difference is a change, not when it is an add
			   or delete. */
			if (command == C_CHANGE) {
				if (script->deleted <= (lin) context || script->inserted <= (lin) context) {
					ASSERT(script->deleted != script->inserted);
					if (script->deleted < script->inserted) {
						script->inserted -= script->deleted;
						command = C_ADD;
					} else if (script->inserted < script->deleted) {
						script->deleted -= script->inserted;
						command = C_DEL;
					}
				} else if (!option.aggregateChanges) {
					/* Result must be multiple of two, so divide by 4 first, and
					   then multiply by 2. */
					unsigned newContext = (context / 4) * 2;
					lin range[4] = { script->line0, script->deleted - context, script->line1, script->inserted - context };

					doDiffInternal(range, newContext);
					goto nextDiff;
				} else {
					script->deleted -= context;
					script->inserted -= context;
				}
			}
		}

		/* Print common words. */
		printToCommonWord(script->line1);

		/* Load whitespace for both files and analyse (same func). If old ws does not
		   contain a newline, don't bother with loading the new because we need
		   regular printing. Otherwise, determine if we need reverse printing or
		   need to change the new ws into a single space */
		if (command == C_CHANGE && !option.wdiffOutput) {
			if (loadNextWhitespace(&option.oldFile)) {
				if (loadNextWhitespace(&option.newFile)) {
					/* Because of the line feeds in both pieces of whitespace, we need to
					   print the pieces synchronized to keep the line counters correct. We
					   then empty the old whitespace buffer because that has already been
					   printed, and we set the new whitespace buffer to a space such that
					   the old and the new word are at least separated by a single space. */
					handleSynchronizedNextWhitespace(!lastWasDelete);
					option.newFile.whitespaceBuffer.data[0] = ' ';
					option.newFile.whitespaceBuffer.used = 1;
					option.oldFile.whitespaceBuffer.used = 0;
				} else {
					reverseDeleteAdd = true;
				}
			}
		}

		if (reverseDeleteAdd) {
			/* This only happens for change commands, so we don't have to check
			   the command anymore. */
			printAddedWords(script);
			printDeletedWords(script);
			reverseDeleteAdd = false;
		} else {
			if (command != C_ADD)
				printDeletedWords(script);
			if (command != C_DEL)
				printAddedWords(script);
		}
		if (option.needMarkers) {
			puts("\n======================================================================");
			lastWasLinefeed = true;
		}

		if (command == C_DEL && option.printDeleted && !option.wdiffOutput)
			lastWasDelete = true;

		/* Update statistics */
		switch (command) {
			case C_ADD:
				statistics.added += script->inserted;
				break;
			case C_DEL:
				statistics.deleted += script->deleted;
				break;
			case C_CHANGE:
				statistics.newChanged += script->inserted;
				statistics.oldChanged += script->deleted;
				break;
			default:
				PANIC();
		}
nextDiff:
		ptr = script;
		script = script->link;
		free(ptr);
	}
}

/** Do the difference action. */
void doDiff(void) {
	VECTOR_INIT_ALLOCATED(option.oldFile.whitespaceBuffer);
	VECTOR_INIT_ALLOCATED(option.newFile.whitespaceBuffer);
	option.oldFile.lastPrinted = 0;
	option.newFile.lastPrinted = 0;
	doDiffInternal(NULL, option.matchContext);
	printEnd();
}
