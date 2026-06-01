# common

This directory assembles code shared by the Crawler, Indexer, and Querier of
the Tiny Search Engine.  It builds a single library, `common.a`, from three
modules.

## pagedir

`pagedir.c` / `pagedir.h` encapsulate all knowledge about the structure of a
pageDirectory:

* `pagedir_init()`    — create the `.crawler` marker file in a pageDirectory.
* `pagedir_save()`    — write one fetched webpage to a file named by its docID
  (URL on line 1, depth on line 2, HTML on the remaining lines).
* `pagedir_validate()`— confirm a directory was produced by the Crawler
  (used by the Indexer and Querier).
* `pagedir_load()`    — read a saved page file back into a `webpage_t`.

## index

`index.c` / `index.h` provide an abstract `index_t` type: a wrapper around a
hashtable mapping each word to a `counters_t` of (docID, count) pairs.

* `index_new` / `index_delete` / `index_find` / `index_insert`
* `index_save()` — write the index to a file in the standard format:
  one line per word, `word docID count [docID count]...`.
* `index_load()` — read such a file back into an `index_t`.

Shared by the Indexer, Querier, and `indextest`.

## word

`word.c` / `word.h` provide `normalizeWord()`, which lower-cases a word in
place.  Shared by the Indexer and Querier.

## Building

`make` builds `common.a` (modules `pagedir`, `index`, `word`).
`make clean` removes the library and object files.

## Assumptions / notes

* `pagedir_init()` assumes the pageDirectory already exists and is writable;
  it does not create the directory itself.
* `pagedir_save()` treats a failure to open the output file as fatal (prints
  to stderr and exits non-zero), since a correctly initialized directory
  should always be writable.
* `pagedir_load()` returns NULL if a page file is missing or malformed, which
  the Indexer uses as its signal to stop iterating over document IDs.
* `index_load()` assumes the input file is in the exact format produced by
  `index_save()`.
* docID is assumed to fit in a standard `int`.
