# Makefile for the 'indexer' directory of the CS50 Tiny Search Engine
#
# Builds the indexer and indextest programs.
#
# Lyndon Huang, Khai Lockwood, Gift Christian
# 6/1/26

L = ../libcs50
C = ../common

LIBS = $C/common.a $L/libcs50.a

CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$L -I$C
CC = gcc

.PHONY: all clean

all: indexer indextest

indexer: indexer.o $(LIBS)
	$(CC) $(CFLAGS) $^ -o $@

indextest: indextest.o $(LIBS)
	$(CC) $(CFLAGS) $^ -o $@

indexer.o: $C/index.h $C/pagedir.h $C/word.h $L/webpage.h $L/counters.h
indextest.o: $C/index.h

clean:
	rm -f *~ *.o
	rm -rf *.dSYM
	rm -f indexer indextest
	rm -f core
