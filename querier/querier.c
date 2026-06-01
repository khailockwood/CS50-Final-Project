// Gift Christian, 5/30/2026, CS50 TSE Querier
// reads an index from the indexer and a page directory from the crawler,
// then answers boolean queries read one line at a time from stdin until EOF

// imports
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "index.h"
#include "pagedir.h"
#include "webpage.h"
#include "word.h"
#include "counters.h"
#include "file.h"
#include "mem.h"

// fileno() comes from stdio but some systems don't declare it in stdio.h,
// so declare it here ourselves to keep the compiler quiet
int fileno(FILE* stream);

// local types
// bundle two counters together so a counters_iterate callback can read from
// one set while writing into the other (used by intersect and unionize)
typedef struct two_counters {
  counters_t* a;
  counters_t* b;
} two_counters_t;

// used by rankAndPrint to remember the current highest-scoring document
typedef struct max_so_far {
  int docID;
  int score;
} max_so_far_t;

// function prototypes, commented so you can see what each one is for up front
// prompt: print "Query? "
static void prompt(void);

// confirm the line is only letters and whitespace
static bool cleanChars(const char* line);

// split a line into a lower-cased array of words
static char** tokenize(char* line, int* nwords);

// echo back the normalized query
static void printCleanQuery(char** words, int nwords);

// enforce the operator rules from the BNF
static bool validateQuery(char** words, int nwords);

//  walk the grammar and build the running score set
static counters_t* processQuery(char** words, int nwords, index_t* index);

// make a fresh duplicate of a counters set
static counters_t* copyCounters(counters_t* src);

// iterate callback that copies one entry across
static void copyHelper(void* arg, const int key, const int count);

// keep the per-doc minimum of two counters (the "and" step)
static void intersect(counters_t* product, counters_t* wordCounters);

// iterate callback doing the min for one doc
static void intersectHelper(void* arg, const int key, const int count);

// add one counters set onto another (the "or" step)
static void unionize(counters_t* sum, counters_t* product);

// iterate callback doing the add for one doc
static void unionHelper(void* arg, const int key, const int count);

// print the matches in decreasing score order
static void rankAndPrint(counters_t* sum, const char* pageDirectory);

// iterate callback tracking the highest score seen
static void findMax(void* arg, const int key, const int count);

// iterate callback counting docs with a score above zero
static void countPositive(void* arg, const int key, const int count);

// true if the word is literally "and" or "or"
static bool isOperator(const char* word);

// main function
int main(const int argc, char* argv[]) {
  // we expect exactly two arguments: the page directory and the index file
  if (argc != 3) {
    fprintf(stderr, "usage: %s pageDirectory indexFilename\n", argv[0]);
    return 1;     // wrong number of arguments, exit with error code 1
  }

  const char* pageDirectory = argv[1];      // the crawler directory
  const char* indexFilename = argv[2];      // the index file

  // make sure the page directory was really produced by our crawler
  if (!pagedir_validate(pageDirectory)) {
    fprintf(stderr, "Error: '%s' is not a crawler directory\n", pageDirectory);
    return 2;     // bad page directory
  }

  // load the index file into memory
  index_t* index = index_load(indexFilename);
  if (index == NULL) {
    fprintf(stderr, "Error: cannot read index from '%s'\n", indexFilename);
    return 3;     // couldn't read the index
  }

  // main query loop: prompt, then read one line at a time until EOF
  char* line;
  while ((line = file_readLine(stdin)) != NULL) {
    prompt();     // print the prompt if we're interactive

    // reject the line right away if it has any character that isn't allowed
    if (!cleanChars(line)) {
      free(line);     // cleanChars already printed the error, free the line
      continue;       // and continue
    }

    int nwords = 0;                             // tokenize will fill this in
    char** words = tokenize(line, &nwords);     // split the line into words
    
    if (words == NULL) {
      // tokenize only returns null when malloc failed, so bail out cleanly
      fprintf(stderr, "Error: out of memory\n");
      free(line);
      index_delete(index);    // free the index before exiting
      return 4;               // exit with error code 4
    }

    if (nwords == 0) {        // blank line: nothing to do, stay silent
      // free the malloced arrays and continue
      free(words);
      free(line);
      continue;
    }
    // echo back the cleaned-up query
    printCleanQuery(words, nwords);     
    
    // check the operator rules before we try to actually answer the query
    if (!validateQuery(words, nwords)) {
      free(words);      // validateQuery already printed the error
      free(line);
      continue;
    }
    
    // do the real work, process the query and get a counters set of docID : score
    counters_t* sum = processQuery(words, nwords, index);   
    
    rankAndPrint(sum, pageDirectory);     // print the ranked results
    counters_delete(sum);                 // done with this query's scores
    free(words);                          // free the word array
    free(line);                           // free the line itself
  }

  if (isatty(fileno(stdin))) {    // if we were interactive
    putchar('\n');                // print a newline so the shell prompt is on its own line
  }

  index_delete(index);    // free the whole index
  return 0;               // and we're done!
}

