# CS50 TSE
## Gift Christian, Lyndon Huang, Khai Lockwood

The assignment and Specs are in a [public repo](https://github.com/CS50DartmouthWI25/home/tree/main/labs/tse).
Do not clone that repo; view it on GitHub.
Watch there for any commits that may represent updates to the assignment or specs.

## Overview

This repository implements the three subsystems of the Tiny Search Engine:

* **`crawler/`** — crawls a website from a seed URL out to a maximum depth and
  saves each fetched page into a pageDirectory.
* **`indexer/`** — builds an inverted index (word → docID → count) from a
  pageDirectory; `indextest` round-trips an index file to verify load/save.
* **`querier/`** — answers ranked `and`/`or` queries from stdin against a
  pageDirectory and its index file.

Two support directories back these programs:

* **`common/`** — shared `pagedir`, `index`, and `word` modules (`common.a`).
* **`libcs50/`** — the provided CS50 data-structure library (`libcs50.a`).

See each subdirectory's `README.md`, `DESIGN.md`, and `IMPLEMENTATION.md` for
component-level detail, assumptions, and testing plans.

## Building and testing

From the top-level directory:

* `make` (or `make all`) builds `libcs50`, `common`, and all three programs.
* `make clean` removes every build artifact and test output across the tree.

Each component is tested from its own directory:

* `make test` runs that component's `testing.sh`.
* `make test &> testing.out` captures the output (the committed `testing.out`
  files were produced this way).

## Assumptions and deviations from the Specs

* **pageDirectory must already exist.** Per the Crawler Requirements Spec, the
  programs do not create the pageDirectory; the caller (or `testing.sh`) is
  responsible for `mkdir`. The crawler marks the directory with a `.crawler`
  file, which the indexer and querier check via `pagedir_validate()`.
* **Trusted input formats.** As the specs permit, the indexer and querier do
  minimal error checking on the contents of crawler page files and index
  files, assuming both follow the documented formats.
* **Document IDs fit in an `int`,** and page files are numbered `1, 2, 3, ...`
  with no gaps; the indexer and querier stop at the first missing numbered file.
* **Indexer organization.** `indexBuild` is split across `main`/`indexBuild`/
  `indexPage` rather than the single-function form in the Design Spec's
  pseudocode; this is functionally equivalent (see `indexer/IMPLEMENTATION.md`).

## Known limitations

* No known bugs. All three components pass their `testing.sh` suites, and the
  indexer/querier run cleanly under valgrind (no memory errors or leaks).
* The shared CS50 test server (`cs50tse.cs.dartmouth.edu`) is reachable only
  from the Dartmouth network, so the crawler's live-crawl tests and on-server
  valgrind run must be performed there; `crawler/testing.out` reflects a run
  with network access available.

