/*
 * pagedir.c - CS50 'pagedir' module
 *
 * See pagedir.h for interface descriptions and usage.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pagedir.h"
#include "webpage.h"
#include "mem.h"

/**************** file-local functions ****************/
/* see helper comments below */
static char* pagedir_pathname(const char* pageDirectory, const char* file);

/**************** pagedir_init ****************/
/* see pagedir.h for description */
bool
pagedir_init(const char* pageDirectory)
{
  // defensive check
  if (pageDirectory == NULL) {
    return false;
  }

  // construct the pathname for the .crawler file in that directory
  char* pathname = pagedir_pathname(pageDirectory, ".crawler");

  // open the file for writing. If error, clean up and return false
  FILE* fp = fopen(pathname, "w");
  free(pathname);
  if (fp == NULL) {
    return false;
  }

  // close file
  fclose(fp);
  return true;
}

/**************** pagedir_save ****************/
/* see pagedir.h for description */
void
pagedir_save(const webpage_t* page, const char* pageDirectory, const int docID)
{
  // defensive check, all pointers must be valid
  mem_assert_const(page, "pagedir_save: page is NULL");
  mem_assert_const(pageDirectory, "pagedir_save: pageDirectory is NULL");

  // construct the pathname "pageDirectory/docID"
  char idstr[12]; // room for an int and '\0'
  snprintf(idstr, sizeof(idstr), "%d", docID); //snprintf prevents buffer overflow
  char* pathname = pagedir_pathname(pageDirectory, idstr);

  // open file for writing
  FILE* fp = fopen(pathname, "w");
  if (fp == NULL) {
    fprintf(stderr, "pagedir_save: cannot write to file '%s'\n", pathname);
    free(pathname);
    exit(1);
  }
  free(pathname);

  // print  URL, depth, and contents of the webpage
  fprintf(fp, "%s\n", webpage_getURL((webpage_t*) page));
  fprintf(fp, "%d\n", webpage_getDepth((webpage_t*) page));
  fprintf(fp, "%s", webpage_getHTML((webpage_t*) page));

  // close the file
  fclose(fp);
}

/**************** pagedir_pathname ****************/
/*
 * Build and return a newly-allocated string "pageDirectory/file".
 * Caller must free the returned string.
 * Exits non-zero (via mem_malloc_assert) if out of memory.
 */
static char*
pagedir_pathname(const char* pageDirectory, const char* file)
{
  // length = directory + '/' + file + '\0'
  size_t len = strlen(pageDirectory) + 1 + strlen(file) + 1;
  char* pathname = mem_malloc_assert(len, "pagedir_pathname: out of memory");
  snprintf(pathname, len, "%s/%s", pageDirectory, file);
  return pathname;
}
