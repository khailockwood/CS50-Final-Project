/*
 * word.h - header file for CS50 'word' module
 *
 * The 'word' module provides a single function, normalizeWord, used by
 * the Indexer and Querier to put every word into a canonical form before
 * it is stored in or looked up from an index.
 *
 */

#ifndef __WORD_H
#define __WORD_H

/**************** normalizeWord ****************/
/*
 * Normalize a word by converting every letter to lower case, in place.
 *
 * Caller provides:
 *   a writable, null-terminated string (may be NULL).
 * Function does:
 *   lower-case each character of the string in place; if word is NULL,
 *   it does nothing.
 */
void normalizeWord(char* word);

#endif // __WORD_H
