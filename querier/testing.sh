#!/bin/bash

# Gift Christian, 5/30/2026, CS50 TSE Querier
# regression tests for the querier

set -u       # blow up if i ever use a variable i forgot to set

DIR=test-data/tiny           # the little hand-built page directory
IDX=test-data/tiny.index     # the matching index file
BAD=/no/such/path            # a path that doesn't exist, for the error tests

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

# 2. the score table from the requirements spec
# the tiny index reproduces the three-document example in REQUIREMENTS.md
# (FILE1/FILE2/FILE3 with cat, dog, emu), so i can check the rankings exactly:
# cat and dog gives FILE3 a score of 3, the highest
# cat or dog gives FILE3 a score of 7, the highest
# dog or dog gives FILE1 a score of 10, the highest
# cat and dog or emu gives FILE1 a score of 7, the highest

./querier $DIR $IDX <<EOF
cat and dog
cat or dog
dog or dog
cat and dog or emu
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
   DARTMOUTH    College
   cat   AND   dog
a cat dog
EOF

# empty input file, the querier should just exit cleanly with no output
./querier $DIR $IDX </dev/null


# 5. one big scripted batch of queries in a single run
./querier $DIR $IDX <<EOF
cat
dog
emu
cat dog
cat or emu
cat and emu
notInTheIndex
EOF
