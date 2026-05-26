# Querier — Implementation Spec

This document describes implementation choices specific to our querier. It builds on DESIGN.md and assumes the Requirements Spec is the source of truth for behavior. We are targeting the full 30/30 functionality tier (operators with precedence, ranked output).

## Data structures

The querier uses these concrete C types:

- `index_t* index` — the loaded index. Built by `index_load(indexFilename, nslots)`, where `nslots` is computed from the line count of the index file (roughly `max(1, lines)`).
- `char* line` — a single raw query line, malloced by `freadlinep(stdin)`.
- `char** words; int nwords;` — array of pointers into `line` (no separate strdup), each pointing at a normalized word. `nwords` is updated by `tokenize`.
- `counters_t* product` — running AND-product within an and-sequence.
- `counters_t* sum` — running OR-total across the whole query, returned by `processQuery`.
- `two_counters_t` — helper struct for iterator callbacks:

```c
typedef struct two_counters {
  counters_t* a;
  counters_t* b;
} two_counters_t;
```

- `max_so_far_t` — helper struct used by the ranking iterator:

```c
typedef struct max_so_far {
  int docID;
  int score;
} max_so_far_t;
```

## Sizing the hashtable

Before calling `index_load` we open the index file, count newlines with `file_numLines`, close it, and pass that as the slot count. One slot per word is a fine load factor for our scale.

## Control flow of main

```
1. check argc == 3; otherwise print usage error to stderr and exit 1
2. pageDirectory = argv[1]; indexFilename = argv[2]
3. pagedir_validate(pageDirectory); on failure exit 2
4. open indexFilename for reading; on failure exit 3
5. nslots = file_numLines(fp); fclose(fp)
6. index = index_load(indexFilename, max(1, nslots))
7. loop:
     prompt()
     line = freadlinep(stdin); if NULL, break
     if !cleanChars(line): print bad-char error, free, continue
     words = tokenize(line, &nwords)
     if nwords == 0: free, continue                # empty query: silent
     printCleanQuery(words, nwords)
     if !validateQuery(words, nwords): free, continue
     sum = processQuery(words, nwords, index)
     rankAndPrint(sum, pageDirectory)
     counters_delete(sum); free(words); free(line)
8. index_delete(index); return 0
```

## Function prototypes

Every static function in `querier.c`:

```c
static void  prompt(void);
static bool  cleanChars(char* line);
static char** tokenize(char* line, int* nwords);
static void  printCleanQuery(char** words, int nwords);
static bool  validateQuery(char** words, int nwords);
static counters_t* processQuery(char** words, int nwords, index_t* index);
static void  intersect(counters_t* product, counters_t* wordCounters);
static void  intersectHelper(void* arg, const int key, const int count);
static void  unionize(counters_t* sum, counters_t* product);
static void  unionHelper(void* arg, const int key, const int count);
static void  rankAndPrint(counters_t* sum, const char* pageDirectory);
static void  findMax(void* arg, const int key, const int count);
static void  countPositive(void* arg, const int key, const int count);
```

## Per-function pseudo code

### prompt

```
if isatty(fileno(stdin)):
    printf("Query? ")
    fflush(stdout)
```

### cleanChars

```
for each char c in line:
    if !isalpha(c) and !isspace(c):
        fprintf(stderr, "Error: bad character '%c' in query.\n", c)
        return false
return true
```

This runs before tokenizing so the user gets a clear error on the first bad character.

### tokenize

```
words = malloc(sizeof(char*) * (strlen(line)/2 + 1))   # upper bound: every other char is a word
n = 0
i = 0
while line[i] != '\0':
    skip spaces (line[i] = '\0' as we go to terminate the previous word? — see note)
    if line[i] == '\0': break
    words[n++] = &line[i]
    advance i past the word
    if line[i] != '\0': line[i++] = '\0'   # terminate this word
for j in 0..n-1: normalizeWord(words[j])
*nwords = n
return words
```

Tokens point into the original `line`, so freeing `line` invalidates `words` — we free `line` after we are done with `words`. We do not use `strtok` (Canvas warning).

### printCleanQuery

```
printf("Query:")
for j in 0..nwords-1:
    printf(" %s", words[j])
printf("\n")
```

### validateQuery

```
if words[0] is "and" or "or":
    fprintf(stderr, "Error: '%s' cannot be first\n", words[0])
    return false
if words[nwords-1] is "and" or "or":
    fprintf(stderr, "Error: '%s' cannot be last\n", words[nwords-1])
    return false
for j in 1..nwords-1:
    if both words[j-1] and words[j] are operators:
        fprintf(stderr, "Error: '%s' and '%s' cannot be adjacent\n", words[j-1], words[j])
        return false
return true
```

