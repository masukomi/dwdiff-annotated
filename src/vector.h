/* Copyright (C) 2011 G.P. Halkes
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
#ifndef VECTOR_H
#define VECTOR_H

#define VECTOR_INITIAL_SIZE 32

#define VECTOR(type, name) struct { type *data; size_t allocated, used; } name
#define VECTOR_INIT(name) do { (name).data = NULL; (name).allocated = 0; (name).used = 0; } while (0)
#define VECTOR_INIT_ALLOCATED(name) do { \
	(name).allocated = VECTOR_INITIAL_SIZE;  \
	(name).data = safe_malloc((name).allocated * sizeof((name).data[0])); \
	(name).used = 0; \
} while (0)
#define VECTOR_APPEND(name, value) do { \
	if ((name).allocated <= (name).used) { \
		(name).allocated = (name).allocated == 0 ? VECTOR_INITIAL_SIZE : (name).allocated * 2; \
		(name).data = safe_realloc((name).data, (name).allocated * sizeof((name).data[0])); \
	} \
	(name).data[(name).used++] = value; \
} while (0)
#define VECTOR_ALLOCATE(name, value) do { \
	if ((name).allocated > (name).used + (value)) break; \
	if ((name).allocated == 0) (name).allocated = VECTOR_INITIAL_SIZE; \
	while ((name).allocated <= (name).used + (value)) (name).allocated *= 2; \
	(name).data = safe_realloc((name).data, (name).allocated * sizeof((name).data[0])); \
} while (0)
#define VECTOR_FREE(name) do { free((name).data); (name).data = NULL; (name).used = 0; (name).allocated = 0; } while (0)

#endif