// prompt function, prints "Query? " when stdin is a terminal, otherwise stay quiet
// so that scripted/piped runs produce clean output with no stray prompts
static void prompt(void) {

  if (isatty(fileno(stdin))) {    // only prompt when a human is typing
    printf("Query? ");            // ask for a query
    fflush(stdout);               // flush so the prompt shows before we block on read
  }
}

// confirm the line is only letters and whitespace
// on the first bad character, print an error to stdout and return false
static bool cleanChars(const char* line) {
  if (line == NULL) {     // defensive: nothing to check
    return false;
  }

  // walk every character in the line
  for (const char* p = line; *p != '\0'; p++) {
    // a character is only okay if it's a letter or whitespace
    if (!isalpha((unsigned char)*p) && !isspace((unsigned char)*p)) {
      printf("Error: bad character '%c' in query.\n", *p);
      return false;     // first offender ends the check
    }
  }
  return true;      // got all the way through, line is clean
}

// tokenize: split line into a malloced array of pointers into line itself
// each pointer is the start of a word, and we null-terminate the word by
// overwriting the space after it. each word is lower-cased in place. returns
// the array (caller frees) and sets *nwords. the tokens point into line, so
// they go invalid once line is freed.
static char** tokenize(char* line, int* nwords) {
  if (line == NULL || nwords == NULL) {     // defensive against bad input
    return NULL;
  }

  // upper bound on word count: every other character could start a new word
  size_t cap = strlen(line) / 2 + 2;
  char** words = mem_malloc(sizeof(char*) * cap);   // room for the pointers
  if (words == NULL) {                              // malloc failed, let the caller handle it
    return NULL;
  }

  int n = 0;            // number of words found so far
  char* p = line;       // like the cursor walking along the line
  while (*p != '\0') {
    // skip any leading whitespace before the next word
    while (*p != '\0' && isspace((unsigned char)*p)) {
      p++;
    }
    if (*p == '\0') {       // hit the end while skipping spaces, so stop
      break;
    }
    words[n++] = p;     // this is the start of a word, record it

    // now walk forward over the word itself until the next space or end
    while (*p != '\0' && !isspace((unsigned char)*p)) {
      p++;
    }
    if (*p != '\0') {       // if we stopped on a space, turn it into a terminator
      *p++ = '\0';
    }
  }

  // lowercase every word we found, in place
  for (int i = 0; i < n; i++) {
    normalizeWord(words[i]);
  }
  *nwords = n;      // hand the count back to the caller
  return words;
}

// echo the normalized query back, one space before each word
static void printCleanQuery(char** words, int nwords) {
  printf("Query:");
  for (int i = 0; i < nwords; i++) {
    printf(" %s", words[i]);    // leading space keeps the words tidy
  }
  putchar('\n');
}

// return true if the word in is one of the literal operators "and"/"or"
static bool isOperator(const char* word) {
  return word != NULL && (strcmp(word, "and") == 0 || strcmp(word, "or") == 0);
}

// enforce the BNF rules; no operator first or last, and no
// two operators next to each other. prints an error on the first violation.
static bool validateQuery(char** words, int nwords) {
  // an operator can't be the very first word
  if (isOperator(words[0])) {
    printf("Error: '%s' cannot be first\n", words[0]);
    return false;
  }
  // an operator can't be the very last word either
  if (isOperator(words[nwords - 1])) {
    printf("Error: '%s' cannot be last\n", words[nwords - 1]);
    return false;
  }
  // and no two operators may sit right next to each other
  for (int i = 1; i < nwords; i++) {
    if (isOperator(words[i - 1]) && isOperator(words[i])) {
      printf("Error: '%s' and '%s' cannot be adjacent\n", words[i - 1], words[i]);
      return false;
    }
  }
  return true;      // passed every rule
}

// this function walks the word array following the BNF grammar
//
//     query       ::= andsequence [or andsequence]...
//     andsequence ::= word [[and] word]...
//
// within one and-sequence we keep a product counters set that starts as a
// copy of the first word's counters and is intersected with each later word's
// counters. at the end of each and-sequence we union the product into sum,
// the running OR-total we hand back to the caller.
static counters_t* processQuery(char** words, int nwords, index_t* index) {
  counters_t* sum = counters_new();     // the running total across or-groups

  if (sum == NULL) {        // malloc failed
    return NULL;            // return null to indicate failure
  }

  int i = 0;                // index into the word array

  while (i < nwords) {
    // seed this and-sequence with a copy of the first word's counters
    counters_t* product = copyCounters(index_find(index, words[i]));

    // ran out of memory mid-query
    if (product == NULL) {
      counters_delete(sum);   // throw away what we had
      return counters_new();  // and return empty
    }

    i++;

    // keep folding words into the product until we hit an "or" or the end
    while (i < nwords && strcmp(words[i], "or") != 0) {
      if (strcmp(words[i], "and") == 0) {   // explicit "and" is just a separator
        i++;
        continue;
      }
      counters_t* wordC = index_find(index, words[i]);    // this word's counters
      intersect(product, wordC);    // and-step, keep the per-doc minimum
      i++;
    }

    unionize(sum, product);     // or-step, add this and-sequence into the total
    counters_delete(product);   // done with the product for this group

    // if we stopped on an "or", step past it to start the next and-sequence
    if (i < nwords && strcmp(words[i], "or") == 0) {
      i++;
    }
  }
  return sum;       // the finished score set
}

