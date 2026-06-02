/*
 * indextest.c - unit test for the 'index' module of the TSE
 *
 * Loads an index from a file produced by the indexer, then writes it back
 * out to a new file. Running the output through indexcmp against the input
 * verifies that index_load and index_save round-trip correctly.
 *
 * usage: ./indextest oldIndexFilename newIndexFilename
 *
 * Lyndon Huang, Khai Lockwood, Gift Christian
 * 6/1/26
 */

#include <stdio.h>
#include <stdlib.h>
#include "index.h"

int main(const int argc, char* argv[])
{
  // validate that exactly two arguments were provided
  if (argc != 3) {
    fprintf(stderr, "usage: ./indextest oldIndexFilename newIndexFilename\n");
    return 1;
  }

  // load the index from the old index file into memory
  index_t* index = index_load(argv[1]);
  if (index == NULL) {
    fprintf(stderr, "error: cannot load %s\n", argv[1]);
    return 1;
  }

  // write the loaded index back out to the new index file
  index_save(index, argv[2]);

  // free the index and exit successfully
  index_delete(index);
  return 0;

}
