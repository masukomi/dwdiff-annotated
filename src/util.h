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

#ifndef UTIL_H
#define UTIL_H

#ifdef __GNUC__
void fatal(const char *fmt, ...) __attribute__((format (printf, 1, 2))) __attribute__((noreturn));
#else
/*@noreturn@*/ void fatal(const char *fmt, ...);
#endif
int ASCIItolower(int c);

#ifdef DEBUG
#define PANIC() do { fprintf(stderr, _("Program-logic error at %s:%d\n"), __FILE__, __LINE__); abort(); } while (0)
#else
#define PANIC() fatal(_("Program-logic error at %s:%d\n"), __FILE__, __LINE__)
#endif
#define ASSERT(_condition) do { if (!(_condition)) PANIC(); } while(0)

char *safe_strdup(const char *orig);
void *safe_malloc(size_t size);
void *safe_calloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
#endif