// return a freshly-allocated counters set mirroring its source
// a NULL src (word not in the index) becomes an empty set, so a later
// intersect correctly zeroes everything out
static counters_t* copyCounters(counters_t* src) {
  counters_t* dst = counters_new();     // start empty
  if (dst == NULL) {                    // malloc failed
    return NULL;
  }

  if (src != NULL) {        // only copy if there's something to copy
    counters_iterate(src, dst, copyHelper);
  }

  return dst;
}

// iterating function callback that sets one (key, count) into the dst set
static void copyHelper(void* arg, const int key, const int count) {
  counters_t* dst = arg;            // arg is the destination counters
  counters_set(dst, key, count);    // copy this entry straight across
}

// for every docID in product, replace its count with the minimum
// of its count and the count in wordCounters. a NULL wordCounters makes the
// other count zero; exactly the spec'd behaviour for a word that's not in the
// index
static void intersect(counters_t* product, counters_t* wordCounters) {
  two_counters_t ctx = { .a = product, .b = wordCounters };   // bundle both sets
  counters_iterate(product, &ctx, intersectHelper);
}

// iterate callback doing the per-doc minimum
static void intersectHelper(void* arg, const int key, const int count) {
  two_counters_t* ctx = arg;        // unpack our two counters
  // if the other set is null the word was missing, so its count is zero
  int other; // declared in the function's scope

  if (ctx->b == NULL) {
    other = 0; // word missing -> count is 0
  } else {
    other = counters_get(ctx->b, key);
  }

  int min;
  if (count < other) {
    min = count;    // the current product count is less than the other count
  } else {
    min = other;
  }

  counters_set(ctx->a, key, min);   // store the minimum back into product
}

// for every docID in product, add its count onto the matching
// entry in sum (set union where the score is summed)
static void unionize(counters_t* sum, counters_t* product) {
  two_counters_t ctx = { .a = sum, .b = product };    // bundle both sets
  counters_iterate(product, &ctx, unionHelper);
}

// iterate callback adding one doc's count into the sum
static void unionHelper(void* arg, const int key, const int count) {
  two_counters_t* ctx = arg;        // unpack our two counters
  if (count == 0) {                 // a zero count adds nothing, so skip it
    return;
  }

  int existing = counters_get(ctx->a, key);     // whatever sum already had
  counters_set(ctx->a, key, existing + count);  // add this group's score on top
}

// rankAndPrint: print the result set in decreasing score order. done as a
// selection sort over the counters; find the max, print it, zero it, repeat.
static void rankAndPrint(counters_t* sum, const char* pageDirectory) {
  int npositive = 0;        // how many docs actually scored above zero
  counters_iterate(sum, &npositive, countPositive);

  if (npositive == 0) {     // nothing matched, say so and stop
    printf("No documents match.\n");
    return;
  }

  printf("Matches %d documents (ranked):\n", npositive);

  // pull out the highest-scoring doc npositive times
  for (int i = 0; i < npositive; i++) {
    max_so_far_t best = { .docID = -1, .score = 0 };  // reset the tracker
    counters_iterate(sum, &best, findMax);            // find the current max
    
    if (best.docID < 0) {                             // safety: nothing left to print
      break;
    }

    // we only want its URL
    webpage_t* page = pagedir_load(pageDirectory, best.docID);
    const char* url = (page != NULL) ? webpage_getURL(page) : "(unknown URL)";

    printf("score %3d doc %4d: %s\n", best.score, best.docID, url);

    if (page != NULL) {     // webpage_delete frees the page and its URL together
      webpage_delete(page);
    }

    counters_set(sum, best.docID, 0);   // zero it so next pass finds the next max
  }
}

// findMax: iterate callback that remembers the highest score seen so far
static void findMax(void* arg, const int key, const int count) {
  max_so_far_t* best = arg;     // our running best
  if (count > best->score) {    // strictly greater so ties keep the first one
    best->score = count;
    best->docID = key;
  }
}

// countPositive: iterate callback that counts docs with a score above zero
static void countPositive(void* arg, const int key, const int count) {
  (void) key;       // silences the unused warning
  int* n = arg;     // arg is our running count
  if (count > 0) {  // only docs that actually matched count
    (*n)++;
  }
}
