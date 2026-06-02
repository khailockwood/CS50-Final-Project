# Makefile for the TSE 'crawler'
#
# Builds the crawler executable, linking against the shared common.a
# library and the libcs50.a library.
#

L = ../libcs50
C = ../common

OBJS = crawler.o
LLIBS = $C/common.a $L/libcs50.a

CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$L -I$C $(FLAGS)
CC = gcc
MAKE = make

# build the crawler executable by linking the object file with the libraries
crawler: $(OBJS) $(LLIBS)
	$(CC) $(CFLAGS) $(OBJS) $(LLIBS) -o crawler

# crawler.o depends on these headers
crawler.o: $C/pagedir.h $L/bag.h $L/hashtable.h $L/webpage.h $L/mem.h

.PHONY: test clean

# run the test script, intermixing each command with its output
test: crawler testing.sh
	bash -v testing.sh

# remove all files produced by Make and by the tests
clean:
	rm -f crawler
	rm -f *~ *.o
	rm -rf *.dSYM
	rm -f testing.out
