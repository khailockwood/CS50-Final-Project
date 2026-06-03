#!/bin/bash

# Gift Christian, CS50 TSE Querier
# regression tests for the querier
#
# Unlike a static hand-built index, this script builds its test data live:
# it runs the crawler on the CS50 'letters' playground, runs the indexer on
# that crawl, and then runs the querier against the *real* crawler/indexer
# output. That way the test exercises the whole TSE pipeline end to end.

set -u       # blow up if i ever use a variable i forgot to set

CRAWLER=../crawler/crawler   # the crawler built in the crawler lab
INDEXER=../indexer/indexer   # the indexer built in the indexer lab
SEED=http://cs50tse.cs.dartmouth.edu/tse/letters/index.html  # small, stable site
DEPTH=5                      # depth 5 saturates the letters graph (9 pages)

DIR=pages-letters            # the page directory the crawler will fill
IDX=letters.index            # the index file the indexer will produce
BAD=/no/such/path            # a path that doesn't exist, for the error tests

# 0. build the test data from the real crawler and indexer
# crawl the letters site, then index what we crawled. if either step fails
# (e.g. no network) the querier tests below still run their argument checks.

rm -rf "$DIR" "$IDX"
mkdir -p "$DIR"
$CRAWLER "$SEED" "$DIR" "$DEPTH"
$INDEXER "$DIR" "$IDX"

# 1. command-line argument checks

# no arguments at all, so it should print usage
./querier

# only one argument, still wrong, should print usage
./querier $DIR

# three arguments, one too many, should print usage
./querier $DIR $IDX extra

# page directory that doesn't exist, should say it's not a crawler dir
./querier $BAD $IDX

# page directory that exists but has no .crawler sentinel, same complaint
./querier . $IDX

# index file that doesn't exist, should say it can't read the index
./querier $DIR $BAD

# 2. ranked queries against the real crawled-and-indexed letters site
# every page links 'home', so a 'home' query matches all 9 documents.
# the index page is the only one carrying 'playground' and 'tse'.
# and/or operators let me intersect and union the matching sets.

./querier $DIR $IDX <<EOF
home
tse
playground
breadth or depth
breadth and depth
fast and fourier and transform
computational and biology
home and tse
playground or eniac
EOF

# 3. the syntax error messages from the spec
# operator-first, operator-last, two-operators-adjacent, and the
# bad-character cases are all here

./querier $DIR $IDX <<EOF
and
or
and earth
or earth
planet earth or
planet earth and
planet earth and or science
planet earth and and science
planet earth or and science
Warning!
(The Lunar Chronicles #4.5)
git-flow
EOF

# 4. whitespace, case folding, short words, and empty input
./querier $DIR $IDX <<EOF
   BREADTH    first
   home   AND   tse
a home tse
EOF

# empty input file, the querier should just exit cleanly with no output
./querier $DIR $IDX </dev/null

# 5. one big scripted batch of queries in a single run
./querier $DIR $IDX <<EOF
algorithm
graph
traversal
algorithm graph
algorithm or traversal
algorithm and traversal
notInTheIndex
EOF
