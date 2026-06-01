/*
 * index.h - header file for 'index' module
 *
 * An 'index' is an abstract data type mapping each word to a set of
 * (docID, count) pairs, recording how many times that word appears in
 * each document. The module is shared by the Indexer and the Querier.
 *
 */

#ifndef __INDEX_H
#define __INDEX_H

#include <stdbool.h>
#include "counters.h"

/**************** types ****************/
typedef struct index index_t; // opaque to users of the module

/**************** index_new ****************/
/*
 * Create a new, empty index.
 *
 * Caller provides:
 *   the number of slots for the underlying hashtable (> 0).
 * Function returns:
 *   a pointer to a new index, or NULL on error (e.g. numSlots <= 0,
 *   or out of memory).
 * Caller is responsible for:
 *   later calling index_delete.
 */
index_t* index_new(const int numSlots);

/**************** index_delete ****************/
/*
 * Delete an index, freeing all words and their counters.
 *
 * Caller provides:
 *   a valid index pointer (may be NULL, in which case nothing happens).
 */
void index_delete(index_t* index);

/**************** index_find ****************/
/*
 * Look up the counters associated with a word.
 *
 * Caller provides:
 *   a valid index and a word (neither NULL).
 * Function returns:
 *   the counters_t for that word, or NULL if the word is absent or any
 *   parameter is NULL.
 * Notes:
 *   the returned counters is owned by the index; do not delete it.
 */
counters_t* index_find(index_t* index, const char* word);

/**************** index_insert ****************/
/*
 * Insert a (word, counters) pair into the index.
 *
 * Caller provides:
 *   a valid index, a word, and a counters object (none NULL).
 * Function returns:
 *   true if the pair was inserted; false on error or if the word was
 *   already present.
 * Notes:
 *   the index takes ownership of the counters and will delete it.
 */
bool index_insert(index_t* index, const char* word, counters_t* counters);

/**************** index_save ****************/
/*
 * Write the index to a file in the standard index file format:
 *   one line per word: "word docID count [docID count]..."
 *
 * Caller provides:
 *   a valid index and the pathname of a writable file (neither NULL).
 * Function returns:
 *   true on success; false if a parameter is NULL or the file cannot be
 *   opened for writing.
 */
bool index_save(const index_t* index, const char* indexFilename);

/**************** index_load ****************/
/*
 * Read an index file (in the format written by index_save) into a new
 * index data structure.
 *
 * Caller provides:
 *   the pathname of a readable index file (not NULL).
 * Function returns:
 *   a pointer to a new index, or NULL if the file cannot be opened.
 * Caller is responsible for:
 *   later calling index_delete.
 */
index_t* index_load(const char* indexFilename);

#endif // __INDEX_H
