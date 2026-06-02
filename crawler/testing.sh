#!/bin/bash
#
# testing.sh - test script for the TSE 'crawler'
#
# Usage: bash -v testing.sh
#   (run via 'make test', which saves output with: make test &> testing.out)
#
# This script tests the crawler in three phases:
#   1. erroneous command-line arguments (each should fail with non-zero exit)
#   2. a valgrind run over a small crawl, to check for memory errors/leaks
#   3. correct crawls over the test sites at increasing depths
#

# the seed URLs for the three test sites
LETTERS=http://cs50tse.cs.dartmouth.edu/tse/letters/index.html
TOSCRAPE=http://cs50tse.cs.dartmouth.edu/tse/toscrape/index.html
WIKIPEDIA=http://cs50tse.cs.dartmouth.edu/tse/wikipedia/index.html

# put all test output under ../data, fresh each run
rm -rf ../data
mkdir -p ../data

#########################################################################
# 1. erroneous arguments: each invocation must fail (non-zero exit status)
#########################################################################

# no arguments
./crawler

# too few arguments
./crawler $LETTERS

# too many arguments
./crawler $LETTERS ../data 1 extra

# seedURL that cannot be normalized / is not a valid URL
./crawler not-a-url ../data 1

# seedURL that is external (not internal to the CS50 test server)
./crawler http://www.example.com/ ../data 1

# pageDirectory that does not exist (cannot create .crawler there)
./crawler $LETTERS ../data/does-not-exist 1

# maxDepth that is not an integer
./crawler $LETTERS ../data notanumber

# maxDepth below range
./crawler $LETTERS ../data -1

# maxDepth above range
./crawler $LETTERS ../data 11

#########################################################################
# 2. valgrind over a small crawl (letters at depth 2)
#########################################################################

mkdir -p ../data/letters-valgrind
# run under valgrind where available (like the CS50 server);
# skip gracefully on machines without it (macOS locally)
if command -v valgrind &> /dev/null; then
  valgrind --leak-check=full --show-leak-kinds=all \
    ./crawler $LETTERS ../data/letters-valgrind 2
else
  echo "valgrind not available on this machine; skipping memory check"
fi

#########################################################################
# 3. correct crawls over the CS50 test sites
#########################################################################

# letters site at depths 0, 1, 2
for depth in 0 1 2; do
  mkdir -p ../data/letters-$depth
  ./crawler $LETTERS ../data/letters-$depth $depth
done

# toscrape site at depth 1 (depth 2+ takes a very long time)
mkdir -p ../data/toscrape-1
./crawler $TOSCRAPE ../data/toscrape-1 1

# wikipedia site at depth 1
mkdir -p ../data/wikipedia-1
./crawler $WIKIPEDIA ../data/wikipedia-1 1
