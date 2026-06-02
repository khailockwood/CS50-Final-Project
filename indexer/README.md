# CS50 TSE Indexer

This directory contains the Indexer for the Tiny Search Engine, along with the
`indextest` unit-test program. See `IMPLEMENTATION.md` for implementation
details and the testing plan, and `REQUIREMENTS.md` and `DESIGN.md` for the
specifications.

## Building and testing

Run `make` to build `indexer` and `indextest` (or `make` from the top-level
directory to build the whole project). Run `make test` to run the test suite,
or `make test &> testing.out` to capture its output. Run `make clean` to remove
the built programs and test artifacts.

## Assumptions

We rely on the assumptions permitted by the Requirements Spec:

- The `pageDirectory` contains files named `1`, `2`, `3`, ... with no gaps, so
  the indexer stops as soon as it cannot open the next numbered file.
- The files in `pageDirectory` follow the Crawler's format (URL on line 1,
  depth on line 2, HTML on the remaining lines), so the code that reads them
  does minimal error checking.
- Index files read by `indextest` follow the standard index file format.

When building a fresh index we cannot know the eventual word count in advance,
so we use a fixed hashtable size of 500 slots.

## Differences from the Specs

The Design Spec's pseudocode suggests a separate argument-parsing step and an
`indexBuild` that creates, fills, and writes the index. Our implementation uses
three functions: `main`, `indexBuild`, and `indexPage`. `main` validates the
command-line arguments directly, creates the index, calls `indexBuild` to fill
it, writes it with `index_save`, and frees it with `index_delete`. `indexBuild`
is therefore responsible only for building the in-memory index from the pages in
`pageDirectory`. This is a functionally equivalent reorganization, and it is
reflected in `IMPLEMENTATION.md`.

## Known limitations

We are not aware of any bugs. The indexer and indextest pass all tests in
`testing.sh` and run cleanly under valgrind, with no memory errors or leaks.

The shared CS50 corpus (`letters`, `toscrape`, `wikipedia`) and the `indexcmp`
tool were not available on our development server, so the indexer was validated
against the smaller `tiny` pageDirectory, using a sorted `diff` and the
`indextest` round-trip rather than `indexcmp`.
