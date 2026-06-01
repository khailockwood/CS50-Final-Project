# common

This directory assembles code shared by the Crawler, Indexer, and Querier of
the Tiny Search Engine.  For the Crawler, it provides the `pagedir` module.

## pagedir

`pagedir.c` / `pagedir.h` implement the *pagesaver* described in the Crawler
Design Spec:

* `pagedir_init()` creates a `.crawler` marker file in the pageDirectory,
  identifying it as a directory produced by the Crawler.
* `pagedir_save()` writes one fetched webpage to a file named by its docID,
  in the format: URL on line 1, depth on line 2, HTML on the remaining lines.

## Building

`make` builds `common.a`.
`make clean` removes the library and object files.

## Assumptions / notes

* `pagedir_init()` assumes the pageDirectory already exists and is writable;
  it does not create the directory itself.
* `pagedir_save()` treats a failure to open the output file as fatal, since a
  correctly initialized directory should always be writable; it prints a
  message to stderr and exits non-zero.
* docID is assumed to fit in a standard `int` (buffer sized for that).
