/*
 * Khai Lockwood
 * 5/31/2026
 * CS50 Tiny Search Engine 'crawler'
 *
 * The crawler starts from a given "seed" URL, fetches that page, scans it
 * for links to other pages on the same site (internal URLs), and repeats
 * the process for each new page it discovers, out to a maximum depth.
 * Every page it successfully fetches is saved to the given pageDirectory.
 *
 * Usage: ./crawler seedURL pageDirectory maxDepth
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "bag.h"
#include "hashtable.h"
#include "webpage.h"
#include "mem.h"
#include "pagedir.h"

/**************** constants ****************/
static const int hashSlots = 200;    // slots in the hashtable of seen URLS
static const int maxDepthLimit = 10; // largest maxDepth accepted on command line

/**************** function prototypes ****************/
int main(const int argc, char* argv[]);
static void parseArgs(const int argc, char* argv[],
                      char** seedURL, char** pageDirectory, int* maxDepth);
static void crawl(char* seedURL, char* pageDirectory, const int maxDepth);
static void pageScan(webpage_t* page, bag_t* pagesToCrawl, hashtable_t* pagesSeen);
static void logr(const char* word, const int depth, const char* url);

/**************** main ****************/
/* Parse the command line, then crawl, see file header for usage. */
int
main(const int argc, char* argv[])
{
  char* seedURL = NULL;
  char* pageDirectory = NULL;
  int maxDepth = 0;

  // validate and extract the three command-line arguments
  parseArgs(argc, argv, &seedURL, &pageDirectory, &maxDepth);

  // crawl each and all subsequent webpages
  crawl(seedURL, pageDirectory, maxDepth);

  free(seedURL);
  return 0;
}

/**************** parseArgs ****************/
/*
 * Given the command-line arguments, extract them into the caller's
 * variables, validating each one. On any error, print a message to
 * stderr and exit non-zero. Returns (via parameters) only on success.
 *
 *   seedURL - normalized and confirmed to be an internal URL
 *   pageDirectory - confirmed initializable via pagedir_init()
 *   maxDepth - confirmed to be an integer in [0, maxDepthLimit]
 */
static void
parseArgs(const int argc, char* argv[],
          char** seedURL, char** pageDirectory, int* maxDepth)
{
  // require exactly three arguments
  if (argc != 4) {
    fprintf(stderr, "usage: %s seedURL pageDirectory maxDepth\n", argv[0]);
    exit(1);
  }

  // seedURL: normalize it, then require that it is internal
  char* normalized = normalizeURL(argv[1]);
  if (normalized == NULL) {
    fprintf(stderr, "error: cannot normalize seedURL '%s'\n", argv[1]);
    exit(2);
  }
  if (!isInternalURL(normalized)) {
    fprintf(stderr, "error: seedURL '%s' is not internal\n", normalized);
    free(normalized);
    exit(3);
  }
  *seedURL = normalized;

  // pageDirectory: must be an existing, writable directory we can mark
  *pageDirectory = argv[2];
  if (!pagedir_init(*pageDirectory)) {
    fprintf(stderr, "error: cannot initialize pageDirectory '%s'\n", *pageDirectory);
    free(normalized);
    exit(4);
  }

  // maxDepth: must be an integer, with no trailing junk, in range
  char excess; // catches a trailing non-numeric character, if any
  if (sscanf(argv[3], "%d%c", maxDepth, &excess) != 1) {
    fprintf(stderr, "error: maxDepth '%s' is not an integer\n", argv[3]);
    free(normalized);
    exit(5);
  }
  if (*maxDepth < 0 || *maxDepth > maxDepthLimit) {
    fprintf(stderr, "error: maxDepth must be in range [0, %d]\n", maxDepthLimit);
    free(normalized);
    exit(6);
  }
}

/**************** crawl ****************/
/*
 * Crawl from seedURL out to maxDepth, saving every fetched page in
 * pageDirectory.  Uses a bag of pages still to crawl and a hashtable of
 * URLs already seen, so no page is fetched twice.
 */
