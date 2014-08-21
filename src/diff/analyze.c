/* Analyze file differences for GNU DIFF.

   Copyright (C) 1988-1989, 1992-1995, 1998, 2001-2002, 2004, 2006-2007,
   2009-2011 Free Software Foundation, Inc.

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

/* This file has been heavily stripped and slightly modified by G.P. Halkes, 2011. */

/* G.P. Halkes: Additions: */
#include "diff.h"
static struct file_data files[2];
bool minimal;
bool speed_large_files;
/* /Additions. */

/* The core of the Diff algorithm.  */
#define ELEMENT lin
#define EQUAL(x,y) ((x) == (y))
#define OFFSET lin
#define EXTRA_CONTEXT_FIELDS /* none */
#define NOTE_DELETE(c, xoff) (files[0].changed[files[0].realindexes[xoff]] = 1)
#define NOTE_INSERT(c, yoff) (files[1].changed[files[1].realindexes[yoff]] = 1)
#define USE_HEURISTIC 1

#define lint
#include "diffseq.h"

/* Discard lines from one file that have no matches in the other file.

   A line which is discarded will not be considered by the actual
   comparison algorithm; it will be as if that line were not in the file.
   The file's `realindexes' table maps virtual line numbers
   (which don't count the discarded lines) into real line numbers;
   this is how the actual comparison algorithm produces results
   that are comprehensible when the discarded lines are counted.

   When we discard a line, we also mark it as a deletion or insertion
   so that it will be printed in the output.  */

static void
discard_confusing_lines (struct file_data filevec[])
{
  int f;
  lin i;
  char *discarded[2];
  lin *equiv_count[2];
  lin *p;

  /* Allocate our results.  */
  p = xmalloc ((filevec[0].buffered_lines + filevec[1].buffered_lines)
	       * (2 * sizeof *p));
  for (f = 0; f < 2; f++)
    {
      filevec[f].undiscarded = p;  p += filevec[f].buffered_lines;
      filevec[f].realindexes = p;  p += filevec[f].buffered_lines;
    }

  /* Set up equiv_count[F][I] as the number of lines in file F
     that fall in equivalence class I.  */

  p = zalloc (filevec[0].equiv_max * (2 * sizeof *p));
  equiv_count[0] = p;
  equiv_count[1] = p + filevec[0].equiv_max;

  for (i = 0; i < filevec[0].buffered_lines; ++i)
    ++equiv_count[0][filevec[0].equivs[i]];
  for (i = 0; i < filevec[1].buffered_lines; ++i)
    ++equiv_count[1][filevec[1].equivs[i]];

  /* Set up tables of which lines are going to be discarded.  */

  discarded[0] = zalloc (filevec[0].buffered_lines
			 + filevec[1].buffered_lines);
  discarded[1] = discarded[0] + filevec[0].buffered_lines;

  /* Mark to be discarded each line that matches no line of the other file.
     If a line matches many lines, mark it as provisionally discardable.  */

  for (f = 0; f < 2; f++)
    {
      size_t end = filevec[f].buffered_lines;
      char *discards = discarded[f];
      lin *counts = equiv_count[1 - f];
      const lin *equivs = filevec[f].equivs;
      size_t many = 5;
      size_t tem = end / 64;

      /* Multiply MANY by approximate square root of number of lines.
	 That is the threshold for provisionally discardable lines.  */
      while ((tem = tem >> 2) > 0)
	many *= 2;

      for (i = 0; i < (lin) end; i++)
	{
	  size_t nmatch;
	  if (equivs[i] == 0)
	    continue;
	  nmatch = counts[equivs[i]];
	  if (nmatch == 0)
	    discards[i] = 1;
	  else if (nmatch > many)
	    discards[i] = 2;
	}
    }

  /* Don't really discard the provisional lines except when they occur
     in a run of discardables, with nonprovisionals at the beginning
     and end.  */

  for (f = 0; f < 2; f++)
    {
      lin end = filevec[f].buffered_lines;
      register char *discards = discarded[f];

      for (i = 0; i < end; i++)
	{
	  /* Cancel provisional discards not in middle of run of discards.  */
	  if (discards[i] == 2)
	    discards[i] = 0;
	  else if (discards[i] != 0)
	    {
	      /* We have found a nonprovisional discard.  */
	      register lin j;
	      lin length;
	      lin provisional = 0;

	      /* Find end of this run of discardable lines.
		 Count how many are provisionally discardable.  */
	      for (j = i; j < end; j++)
		{
		  if (discards[j] == 0)
		    break;
		  if (discards[j] == 2)
		    ++provisional;
		}

	      /* Cancel provisional discards at end, and shrink the run.  */
	      while (j > i && discards[j - 1] == 2)
		discards[--j] = 0, --provisional;

	      /* Now we have the length of a run of discardable lines
		 whose first and last are not provisional.  */
	      length = j - i;

	      /* If 1/4 of the lines in the run are provisional,
		 cancel discarding of all provisional lines in the run.  */
	      if (provisional * 4 > length)
		{
		  while (j > i)
		    if (discards[--j] == 2)
		      discards[j] = 0;
		}
	      else
		{
		  register lin consec;
		  lin minimum = 1;
		  lin tem = length >> 2;

		  /* MINIMUM is approximate square root of LENGTH/4.
		     A subrun of two or more provisionals can stand
		     when LENGTH is at least 16.
		     A subrun of 4 or more can stand when LENGTH >= 64.  */
		  while (0 < (tem >>= 2))
		    minimum <<= 1;
		  minimum++;

		  /* Cancel any subrun of MINIMUM or more provisionals
		     within the larger run.  */
		  for (j = 0, consec = 0; j < length; j++)
		    if (discards[i + j] != 2)
		      consec = 0;
		    else if (minimum == ++consec)
		      /* Back up to start of subrun, to cancel it all.  */
		      j -= consec;
		    else if (minimum < consec)
		      discards[i + j] = 0;

		  /* Scan from beginning of run
		     until we find 3 or more nonprovisionals in a row
		     or until the first nonprovisional at least 8 lines in.
		     Until that point, cancel any provisionals.  */
		  for (j = 0, consec = 0; j < length; j++)
		    {
		      if (j >= 8 && discards[i + j] == 1)
			break;
		      if (discards[i + j] == 2)
			consec = 0, discards[i + j] = 0;
		      else if (discards[i + j] == 0)
			consec = 0;
		      else
			consec++;
		      if (consec == 3)
			break;
		    }

		  /* I advances to the last line of the run.  */
		  i += length - 1;

		  /* Same thing, from end.  */
		  for (j = 0, consec = 0; j < length; j++)
		    {
		      if (j >= 8 && discards[i - j] == 1)
			break;
		      if (discards[i - j] == 2)
			consec = 0, discards[i - j] = 0;
		      else if (discards[i - j] == 0)
			consec = 0;
		      else
			consec++;
		      if (consec == 3)
			break;
		    }
		}
	    }
	}
    }

  /* Actually discard the lines. */
  for (f = 0; f < 2; f++)
    {
      char *discards = discarded[f];
      lin end = filevec[f].buffered_lines;
      lin j = 0;
      for (i = 0; i < end; ++i)
	if (minimal || discards[i] == 0)
	  {
	    filevec[f].undiscarded[j] = filevec[f].equivs[i];
	    filevec[f].realindexes[j++] = i;
	  }
	else
	  filevec[f].changed[i] = 1;
      filevec[f].nondiscarded_lines = j;
    }

  free (discarded[0]);
  free (equiv_count[0]);
}

