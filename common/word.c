/*
 * word.c -'word' module
 *
 * See word.h for interface descriptions and usage.
 *
 */

#include <stdlib.h>
#include <ctype.h>
#include "word.h"

/**************** normalizeWord ****************/
/* see word.h for description */
void
normalizeWord(char* word)
{
  // defensive check
  if (word == NULL) {
    return;
  }

  // convert each character to lower case in place
  for (char* c = word; *c != '\0'; c++) {
    *c = tolower((unsigned char) *c);
  }
}
