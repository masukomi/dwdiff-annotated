dwdiff - Annotated

This repository contains the source code to dwdiff, which 
I am gradually going through and documenting the key pieces of 
in an attempt to understand how it functions.

Please see the DEVELOPER_OVERVIEW.md file for more geeky
details on what's here and how it works.

Original README below.

--------------------------------------------------------------

Introduction
============

dwdiff is a diff program that operates at the word level instead of the line
level. It is different from wdiff in that it allows the user to specify what
should be considered whitespace, and in that it takes an optional list of
characters that should be considered delimiters. Delimiters are single
characters that are treated as if they are words, even when there is no
whitespace separating them from preceding words or delimiters. dwdiff is mostly
commandline compatible with wdiff. Only the --autopager, --terminal and
--avoid-wraps options are not supported.

The default output from dwdiff is the new text, with the deleted and inserted
parts annotated with markers. Command line options are available to change
both what is printed, and the markers.

dwdiff is licensed under the GNU General Public License version 3. See the file
COPYING for details.

Motivation
==========

I wrote dwdiff because when diff'ing C code with wdiff, wdiff would often find
differences that were larger than I wanted. For example, when one modifies the
function header of a program:

	void someFunction(SomeType var)

into

	void someFunction(SomeOtherType var)

wdiff would say that `someFunction(SomeType` was replaced by
`someFunction(SomeOtherType`, while what I wanted it to say was that `SomeType`
was replaced by "SomeOtherType".

Prerequisites and installation
==============================

dwdiff relies on the GNU gettext library for providing localised messages, and
on [libicu](http://icu-project.org) for Unicode support. dwdiff can be compiled
without support for either of these libraries, which means all messages will be
in English and Unicode will not be supported.

There are two ways in which to compile dwdiff:

Using the configure script:
---

	$ ./configure
	#or
	$ ./configure --prefix=/usr
	#(see ./configure --help for more tuning options)
	$ make all
	$ make install
	#(assumes working install program)

Manually editing the Makefile to suit your computer:
---

	$ cp Makefile.in Makefile

Edit the values for GETTEXTFLAGS, GETTEXTLIBS, LOCALEDIR, LINGUAS, ICUFLAGS,
ICULIBS and prefix in Makefile

	$ make all
	$ make install
	#(assumes working install program)

The Makefile.in in the distribution should work on all POSIX compatible make's.
I have tested it with both GNU make and BSD make.

dwdiff uses several POSIX functions, namely: mkstemp, open, read, lseek, write,
close, umask, snprintf. dwdiff should compile on any Un*x system that provides
these functions.

Reporting bugs
==============

If you think you have found a bug, please check that you are using the latest
version of dwdiff [http://os.ghalkes.nl/dwdiff.html]. When reporting bugs,
please include a minimal example that demonstrates the problem.

Author
======

Gertjan Halkes <dwdiff@ghalkes.nl>

Language files were contributed by the authors named in them.