/* Adjust inserts/deletes of identical lines to join changes
   as much as possible.

   We do something when a run of changed lines include a
   line at one end and have an excluded, identical line at the other.
   We are free to choose which identical line is included.
   `compareseq' usually chooses the one at the beginning,
   but usually it is cleaner to consider the following identical line
   to be the "change".  */

static void
shift_boundaries (struct file_data filevec[])
{
  int f;

  for (f = 0; f < 2; f++)
    {
      char *changed = filevec[f].changed;
      char *other_changed = filevec[1 - f].changed;
      lin const *equivs = filevec[f].equivs;
      lin i = 0;
      lin j = 0;
      lin i_end = filevec[f].buffered_lines;

      while (1)
	{
	  lin runlength, start, corresponding;

	  /* Scan forwards to find beginning of another run of changes.
	     Also keep track of the corresponding point in the other file.  */

	  while (i < i_end && !changed[i])
	    {
	      while (other_changed[j++])
		continue;
	      i++;
	    }

	  if (i == i_end)
	    break;

	  start = i;

	  /* Find the end of this run of changes.  */

	  while (changed[++i])
	    continue;
	  while (other_changed[j])
	    j++;

	  do
	    {
	      /* Record the length of this run of changes, so that
		 we can later determine whether the run has grown.  */
	      runlength = i - start;

	      /* Move the changed region back, so long as the
		 previous unchanged line matches the last changed one.
		 This merges with previous changed regions.  */

	      while (start && equivs[start - 1] == equivs[i - 1])
		{
		  changed[--start] = 1;
		  changed[--i] = 0;
		  while (changed[start - 1])
		    start--;
		  while (other_changed[--j])
		    continue;
		}

	      /* Set CORRESPONDING to the end of the changed run, at the last
		 point where it corresponds to a changed run in the other file.
		 CORRESPONDING == I_END means no such point has been found.  */
	      corresponding = other_changed[j - 1] ? i : i_end;

	      /* Move the changed region forward, so long as the
		 first changed line matches the following unchanged one.
		 This merges with following changed regions.
		 Do this second, so that if there are no merges,
		 the changed region is moved forward as far as possible.  */

	      while (i != i_end && equivs[start] == equivs[i])
		{
		  changed[start++] = 0;
		  changed[i++] = 1;
		  while (changed[i])
		    i++;
		  while (other_changed[++j])
		    corresponding = i;
		}
	    }
	  while (runlength != i - start);

	  /* If possible, move the fully-merged run of changes
	     back to a corresponding run in the other file.  */

	  while (corresponding < i)
	    {
	      changed[--start] = 1;
	      changed[--i] = 0;
	      while (other_changed[--j])
		continue;
	    }
	}
    }
}

