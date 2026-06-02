#!/bin/bash
#
#testing.sh - test script for the TSE indexer
#Lyndon Huang, Khai Lockwood, Gift Christian, 6/2/26
#

pageDir=../querier/test-data/tiny
output=tiny.index.test

# --- invalid arguments: each should print an error and exit non-zero ---

# no arguments
./indexer
echo "exit status: $?"

# one argument (missing indexFilename)
./indexer $pageDir
echo "exit status: $?"

# too many arguments
./indexer $pageDir $output extra
echo "exit status: $?"

# pageDirectory does not exist
./indexer /no/such/dir $output
echo "exit status: $?"

# directory exists but is not a crawler directory (no .crawler marker)
./indexer ../common $output
echo "exit status: $?"

# indexFilename cannot be created (parent directory does not exist)
./indexer $pageDir /no/such/dir/out.index
echo "exit status: $?"

# --- valid run: should succeed silently and exit zero ---

# build an index from the tiny pageDirectory
./indexer $pageDir $output
echo "exit status: $?"

# round-trip through indextest; an empty diff means load/save preserved it
./indextest $output $output.new
diff <(sort $output) <(sort $output.new)

# --- memory checks: both should report zero leaks and zero errors ---

valgrind --leak-check=full ./indexer $pageDir $output
valgrind --leak-check=full ./indextest $output $output.new
