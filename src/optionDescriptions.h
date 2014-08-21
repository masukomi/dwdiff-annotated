/* Copyright (C) 2010-2011 G.P. Halkes
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

#ifndef OPTIONDESCRIPTIONS_H
#define OPTIONDESCRIPTIONS_H

const char *descriptions[] = {
/* Options showing info about the program */
N_("-h, --help                             Print this help message\n"),
N_("-v, --version                          Print version and copyright information\n"),

/* Options changing what is considered (non-)whitespace */
N_("-d <delim>, --delimiters=<delim>       Specifiy delimiters\n"),
N_("-P, --punctuation                      Use punctuation characters as delimiters\n"),
N_("-W <ws>, --white-space=<ws>            Specify whitespace characters\n"),
#ifdef DWDIFF_COMPILE
N_("--diff-input                           Read the input as the output from diff\n"),
N_("-S[<marker>], --paragraph-separator[=<marker>]  Show inserted or deleted blocks\n"
   "                               of empty lines, optionally overriding the marker\n"),

/* Options changing what is output */
N_("-1, --no-deleted                       Do not print deleted words\n"),
N_("-2, --no-inserted                      Do not print inserted words\n"),
N_("-3, --no-common                        Do not print common words\n"),
N_("-L[<width>], --line-numbers[<width>]   Prepend line numbers\n"),
N_("-C<num>, --context=<num>               Show <num> lines of context\n"),
N_("-s, --statistics                       Print statistics when done\n"),
#endif
N_("--wdiff-output                         Produce wdiff compatible output\n"),

/* Options changing the matching */
N_("-i, --ignore-case                      Ignore differences in case\n"),
N_("-I, --ignore-formatting                Ignore formatting differences\n"),
/* TRANSLATORS:
   The context meant here are words preceeding and succeeding each word in
   the text. By using these extra context words when applying the diff program,
   frequently occuring words will be more likely to be matched to the
   correct corresponding word in the other text, thus giving a better result. */
N_("-m <num>, --match-context=<num>        Use <num> words of context for matching\n"),
/* TRANSLATORS:
   The use of context words for matching is more expensive, because after the
   first pass of diff the changes reported need refining. However, if the user
   can live with multiple changes that are within (2 * match-context + 1) words
   from eachother being reported as a single change, they can use this option. */
N_("--aggregate-changes                    Allow close changes to aggregate\n"),
N_("-A <alg>, --algorithm=<alg>            Choose algorithm: best, normal, fast\n"),

#ifdef DWDIFF_COMPILE
/* Options changing the appearance of the output */
N_("-c[<spec>], --color[=<spec>]           Color mode\n"),
N_("-l, --less-mode                        As -p but also overstrike whitespace\n"),
N_("-p, --printer                          Use overstriking and bold text\n"),
N_("-w <string>, --start-delete=<string>   String to mark begin of deleted text\n"),
N_("-x <string>, --stop-delete=<string>    String to mark end of deleted text\n"),
N_("-y <string>, --start-insert=<string>   String to mark begin of inserted text\n"),
N_("-z <string>, --stop-insert=<string>    String to mark end of inserted text\n"),
N_("-R, --repeat-markers                   Repeat markers at newlines\n"),
#endif

#ifdef DWFILTER_COMPILE
N_("-r, --reverse                          Format new as old\n"),
#endif

NULL};

#endif
