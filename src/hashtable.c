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

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "definitions.h"
#include "hashtable.h"

#define MEMBLOCK_SIZE 32768
#define ALIGNOF(type) offsetof(struct { char c; type member; }, member)
#define ROUNDUP(x, to) (((x + (to - 1)) / to) * to)

typedef struct MemBlock {
	struct MemBlock *next;
	size_t size;
	size_t idx;
} MemBlock;

static MemBlock *head;

#define HASHTABLE_SIZE 8091

typedef struct Tuple {
	ValueType value;
	size_t stringLength;
	struct Tuple *next;
	char string[1];
} Tuple;

static Tuple *hashtable[HASHTABLE_SIZE];
static ValueType nextValue;

ValueType baseHashMax;

static Tuple *allocFromBlock(size_t size) {
	Tuple *result;

	size = ROUNDUP(size, ALIGNOF(Tuple));
	if (head == NULL || size > head->size - head->idx) {
		MemBlock *newBlock;
		if (size > 2048) {
			newBlock = safe_malloc(size + sizeof(MemBlock));
			newBlock->size = size + sizeof(MemBlock);
		} else {
			newBlock = safe_malloc(MEMBLOCK_SIZE);
			newBlock->size = MEMBLOCK_SIZE;
		}
		newBlock->idx = ROUNDUP(sizeof(MemBlock), ALIGNOF(Tuple));
		newBlock->next = head;
		head = newBlock;
	}
	result = (Tuple *)(((char *) head) + head->idx);
	head->idx += size;
	return result;
}

static void freeBlocks(void) {
	MemBlock *ptr;
	while (head != NULL) {
		ptr = head;
		head = head->next;
		free(ptr);
	}
}

#ifdef PROFILE_HASH
static int collisions;
static int hits;

void printHashStatistics(void) {
	fprintf(stderr, "Hash statistics: unique words: %d, collisions %d (%.2f%%), hits %d\n",
		(int) nextValue, collisions, (double) collisions * 100.0 / nextValue, hits);
	collisions = 0;
	hits = 0;
}
#endif

/** Calculate a hash value for a string.
	@param key The string to hash.
	@return The hash value associated with the string.

	This function uses the djb2 hash function.
*/
static unsigned int hash(void *data, size_t size) {
	const unsigned char *ptr;
	size_t i;
	unsigned int hashValue = 5381;

	ptr = data;

	for (i = 0; i < size; i++)
		hashValue = (hashValue << 5) + hashValue + (unsigned int) *ptr++;

	return hashValue;
}

/** Get the value associated with a word.
	@param word The word to get the value for.

	This function gets the value associated with a word. If the word has not been
	seen before a new value will be given.
*/
ValueType getValueFromContext(CharBuffer *word) {
	return getValue(word->data, word->used);
}

ValueType getValue(void *data, size_t size) {
	Tuple *tuple;
	unsigned int hashValue = hash(data, size) % HASHTABLE_SIZE;

	tuple = hashtable[hashValue];
	/* Search the singly linked list. */
	while (tuple != NULL && !(size == tuple->stringLength && memcmp(data, tuple->string, size) == 0))
		tuple = tuple->next;

	if (tuple == NULL) {
		ASSERT(nextValue != VALUE_MAX);
		#ifdef PROFILE_HASH
		if (hashtable[hashValue] != NULL)
			collisions++;
		#endif
		tuple = allocFromBlock(sizeof(Tuple) - 1 + size);

		tuple->value = nextValue++;
		tuple->stringLength = size;
		memcpy(tuple->string, data, size);
		tuple->next = hashtable[hashValue];
		hashtable[hashValue] = tuple;
	}
	#ifdef PROFILE_HASH
	else
		hits++;
	#endif
	return tuple->value;
}

ValueType getHashMax(void) {
	ValueType hashMax = nextValue;
	int i;

	#ifdef PROFILE_HASH
	printHashStatistics();
	#endif

	/* Reset the hashtable for the next iteration. */
	for (i = 0; i < HASHTABLE_SIZE; i++)
		hashtable[i] = NULL;
	freeBlocks();

	nextValue = 0;

	return hashMax;
}
