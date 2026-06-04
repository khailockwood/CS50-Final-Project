/*
 * Lyndon Huang, Khai Lockwood, Gift Christian
 * 6/1/26
 * CS50
 *
 * indexer.c - builds an inverted index from a crawler pageDirectory
 *             and writes it to a file.
 *
 * Reads the document files produced by the Crawler from a pageDirectory,
 * builds an inverted index mapping each word to the (docID, count) pairs
 * recording how often it appears in each document, and writes that index
 * to a file in the standard index format.
 *
 * usage: ./indexer pageDirectory indexFilename
 *   pageDirectory  - a directory produced by the Crawler
 *   indexFilename  - the writable file into which the index is written
 *
 * Exits zero on success; exits non-zero with a message to stderr on any
 * unrecoverable error: wrong arguments, invalid pageDirectory, unwritable
 * indexFilename, or out of memory.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"
#include "pagedir.h"
#include "word.h"
#include "webpage.h"
#include "counters.h"

static const int numSlots = 900;   // hashtable size (spec suggests 200..900)

/* function prototypes */
static void indexBuild(index_t* index, const char* pageDirectory);
static void indexPage(index_t* index, webpage_t* page, const int docID);

int main(const int argc, char* argv[])
{
  //validate that exactly 2 args provided
  if (argc != 3) {
    fprintf(stderr, "Error, must enter exactly 3 arguments\n");
    return 1;
  }

  char* pageDirectory = argv[1];
  char* indexFilename = argv[2];
  //confirm validation by crawler
  if (!pagedir_validate(pageDirectory)){
    fprintf(stderr, "error: %s is not a crawler directory\n", pageDirectory);
    return 1;
  }
  //confirm open then release 
  FILE* fp = fopen(indexFilename, "w");
  if (fp == NULL) {
    fprintf(stderr, "error: unable to write %s\n", indexFilename);
    return 1;
  }
  
  fclose(fp);
  index_t* index = index_new(numSlots);
  if (index == NULL) {
    fprintf(stderr, "error: out of memory\n");
    return 1;
  }
  //scan every page in directory and add words to index
  indexBuild(index, pageDirectory);
 
  if (!index_save(index, indexFilename)) {
    fprintf(stderr, "error: failed to write\n");
    index_delete(index);
    return 1;
  }
  
  index_delete(index);
  return 0;

}
/************ indexBuild *************/
/* loops over document IDs,
   loading each page and adding
   its words to the index
*/

static void indexBuild(index_t* index, const char* pageDirectory)
{
  if (index == NULL || pageDirectory == NULL) {
    return;
  }

  // loop docID starting at 1, incrementing
  int docID = 1;
  webpage_t* page;
  while ((page = pagedir_load(pageDirectory, docID)) != NULL) {
    indexPage(index, page, docID);
    webpage_delete(page);
    docID += 1;
  }
}

/********* indexPage ************/
/*
  scans every word of the page,
  normalizes it, and records its occurrence
  in the index under the given docID
*/

static void indexPage(index_t* index, webpage_t* page, const int docID)
{

   if (index == NULL || page == NULL) {
     return;
   }
   
   int pos = 0;
   char* word;
   counters_t* ctrs;
   // while next word is not null, loop through and skip trivial words
   while ((word = webpage_getNextWord(page, &pos)) != NULL) {
     if (strlen(word) >= 3) {
       //normalize
       normalizeWord(word);
       //index 
       ctrs = index_find(index, word);
       if (ctrs == NULL) {
	 ctrs = counters_new();
	 index_insert(index, word, ctrs);
       }
       counters_add(ctrs, docID);
     }
     
     free(word);
     
   }
}







