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
#include "file.h"
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

/**************** pagedir_validate ****************/
/* see pagedir.h for description */
bool
pagedir_validate(const char* pageDirectory)
{
  // defensive check
  if (pageDirectory == NULL) {
    return false;
  }

  // construct the pathname for the .crawler file in that directory
  char* pathname = pagedir_pathname(pageDirectory, ".crawler");

  // open the file for reading. If error, clean up and return false
  FILE* fp = fopen(pathname, "r");
  free(pathname);
  if (fp == NULL) {
    return false;
  }

  // close file
  fclose(fp);
  return true;
}

/**************** pagedir_load ****************/
/* see pagedir.h for description */
webpage_t*
pagedir_load(const char* pageDirectory, const int docID)
{
  // defensive check
  if (pageDirectory == NULL) {
    return NULL;
  }

  // construct the pathname "pageDirectory/docID"
  char idstr[12]; // room for an int and '\0'
  snprintf(idstr, sizeof(idstr), "%d", docID);
  char* pathname = pagedir_pathname(pageDirectory, idstr);

  // open file for reading
  FILE* fp = fopen(pathname, "r");
  free(pathname);
  if (fp == NULL) {
    return NULL;
  }

  // read the URL (line 1), depth (line 2), and HTML (rest of file)
  char* url = file_readLine(fp);
  char* depthStr = file_readLine(fp);
  char* html = file_readFile(fp);
  fclose(fp);

  // a valid page file must have all three parts
  if (url == NULL || depthStr == NULL || html == NULL) {
    free(url);
    free(depthStr);
    free(html);
    return NULL;
  }

  // convert the depth string to an integer
  int depth = 0;
  sscanf(depthStr, "%d", &depth);
  free(depthStr);

  // build a webpage_t; it takes ownership of url and html
  return webpage_new(url, depth, html);
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
