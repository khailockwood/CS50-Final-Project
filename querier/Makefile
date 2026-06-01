# Gift Christian, 5/30/2026, CS50 TSE Querier
# builds the querier executable from querier.c, linking against the common library and the libcs50 library one directory up

# C is where the common module lives, L is where the cs50 library lives
C = ../common
L = ../libcs50

# compiler flags
CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$C -I$L $(FLAGS)

# gcc to compile
CC = gcc

MAKE = make

# libraries to link against
LIBS = $C/common.a $L/libcs50.a

# PROG is the program we're building, OBJS is the object file it's built from
PROG = querier
OBJS = querier.o

# target commands
.PHONY: all test valgrind clean

# default target.. build the program
all: $(PROG)

# link the object file and the libraries into the final executable
$(PROG): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $^ -o $@

# querier.o depends on all these headers, so it rebuilds if any of them change
querier.o: $C/index.h $C/pagedir.h $C/word.h $L/counters.h $L/file.h $L/mem.h

# run the testing script
test: $(PROG)
	bash -v testing.sh

# run the program under valgrind to check for leaks
valgrind: $(PROG)
	( \
	echo "cat and dog"; \
	echo "cat or emu"; \
	echo "dog or dog"; \
	echo "and dog"; \
	echo "bad!char"; \
	echo "notintheindex"; \
	) | valgrind --leak-check=full --show-leak-kinds=all \
	./$(PROG) test-data/tiny test-data/tiny.index

# tidy up
clean:
	rm -f core
	rm -f $(PROG) *.o *~
	rm -f testing.out
