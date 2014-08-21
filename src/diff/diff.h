/* Shared definitions for GNU DIFF

   Copyright (C) 1988-1989, 1991-1995, 1998, 2001-2002, 2004, 2009-2011 Free
   Software Foundation, Inc.

   This file is part of GNU DIFF.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 3 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This file has been heavily stripped and slightly modified by G.P. Halkes, 2011.
   Definitions regarding the lin type and the MIN and MAX macros were taken
   from diffutils-3.2/src/system.h, which is distributed under the same
   license as this file.
*/
#ifndef DIFF_H
#define DIFF_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include "static_assert.h"

typedef ptrdiff_t lin;
#define LIN_MAX PTRDIFF_MAX
static_assert((lin) -1 < 0);
static_assert(sizeof (ptrdiff_t) <= sizeof (lin));
static_assert(sizeof (lin) <= sizeof (long int));

#include "definitions.h"
#include "util.h"

#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

#define xmalloc safe_malloc
#define zalloc safe_calloc


/* Use heuristics for better speed with large files with a small
   density of changes.  */
extern bool speed_large_files;

/* Don't discard lines.  This makes things slower (sometimes much
   slower) but will find a guaranteed minimal set of changes.  */
extern bool minimal;

/* The result of comparison is an "edit script": a chain of `struct change'.
   Each `struct change' represents one place where some lines are deleted
   and some are inserted.

   LINE0 and LINE1 are the first affected lines in the two files (origin 0).
   DELETED is the number of lines deleted here from file 0.
   INSERTED is the number of lines inserted here in file 1.

   If DELETED is 0 then LINE0 is the number of the line before
   which the insertion was done; vice versa for INSERTED and LINE1.  */

struct change
{
  struct change *link;		/* Previous or next edit command  */
  lin inserted;			/* # lines of file 1 changed here.  */
  lin deleted;			/* # lines of file 0 changed here.  */
  lin line0;			/* Line number of 1st deleted line.  */
  lin line1;			/* Line number of 1st inserted line.  */
  bool ignore;			/* Flag used in context.c.  */
};

/* Structures that describe the input files.  */

/* Data on one input file being compared.  */

struct file_data {
    /* linbuf_base <= buffered_lines <= valid_lines <= alloc_lines.
       linebuf[linbuf_base ... buffered_lines - 1] are possibly differing.
       linebuf[linbuf_base ... valid_lines - 1] contain valid data.
       linebuf[linbuf_base ... alloc_lines - 1] are allocated.  */
    lin buffered_lines;

    /* Vector, indexed by line number, containing an equivalence code for
       each line.  It is this vector that is actually compared with that
       of another file to generate differences.  */
    const lin *equivs;

    /* Vector, like the previous one except that
       the elements for discarded lines have been squeezed out.  */
    lin *undiscarded;

    /* Vector mapping virtual line numbers (not counting discarded lines)
       to real ones (counting those lines).  Both are origin-0.  */
    lin *realindexes;

    /* Total number of nondiscarded lines.  */
    lin nondiscarded_lines;

    /* Vector, indexed by real origin-0 line number,
       containing 1 for a line that is an insertion or a deletion.
       The results of comparison are stored here.  */
    char *changed;

    /* 1 more than the maximum equivalence value used for this or its
       sibling file.  */
    lin equiv_max;
};

/* Data on two input files being compared.  */

struct comparison
  {
    struct file_data file[2];
    struct comparison const *parent;  /* parent, if a recursive comparison */
  };

struct change *diff_2_files(struct comparison *cmp);

#endif