Character cleanliness is already enforced by `cleanChars`, so here we only check structure.

### processQuery

Implements the BNF described in DESIGN.md. Returns a fresh `counters_t*` the caller must delete.

```
sum = counters_new()
i = 0
while i < nwords:
    # start an and-sequence
    wordC = index_find(index, words[i])         # NULL or pointer borrowed from index
    product = counters_new()
    if wordC != NULL: copyCounters(wordC, product)   # seed product with first word
    i = i + 1
    while i < nwords and strcmp(words[i], "or") != 0:
        if strcmp(words[i], "and") == 0:
            i = i + 1
            continue
        wordC = index_find(index, words[i])
        intersect(product, wordC)               # wordC may be NULL → product becomes empty
        i = i + 1
    unionize(sum, product)
    counters_delete(product)
    if i < nwords and strcmp(words[i], "or") == 0:
        i = i + 1
return sum
```

`copyCounters` is a one-line helper that iterates `wordC` and calls `counters_set` on `product` for each (key, count). We do this because we never want to mutate counters that belong to the index.

### intersect

For each docID in `product`, keep it only if it is also in `wordCounters`, and store the minimum.

```c
static void intersect(counters_t* product, counters_t* wordCounters) {
  two_counters_t ctx = { .a = product, .b = wordCounters };
  counters_iterate(product, &ctx, intersectHelper);
}

static void intersectHelper(void* arg, const int key, const int count) {
  two_counters_t* ctx = arg;
  int other = (ctx->b == NULL) ? 0 : counters_get(ctx->b, key);
  int min = (count < other) ? count : other;
  counters_set(ctx->a, key, min);   # zero is fine; iterator is safe under set per libcs50
}
```

