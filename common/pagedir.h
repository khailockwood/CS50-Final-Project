/*
 * pagedir.h - header file for CS50 'pagedir' module
 *
 * The 'pagedir' module handles the initialization of a pageDirectory
 * (marking it as the product of a Crawler) and of saving fetched webpages
 * into that directory, one file per page. It contains all knowledge
 * about the structure and naming of a pageDirectory so that it may be
 * re-used by Crawler, Indexer, and Querier.
 *
 */

#ifndef __PAGEDIR_H
#define __PAGEDIR_H

#include <stdbool.h>
#include "webpage.h"

/**************** pagedir_init ****************/
/*
 * Initialize the given pageDirectory so it may receive crawler output.
 *
 * Caller provides:
 *   the pathname of an existing, writable directory (not NULL).
 * Function does:
 *   construct the pathname "pageDirectory/.crawler" and create that file,
 *   marking the directory as one produced by the Crawler.
 * Function returns:
 *   true  if the marker file was created successfully;
 *   false if pageDirectory is NULL or the file could not be created.
*/

bool pagedir_init(const char* pageDirectory);

/**************** pagedir_save ****************/
/*
 * Save the given webpage into pageDirectory as a file named docID.
 *
 * Caller provides:
 *   a valid webpage (non-NULL),
 *   the pathname of an initialized pageDirectory (non-NULL),
 *   a positive integer docID identifying the file.
 * Function does:
 *   construct the pathname "pageDirectory/docID" and write to it:
 *     the URL on the first line,
 *     the depth on the second line,
 *     the page's HTML contents on the remaining lines.
 * Function guarantees:
 *   if any parameter is NULL, or the file cannot be opened, we print an
 *   error to stderr and exit non-zero (this should not happen if the
 *   caller initialized the directory with pagedir_init).
 */
void pagedir_save(const webpage_t* page, const char* pageDirectory, const int docID);

/**************** pagedir_validate ****************/
/*
 * Verify that the given directory was produced by the Crawler.
 *
 * Caller provides:
 *   the pathname of a directory (not NULL).
 * Function does:
 *   construct the pathname "pageDirectory/.crawler" and try to read it.
 * Function returns:
 *   true  if the .crawler marker file exists and is readable;
 *   false if pageDirectory is NULL or the marker file is absent.
 */
bool pagedir_validate(const char* pageDirectory);

/**************** pagedir_load ****************/
/*
 * Load a webpage from a file in a Crawler-produced pageDirectory.
 *
 * Caller provides:
 *   the pathname of a valid pageDirectory (not NULL),
 *   a positive integer docID naming the file to load.
 * Function does:
 *   open "pageDirectory/docID" and read the URL, depth, and HTML,
 *   then build a webpage_t from them.
 * Function returns:
 *   a pointer to a new webpage_t (caller must later webpage_delete it);
 *   NULL if pageDirectory is NULL or the file cannot be opened.
 */
webpage_t* pagedir_load(const char* pageDirectory, const int docID);

#endif // __PAGEDIR_H