static void
crawl(char* seedURL, char* pageDirectory, const int maxDepth)
{
  // the set of URLs we have already seen (so we never re-add them)
  hashtable_t* pagesSeen = hashtable_new(hashSlots);
  // mem_assert is a helper function in libcs50, just returns a pointer, prints an error message if pointer is NULL
  mem_assert(pagesSeen, "crawl: could not create hashtable");
  hashtable_insert(pagesSeen, seedURL, ""); // mark the seed as seen

  // the bag of pages we still need to crawl, seeded at depth 0
  bag_t* pagesToCrawl = bag_new();
  mem_assert(pagesToCrawl, "crawl: could not create bag");
  // webpage_new takes ownership of its url string, so need to make a copy
  char* seedCopy = mem_malloc_assert(strlen(seedURL) + 1, "crawl: seed copy");
  strcpy(seedCopy, seedURL);
  webpage_t* seedPage = webpage_new(seedCopy, 0, NULL);
  mem_assert(seedPage, "crawl: could not create seed webpage");
  bag_insert(pagesToCrawl, seedPage);

  // iterate over pages until the bag is empty
  int docID = 0;
  webpage_t* page;
  while ((page = bag_extract(pagesToCrawl)) != NULL) {
    // fetch the HTML for this page (webpage_fetch enforces the 1-sec delay)
    if (webpage_fetch(page)) {
      logr("Fetched", webpage_getDepth(page), webpage_getURL(page));
      // save the fetched page to the next docID file
      pagedir_save(page, pageDirectory, ++docID);
      // if not yet at maxDepth, scan this page for more URLs to crawl
      if (webpage_getDepth(page) < maxDepth) {
        logr("Scanning", webpage_getDepth(page), webpage_getURL(page));
        pageScan(page, pagesToCrawl, pagesSeen);
      }
    }
    webpage_delete(page); // done with this page
  }

  // clean up: hashtable items are "" literals (not malloc'd), so need to pass NULL
  hashtable_delete(pagesSeen, NULL);
  bag_delete(pagesToCrawl, webpage_delete);
}

/**************** pageScan ****************/
/*
 * Scan the given (already-fetched) page for embedded URLs.  For each URL
 * that is internal and has not been seen before, add it to the hashtable
 * of seen URLs and to the bag of pages still to crawl (at depth+1).
 */
static void
pageScan(webpage_t* page, bag_t* pagesToCrawl, hashtable_t* pagesSeen)
{
  int depth = webpage_getDepth(page);
  int pos = 0;
  char* url;
  // find every URL found in the page's HTML
  while ((url = webpage_getNextURL(page, &pos)) != NULL) {
    char* normalized = normalizeURL(url);
    free(url);
    if (normalized != NULL) {
      logr("Found", depth, normalized);
      if (isInternalURL(normalized)) {
        // hashtable_insert returns true only if this URL is new
        if (hashtable_insert(pagesSeen, normalized, "")) {
          // hand a copy to a new webpage at the next depth, add it to the bag
          webpage_t* newPage = webpage_new(normalized, depth + 1, NULL);
          mem_assert(newPage, "pageScan: could not create webpage");
          bag_insert(pagesToCrawl, newPage);
          logr("Added", depth, normalized);
        }
        else {
          // if already seen: webpage_new did not take it, so free it
          logr("IgnDupl", depth, normalized);
          free(normalized);
        }
      }
      else {
        // if external URL: don't crawl
        logr("IgnExtrn", depth, normalized);
        free(normalized);
      }
    }
  }
}

/**************** logr ****************/
/*
 * Log one line of crawler progress to stdout, indented by depth, in the
 * format suggested by the knowledge unit, e.g.:
 *   0   Fetched: http://cs50tse.cs.dartmouth.edu/tse/letters/index.html
 */
static void
logr(const char* word, const int depth, const char* url)
{
  printf("%2d %*s%9s: %s\n", depth, depth, "", word, url);
}