If `wordCounters` is NULL (the word wasn't in the index), `other` is 0 and every docID in `product` is zeroed — exactly the desired behavior.

### unionize

For each docID in `product`, add its count onto `sum`.

```c
static void unionize(counters_t* sum, counters_t* product) {
  two_counters_t ctx = { .a = sum, .b = product };
  counters_iterate(product, &ctx, unionHelper);
}

static void unionHelper(void* arg, const int key, const int count) {
  two_counters_t* ctx = arg;
  if (count == 0) return;
  int existing = counters_get(ctx->a, key);
  counters_set(ctx->a, key, existing + count);
}
```

### rankAndPrint

```
ctx.count = 0
counters_iterate(sum, &ctx, countPositive)
if ctx.count == 0:
    printf("No documents match.\n")
    return
printf("Matches %d documents (ranked):\n", ctx.count)
repeat ctx.count times:
    best.docID = -1; best.score = 0
    counters_iterate(sum, &best, findMax)
    if best.docID == -1: break        # safety
    url = pagedir_loadURL(pageDirectory, best.docID)
    printf("score\t%d doc\t%d: %s\n", best.score, best.docID, url)
    free(url)
    counters_set(sum, best.docID, 0)
```

`findMax` updates `best` when it sees a strictly higher score. `countPositive` increments a counter when `count > 0`.

`pagedir_loadURL(dir, id)` is a small helper in `common/pagedir` (added in lab 4) that opens `dir/id` and reads the first line.

## Error handling

The requirements spec splits errors into two groups: **unrecoverable** (print to stderr and exit non-zero) and **recoverable query-syntax** (print inline to stdout and prompt again). The example output in the requirements spec shows the latter mixed with normal `Query:` echoes, so they go to stdout.

| Case | Where | Stream | Message | Result |
|---|---|---|---|---|
| wrong argc | main start | stderr | `usage: querier pageDirectory indexFilename` | exit 1 |
| `pageDirectory/.querier` not readable | `pagedir_validate` | stderr | `Error: '%s' is not a querier directory` | exit 2 |
| `indexFilename` not readable | after `fopen` | stderr | `Error: cannot open '%s' for reading` | exit 3 |
| out of memory anywhere | after malloc / `_new` | stderr | `Error: out of memory` | exit 4 |
| `pageDirectory/1` not readable at print time | inside `rankAndPrint` | stderr | `Error: cannot read '%s/%d'` | exit 5 |
| bad character in query | `cleanChars` | stdout | `Error: bad character '%c' in query.` | skip query, next prompt |
| `and`/`or` first | `validateQuery` | stdout | `Error: '%s' cannot be first` | skip query |
| `and`/`or` last | `validateQuery` | stdout | `Error: '%s' cannot be last` | skip query |
| two operators adjacent | `validateQuery` | stdout | `Error: '%s' and '%s' cannot be adjacent` | skip query |
| empty query (only spaces) | after `tokenize` | — | nothing printed | silent continue |
| EOF on stdin | `freadlinep` returns NULL | — | nothing printed | clean exit 0 |

Notes:
- The bad-character check runs **before** the `Query:` echo, so a query like `Warning!` prints only the error, no echo. This matches the requirements-spec example output.
- Every error message string above matches verbatim with the example output in the requirements spec.
- Every function checks pointer parameters for NULL and returns early or returns a safe sentinel (NULL, false, 0). Internal allocation failures (e.g. `counters_new` returning NULL) propagate up to main, which prints to stderr and exits.

## Resource management

Every allocation has a matching free:
- `line` from `freadlinep` → freed at the end of every loop iteration.
- `words` from `tokenize` → freed at the end of every loop iteration. Individual entries are not freed because they point inside `line`.
- `product` inside `processQuery` → `counters_delete` after `unionize`.
- `sum` returned by `processQuery` → `counters_delete` after `rankAndPrint`.
- `url` from `pagedir_loadURL` → freed after each print line.
- `index` → `index_delete(index, counters_delete)` once on exit.

Valgrind should report zero leaks on a clean exit (EOF) and zero leaks on any handled error.

## Testing plan

`testing.sh` is run as `make test &> testing.out` and exercises:

1. **Bad arguments** — wrong count, bad pageDirectory, unreadable indexFile. Confirm exit codes and stderr messages.
2. **Single-word queries** — a word present, a word absent, an indexed word from the shared output.
3. **Implicit AND** — `computer science` matches docs with both.
4. **Explicit AND** — `computer and science` produces the same output as above.
5. **OR** — `computer or science` returns the union, scores summed.
6. **Mixed precedence** — `baseball or basketball or ultimate frisbee` and the toscrape examples from the requirements spec.
7. **Whitespace cleanup** — `  many   spaces   in   here  ` echoes with collapsed spaces.
8. **Case folding** — `DARTMOUTH College` echoes as `dartmouth college` and matches the lowercase version.
9. **Short words** — `a dartmouth student` returns no documents (1- and 2-letter words aren't in the index, per spec).
10. **Bad characters** — `Warning!`, `(The Lunar Chronicles #4.5)`, `git-flow`, `1234`.
11. **Operator first / last** — every variant from the spec: `and`, `or`, `and earth`, `or earth`, `planet earth and`, `planet earth or`.
12. **Adjacent operators** — `planet earth and or science`, `planet earth and and science`, `planet earth or and science`.
13. **Empty input / EOF** — blank lines and Ctrl-D both handled cleanly.
14. **Ranking sanity** — the score table from the requirements spec (D1/D2/D3 with cat/dog/emu queries) recreated against a tiny hand-built index; confirm scores and ordering match exactly. Also confirm `score` lines are non-increasing across larger queries.
15. **Fuzz** — `fuzzquery indexFilename 100 0 | ./querier pageDirectory indexFilename` to throw randomly-shaped valid queries at the parser.
16. **Valgrind** — at least one fuzz run and one error-path run under `valgrind --leak-check=full`.

Tests run against both a small local crawl/index pair (for debugging) and against `/cs50/shared/tse/output/` (for confidence at scale).

The script uses `bash -v` so the command and its output appear in `testing.out`, and a banner line separates each test case.

## Assumptions and limitations

These align with the "may assume" list in the requirements spec, plus a few of our own choices:

- `pageDirectory` contains contiguous files `1, 2, 3, ...` with no gaps, in querier format.
- `indexFilename` follows the index file format from Lab 5 — one word per line, followed by docID/count pairs.
- The index and pageDirectory correspond to each other; we do not detect mismatches.
- A NULL `counters_t*` returned from `index_find` is treated as an empty set (no special branch in callers other than `intersect`, which interprets a NULL `b` as zero counts).
- 1- and 2-letter words are not in the index by indexer spec, so queries containing them are simply unsatisfiable for those words; we do not warn or special-case them.
- Words longer than the longest line in the index file are still handled because we tokenize from a malloced line buffer of arbitrary length.
- We do not attempt to deduplicate matching documents across and-sequences beyond what `unionize` does — duplicates are correctly summed (matches the spec's `dog or dog` example where D1 scores 10).
- Any limitation we discover during testing will be documented in `README.md` per the grading guidance.
