# CS50 TSE querier
## Design Spec

In this document we reference the [Requirements Specification](REQUIREMENTS.md) from github and focus on the implementation-independent design decisions.


Here we focus on the core subset:

- User interface
- Inputs and outputs
- Functional decomposition into modules
- Pseudo code (plain English-like language) for logic/algorithmic flow
- Major data structures

## User interface

As described in the [Requirements Spec](REQUIREMENTS.md), the querier has two interfaces with the user: the command line, and an interactive prompt on stdin.

On the command line it takes exactly two arguments:

```
$ querier pageDirectory indexFilename
```

For example, to query the index built from a depth-2 crawl of the letters site:

```
$ ./querier ../data/letters ../data/letters.index
```

Once running, the querier prompts the user with Query? and reads one query per line from stdin until EOF.

## Inputs and outputs

*Input:* the querier reads from three places:

- pageDirectory on the command line — a directory produced by the crawler. Each file inside is named by docID (1, 2, 3, ...) and has the URL on its first line.
- indexFilename on the command line — a file produced by the indexer; one word per line followed by docID count pairs.
- stdin — one query per line until EOF.

*Output:* for each query, the querier writes to stdout:

- the cleaned query echo, in the form Query: word word word,
- either No documents match. when nothing satisfies the query, or
- a header Matches N documents (ranked): followed by one score S doc D: URL line per matching document, in decreasing score order.

Unrecoverable errors (bad command line, missing pageDirectory/.crawler, unreadable index file, out of memory) print a message to stderr and exit with a non-zero status. Recoverable query-syntax errors (bad character, operator first/last, two adjacent operators) print a message inline to stdout and the prompt loop continues.

## Functional decomposition into modules

We anticipate the following modules or functions:

 1. *main*, which parses arguments, loads the index, runs the query loop, and cleans up
 2. *prompt*, which writes Query? to stdout only when stdin is a terminal
 3. *cleanChars*, which checks that the raw line contains only letters and spaces
 4. *tokenize*, which splits the line into a normalized (lowercased) array of words
 5. *validateQuery*, which checks the word array satisfies the BNF structure (no operator first or last, no two operators adjacent)
 6. *processQuery*, which walks the word array following the BNF grammar and builds the result counters
 7. *intersect* and *unionize*, the helpers that combine counters (AND-product and OR-sum)
 8. *rankAndPrint*, which prints the result set in decreasing score order

And we lean on these modules from earlier labs:

  1. *index* (from common) — index_load reads the index file into an index_t, index_find looks up a word, index_delete frees it
  2. *pagedir* (from common) — pagedir_validate checks for pageDirectory/.crawler, and a helper reads the URL from pageDirectory/D
  3. *word* (from common) — normalizeWord lowercases a word in place
  4. *counters* (from libcs50) — the core data type for (docID, count) sets

## Pseudo code for logic/algorithmic flow

The querier will run as follows:

    parse the command line, 
    validate parameters
    validate pageDirectory by checking for pageDirectory/.crawler
    count lines in indexFilename to size the hashtable
    load the index from indexFilename into an index_t
    loop:
        if stdin is a terminal, print "Query?"
        read one line from stdin; exit cleanly on EOF
        if the line contains any character that is not a letter or space:
            print "Error: bad character 'X' in query." and continue
        tokenize the line into a lowercased array of words
        if the array is empty (blank line), continue
        print "Query: word word word"
        validate the array structure:
            if the first or last word is "and" or "or", print error and continue
            if any two adjacent words are both operators, print error and continue
        run processQuery to build the result counters
        rank and print the result counters
        free per-query memory
    free the index and exit zero

Inside processQuery we follow the BNF grammar with two nested loops:

    sum = new empty counters                        # running OR-total
    while there are more words:
        product = counters copied from index lookup of the current word
        advance past the word
        while there are more words and the current word is not "or":
            if the current word is "and", just advance
            otherwise intersect product with the index lookup of the current word
            advance
        unionize sum with product
        delete product
        if the current word is "or", advance past it
    return sum

intersect(product, wordCounters) walks every (docID, count) in product and replaces each with min(count, count_in_wordCounters). Documents missing from wordCounters end up with score zero. unionize(sum, product) walks every (docID, count) in product and adds it onto the matching entry in sum.

After processQuery returns, rankAndPrint does a selection sort over the result: find the docID with the highest score, print its line, zero that entry, and repeat until no positive scores remain.


## Major data structures

Helper modules provide all the data structures we need:

- *index* — a hashtable mapping each word to a *counters* set of (docID, count) pairs; loaded once from indexFilename and read-only afterward
- *counters* — used three ways in the querier:
  - the per-word lookup result from the index (borrowed, never modified)
  - a *product* counters set, holding the running AND-result inside one and-sequence
  - a *sum* counters set, holding the running OR-result across the whole query
- a small *two_counters* struct that bundles two counters_t* pointers; we pass it as the arg to counters_iterate so the helper callback can read from one set and write to the other (the same trick we used in activity 16's set_intersect)