/* Cons an additional entry onto the front of an edit script OLD.
   LINE0 and LINE1 are the first affected lines in the two files (origin 0).
   DELETED is the number of lines deleted here from file 0.
   INSERTED is the number of lines inserted here in file 1.

   If DELETED is 0 then LINE0 is the number of the line before
   which the insertion was done; vice versa for INSERTED and LINE1.  */

static struct change *
add_change (lin line0, lin line1, lin deleted, lin inserted,
	    struct change *old)
{
  struct change *new = xmalloc (sizeof *new);

  new->line0 = line0;
  new->line1 = line1;
  new->inserted = inserted;
  new->deleted = deleted;
  new->link = old;
  return new;
}

/* Scan the tables of which lines are inserted and deleted,
   producing an edit script in forward order.  */

static struct change *
build_script (struct file_data const filevec[])
{
  struct change *script = 0;
  char *changed0 = filevec[0].changed;
  char *changed1 = filevec[1].changed;
  lin i0 = filevec[0].buffered_lines, i1 = filevec[1].buffered_lines;

  /* Note that changedN[-1] does exist, and is 0.  */

  while (i0 >= 0 || i1 >= 0)
    {
      if (changed0[i0 - 1] | changed1[i1 - 1])
	{
	  lin line0 = i0, line1 = i1;

	  /* Find # lines changed here in each file.  */
	  while (changed0[i0 - 1]) --i0;
	  while (changed1[i1 - 1]) --i1;

	  /* Record this change.  */
	  script = add_change (i0, i1, line0 - i0, line1 - i1, script);
	}

      /* We have reached lines in the two files that match each other.  */
      i0--, i1--;
    }

  return script;
}

/* Report the differences of two files.  */
struct change *diff_2_files (struct comparison *cmp)
{
  struct change *script;

  struct context ctxt;
  lin diags;
  lin too_expensive;

  /* Allocate vectors for the results of comparison:
     a flag for each line of each file, saying whether that line
     is an insertion or deletion.
     Allocate an extra element, always 0, at each end of each vector.  */

  size_t s = cmp->file[0].buffered_lines + cmp->file[1].buffered_lines + 4;
  char *flag_space = zalloc (s);
  cmp->file[0].changed = flag_space + 1;
  cmp->file[1].changed = flag_space + cmp->file[0].buffered_lines + 3;

  /* Some lines are obviously insertions or deletions
     because they don't match anything.  Detect them now, and
     avoid even thinking about them in the main comparison algorithm.  */

  discard_confusing_lines (cmp->file);

  /* Now do the main comparison algorithm, considering just the
     undiscarded lines.  */

  ctxt.xvec = cmp->file[0].undiscarded;
  ctxt.yvec = cmp->file[1].undiscarded;
  diags = (cmp->file[0].nondiscarded_lines
	   + cmp->file[1].nondiscarded_lines + 3);
  ctxt.fdiag = xmalloc (diags * (2 * sizeof *ctxt.fdiag));
  ctxt.bdiag = ctxt.fdiag + diags;
  ctxt.fdiag += cmp->file[1].nondiscarded_lines + 1;
  ctxt.bdiag += cmp->file[1].nondiscarded_lines + 1;

  ctxt.heuristic = speed_large_files;

  /* Set TOO_EXPENSIVE to be approximate square root of input size,
     bounded below by 256.  */
  too_expensive = 1;
  for (;  diags != 0;  diags >>= 2)
    too_expensive <<= 1;
  ctxt.too_expensive = MAX (256, too_expensive);

  files[0] = cmp->file[0];
  files[1] = cmp->file[1];

  compareseq (0, cmp->file[0].nondiscarded_lines,
	      0, cmp->file[1].nondiscarded_lines, minimal, &ctxt);

  free (ctxt.fdiag - (cmp->file[1].nondiscarded_lines + 1));

  /* Modify the results slightly to make them prettier
     in cases where that can validly be done.  */

  shift_boundaries (cmp->file);

  /* Get the results of comparison in the form of a chain
     of `struct change's -- an edit script.  */

  script = build_script (cmp->file);

  free (cmp->file[0].undiscarded);
  free (flag_space);

  return script;
}
