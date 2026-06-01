/*
 * index.c
 *
 * See index.h for interface descriptions and usage.
 *
 * An index is a wrapper around a hashtable mapping word to counters,
 * where each counters object maps docID to count.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "index.h"
#include "hashtable.h"
#include "counters.h"
#include "file.h"
#include "mem.h"

/**************** global types ****************/
typedef struct index {
  hashtable_t* ht; // hashtable mapping word to counters_t
} index_t;

/**************** file-local functions ****************/
static void index_saveWord(void* arg, const char* key, void* item);
static void index_saveCount(void* arg, const int key, const int count);
static void index_deleteCounters(void* item);

/**************** index_new ****************/
/* see index.h for description */
index_t*
index_new(const int numSlots)
{
  // defensive check
  if (numSlots <= 0) {
    return NULL;
  }

  index_t* index = mem_malloc(sizeof(index_t));
  if (index == NULL) {
    return NULL;
  }

  index->ht = hashtable_new(numSlots);
  if (index->ht == NULL) {
    free(index);
    return NULL;
  }
  return index;
}

/**************** index_delete ****************/
/* see index.h for description */
void
index_delete(index_t* index)
{
  if (index != NULL) {
    // delete first each counters object, then the hashtable, then the struct
    hashtable_delete(index->ht, index_deleteCounters);
    free(index);
  }
}

/**************** index_find ****************/
/* see index.h for description */
counters_t*
index_find(index_t* index, const char* word)
{
  if (index == NULL || word == NULL) {
    return NULL;
  }
  return hashtable_find(index->ht, word);
}

/**************** index_insert ****************/
/* see index.h for description */
bool
index_insert(index_t* index, const char* word, counters_t* counters)
{
  if (index == NULL || word == NULL || counters == NULL) {
    return false;
  }
  return hashtable_insert(index->ht, word, counters);
}

/**************** index_save ****************/
/* see index.h for description */
bool
index_save(const index_t* index, const char* indexFilename)
{
  // defensive check
  if (index == NULL || indexFilename == NULL) {
    return false;
  }

  // open the index file for writing
  FILE* fp = fopen(indexFilename, "w");
  if (fp == NULL) {
    return false;
  }

  // for each (word, counters) pair, write a line to the file
  hashtable_iterate(index->ht, fp, index_saveWord);

  fclose(fp);
  return true;
}

/**************** index_load ****************/
/* see index.h for description */
index_t*
index_load(const char* indexFilename)
{
  // defensive check
  if (indexFilename == NULL) {
    return NULL;
  }

  // open the index file for reading
  FILE* fp = fopen(indexFilename, "r");
  if (fp == NULL) {
    return NULL;
  }

  // size the index by the number of words (lines) in the file
  int numLines = file_numLines(fp);
  index_t* index = index_new(numLines > 0 ? numLines : 1);
  if (index == NULL) {
    fclose(fp);
    return NULL;
  }

  // read each line, each line is a word followed by "docID count" pairs
  char* word;
  while ((word = file_readWord(fp)) != NULL) {
    counters_t* counters = counters_new();
    index_insert(index, word, counters);

    int docID, count;
    while (fscanf(fp, "%d %d", &docID, &count) == 2) {
      counters_set(counters, docID, count);
    }
    free(word);
  }

  fclose(fp);
  return index;
}

/**************** index_saveWord ****************/
/*
 * Helper passed to hashtable_iterate, we write one word and its counters
 * to the file, in the form "word docID count [docID count]...\n".
 * arg is the FILE*; key is the word; item is the counters_t.
 */
static void
index_saveWord(void* arg, const char* key, void* item)
{
  FILE* fp = arg;
  counters_t* counters = item;
  if (fp != NULL && key != NULL && counters != NULL) {
    fprintf(fp, "%s", key);
    counters_iterate(counters, fp, index_saveCount);
    fprintf(fp, "\n");
  }
}

/**************** index_saveCount ****************/
/*
 * Helper passed to counters_iterate: write " docID count" for one pair.
 * arg is the FILE*; key is the docID; count is the count.
 */
static void
index_saveCount(void* arg, const int key, const int count)
{
  FILE* fp = arg;
  if (fp != NULL) {
    fprintf(fp, " %d %d", key, count);
  }
}

/**************** index_deleteCounters ****************/
/*
 * Helper passed to hashtable_delete, deletes one counters object.
 */
static void
index_deleteCounters(void* item)
{
  counters_t* counters = item;
  if (counters != NULL) {
    counters_delete(counters);
  }
}
