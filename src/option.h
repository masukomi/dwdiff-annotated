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

#ifndef OPTION_H
#define OPTION_H

#define DEFAULT_LINENUMBER_WIDTH 4
#define BITMASK_SIZE (UCHAR_MAX+7)/8
struct {
	InputFile oldFile,
		newFile;
	const char *delStart,
		*delStop,
		*addStart,
		*addStop;
	char *delColor,
		*addColor;
	size_t delStartLen,
		delStopLen,
		addStartLen,
		addStopLen,
		delColorLen,
		addColorLen;
	/* Bitmaps for single byte checking. */
	char delimiters[BITMASK_SIZE],
		whitespace[BITMASK_SIZE];
	/* Lists for UTF8 mode. */
#ifdef USE_UNICODE
	CharList delimiterList;
	CharList whitespaceList;
	uint32_t punctuationMask;
	UNormalizationMode decomposition;
#endif
	bool whitespaceSet;
	bool printDeleted,
		printAdded,
		printCommon,
		needMarkers,
		needStartStop;
	bool printer,
		less,
		statistics,
		ignoreCase,
		colorMode;
	int lineNumbers;
	bool context;
	int contextLines;
	unsigned matchContext;
	bool aggregateChanges;
	bool paraDelim;
	const char *paraDelimMarker;
	size_t paraDelimMarkerLength;
	bool wdiffOutput;

	FILE *output;
	bool dwfilterMode;
	bool repeatMarkers;
	bool diffInput;
} option;

void parseCmdLine(int argc, char *argv[]);

#endif
