# TSE Crawler

The crawler crawls a website starting from a "seed" URL, following links to
internal pages out to a given maximum depth, and saves every page it fetches
into a pageDirectory.

## Usage

```bash
./crawler seedURL pageDirectory maxDepth
```

* `seedURL` — an internal URL (one that normalizes to begin with
  `http://cs50tse.cs.dartmouth.edu/tse/`) to start crawling from.
* `pageDirectory` — an existing, writable directory in which to save pages;
  the crawler marks it by creating a `.crawler` file.
* `maxDepth` — an integer in the range `[0, 10]` giving the maximum crawl depth
  (`0` crawls only the seed page).

Example:

```bash
mkdir -p ../data/letters
./crawler http://cs50tse.cs.dartmouth.edu/tse/letters/index.html ../data/letters 2
```

## Building and testing

* `make` builds the `crawler` executable (run from this directory; the
  `common` and `libcs50` libraries must be built first, or just run `make`
  from the top-level directory).
* `make test` runs `testing.sh`.
* `make test &> testing.out` saves the test output.
* `make clean` removes the executable, object files, and test output.

## Assumptions

* The `pageDirectory` already exists and is writable; the crawler does not
  create it (`testing.sh` and the user are responsible for `mkdir`).
* The `pageDirectory` does not already contain files named with integers
  (`1`, `2`, ...), per the Requirements Spec; existing such files would be
  overwritten.
* `maxDepth` fits in an `int`, and document IDs fit in an `int`.

## Implementation notes and deviations

* The crawler relies on `webpage_fetch` to enforce the required one-second
  pause between fetches, so it does not implement that delay itself.
* Logging is printed to stdout in the format suggested by the knowledge unit
  (an action word indented by depth, followed by the URL).
* Out-of-memory conditions are treated as fatal via the `mem_assert` family,
  which print to stderr and exit non-zero.

## Known bugs / limitations

* None known. The crawler passes all cases in `testing.sh`, and valgrind
  reports no memory errors or leaks on a normal exit.
