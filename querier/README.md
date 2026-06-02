# CS50 TSE querier
## Implementation overview

The querier is the third sub-system of the Tiny Search Engine. Given a pageDirectory produced by the crawler and an indexFilename produced by the indexer, it reads queries one per line from stdin and prints the matching documents ranked by score. S

## Building

From the top of the repo, make can builds everything. To build just the querier:

```
cd querier && make
```

## Usage

The querier takes exactly two arguments on the command line:

```
./querier pageDirectory indexFilename
```

For each line read from stdin it prints the cleaned query, then either No documents match. or a Matches N documents (ranked): header followed by one score S doc D: URL line per matching document, in decreasing score order.

## Testing

```
make test &> testing.out
```

this script is self-contained: it uses the small test-data/tiny page directory and test-data/tiny.index checked into the repo, which reproduce the FILE1, 2, 3 score table from the Requirements Spec exactly. It covers the command-line argument checks and failing and passing tests from querier.c code in one run

## What is implemented

The full functionality like single-word queries, implicit and explicit "and", "or", correct precedence and ranked output in decreasing score order

## Assumptions

- pageDirectory is a real crawler directory (it contains .crawler and files named 1, 2, 3, ...), for me i had to manually add this in the tiny directory for testing

- indexFilename follows the lab-5 index format, one word docID count [docID count]... per line.

- Words shorter than three letters 
are not in the index by the indexer spec, so any such query word simply matches nothing.

- A word missing from the index contributes zero counts to its and-sequence, which zeroes out the running product, exactly as the spec requires.

## Files

- querier.c — the program itself
- Makefile — build, test, clean
- testing.sh — regression tests run by make test
- testing.out — captured output of make test &> testing.out
- test-data/tiny/ — hand-built page directory used by the tests
- test-data/tiny.index — matching index file
- DESIGN.md — design spec
- IMPLEMENTATION.md — implementation spec and testing plan
