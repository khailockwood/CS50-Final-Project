# Makefile for the CS50 'common' library
#
# Builds and archives the code shared by the Crawler, Indexer, and Querier
# into common.a. For now this is just the 'pagedir' module.
#.

L = ../libcs50

OBJS = pagedir.o
LIB = common.a

CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$L $(FLAGS)
CC = gcc
MAKE = make

# build the library by archiving the object files
$(LIB): $(OBJS)
	ar cr $(LIB) $(OBJS)

# object-file dependencies on headers
pagedir.o: pagedir.h $L/webpage.h $L/mem.h

.PHONY: clean

clean:
	rm -f $(LIB) *~ *.o
	rm -rf *.dSYM
