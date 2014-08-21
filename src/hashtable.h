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

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <limits.h>
#include "definitions.h"
#include "buffer.h"

ValueType getValueFromContext(CharBuffer *word);
ValueType getValue(void *data, size_t size);
ValueType getHashMax(void);

extern ValueType baseHashMax;
#endif
