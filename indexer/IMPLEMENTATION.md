# CS50 TSE Indexer
## Implementation Spec

In this document we reference the [Requirements Specification](REQUIREMENTS.md) and [Design Specification](DESIGN.md) and focus on the implementation-specific decisions.
The knowledge unit noted that an [implementation spec](https://github.com/CS50DartmouthFA2025/home/blob/main/knowledge/units/design.md#implementation-spec) may include many topics; not all are relevant to the TSE or the Indexer.
Here we focus on the core subset:

-  Data structures
-  Control flow: pseudo code for overall flow, and for each of the functions
-  Detailed function prototypes and their parameters
-  Error handling and recovery
-  Testing plan

## Data structures

We use one primary data structure: an *index*, which is an inverted index mapping each word to a set of *(docID, count)* pairs.
The index is implemented as a `hashtable` keyed by word, where each item is a `counters` keyed by docID and storing the number of occurrences of that word in that document.
Both the hashtable and counters start empty.
The size of the hashtable (slots) is impossible to determine in advance when building a fresh index, so we use 500.
When loading an index from a file, we count the number of lines in the file (one word per line) and use that count to choose an appropriate hashtable size.

## Control flow

The Indexer is implemented in one file `indexer.c`, with three functions.

### main

The `main` function validates the arguments, creates the index, calls indexBuild, saves with index_save, and frees with index_delete, then exits. 

### indexBuild
Given an index and pageDirectory, loop docIDs from 1, load each page,
pass it to indexPage, delete it, stop when load returns NULL 

Pseudocode:

loop over document ID numbers, counting from 1
    load a webpage from the file 'pageDirectory/id'
    if successful,
        pass the webpage and docID to indexPage
        delete the webpage
    else
        break out of the loop

### indexPage

This function implements the *indexPage* mentioned in the design.
Given a `webpage` and a `docID`, scan the page to extract every word, ignoring words shorter than three characters, normalize each word, and add the *(word, docID)* occurrence into the index.
Pseudocode:

	while there is another word in the page
		if the word has length >= 3
			normalize the word (convert to lower case)
			look up the word in the index
				if not found, add it (with a new counters object)
			increment the counter for this docID
		free the word

## Other modules

### pagedir

We extend the re-usable module `pagedir.c` to support the Indexer's need to validate a pageDirectory (i.e., confirm it was produced by the Crawler) and to load a webpage from a file in that directory.
We chose to extend this module, in `../common`, so that all knowledge about pageDirectory structure remains in one place... anticipating future use by the Querier.

Pseudocode for `pagedir_validate`:

	construct the pathname for the .crawler file in that directory
	open the file for reading; on error, return false.
	close the file and return true.

Pseudocode for `pagedir_load`:

	construct the pathname for the page file in pageDirectory
	open that file for reading; if NULL, return NULL.
	read the URL from the first line
	read the depth from the second line
	read the HTML from the rest of the file
	close the file
	construct a new webpage_t with that URL, depth, and HTML
	return the webpage

### index

We create a new re-usable module `index.c` in `../common` to provide an abstract `index_t` data type, with functions to create, delete, search, save, and load an index.
The module is essentially a wrapper around a hashtable of counters objects, with extra knowledge about the index file format.

Pseudocode for `index_save`:

	open the indexFilename for writing
	for each (word, counters) pair in the hashtable
		print the word to the file
		for each (docID, count) pair in the counters
			print " docID count" to the file
		print a newline
	close the file

Pseudocode for `index_load`:

	count the number of lines in the file
	create a new index sized appropriately for that count
	for each line in the file,
		read a word
		create a new counters object for that word
		insert (word, counters) into the index
		loop reading "docID count" pairs with fscanf
			set counters[docID] = count
	close the file
	return the index

### word

We create a new tiny module `word.c` in `../common` to provide a single function `normalizeWord`, which converts every letter in a string to lower case (in place).
We chose this as a separate module because it is shared between the Indexer and the future Querier.

### libcs50

We leverage the modules of libcs50, most notably `hashtable`, `counters`, `webpage`, and `file`.
See that directory for module interfaces.
The `webpage` module allows us to represent pages as `webpage_t` objects and to scan a (loaded) page for words via `webpage_getNextWord`; in that regard, it serves as the word scanner described in the design.
The `file` module provides convenient functions for reading lines and counting lines in a file.

## Function prototypes

### indexer

Detailed descriptions of each function's interface is provided as a paragraph comment prior to each function's implementation in `indexer.c` and is not repeated here.

```c
int main(const int argc, char* argv[]);
static void indexBuild(index_t* index, const char* pageDirectory);
static void indexPage(index_t* index, webpage_t* page, const int docID);
```

### pagedir

Detailed descriptions of each function's interface is provided as a paragraph comment prior to each function's declaration in `pagedir.h` and is not repeated here.

```c
bool pagedir_init(const char* pageDirectory);
void pagedir_save(const webpage_t* page, const char* pageDirectory, const int docID);
bool pagedir_validate(const char* pageDirectory);
webpage_t* pagedir_load(const char* pageDirectory, const int docID);
```

### index

Detailed descriptions of each function's interface is provided as a paragraph comment prior to each function's declaration in `index.h` and is not repeated here.

```c
index_t* index_new(const int numSlots);
void index_delete(index_t* index);
counters_t* index_find(index_t* index, const char* word);
bool index_insert(index_t* index, const char* word, counters_t* counters);
bool index_save(const index_t* index, const char* indexFilename);
index_t* index_load(const char* indexFilename);
```

### word

```c
void normalizeWord(char* word);
```

## Error handling and recovery

All the command-line parameters are rigorously checked before any data structures are allocated or work begins; problems result in a message printed to stderr and a non-zero exit status.

Out-of-memory errors are handled by variants of the `mem_assert` functions, which result in a message printed to stderr and a non-zero exit status.
We anticipate out-of-memory errors to be rare and thus allow the program to crash (cleanly) in this way.

All code uses defensive-programming tactics to catch and exit (using variants of the `mem_assert` functions), e.g., if a function receives bad parameters.

That said, certain errors are caught and handled internally: for example, `pagedir_validate` returns false if the directory is not a Crawler-produced directory, allowing the Indexer to decide what to do; `pagedir_load` returns NULL when a page file cannot be opened, which the Indexer uses as the signal to stop iterating over document IDs.

## Testing plan

Here is an implementation-specific testing plan.

### Unit testing

The `indextest` program serves as a unit test for the `index` module: it reads an index file into the internal `index_t` data structure using `index_load`, then writes the index back out to a new file using `index_save`.
Comparing the two files confirms the load/save round-trip preserves the index; because the index file format permits the lines (and the *(docID, count)* pairs within a line) to appear in any order, we sort both files before comparing them with `diff`.
The other modules, `pagedir` and `word`, are small enough that their behavior is exercised and observed during the integration test rather than through separate unit drivers.

### Regression testing

The shared CS50 corpus and the `indexcmp` tool were not available on our development server, so rather than comparing against a saved reference index we validate the indexer's output against the known content of a small pageDirectory.
We use the `tiny` pageDirectory (three documents), whose word counts can be verified by inspection, and re-run `testing.sh` after any change to confirm the output is unchanged.

### Integration/system testing

We write a script `testing.sh` that invokes the indexer and indextest several times.

First, a sequence of invocations with erroneous arguments — no arguments, one argument, too many arguments, a non-existent pageDirectory, a directory that is not a Crawler-produced directory, and an unwritable indexFilename — each followed by printing the exit status, to confirm the program prints an error to stderr and exits non-zero in every case.

Second, a valid run of the indexer over the `tiny` pageDirectory, followed by `indextest` on the resulting index file and a sorted `diff` between the original and round-tripped index files, to confirm the two are equivalent.

Third, runs of both `indexer` and `indextest` under valgrind, to verify there are no memory errors or leaks.

We run the script with `bash -v testing.sh` so each command is echoed alongside its output, and we save that output to `testing.out`.
We verify correct behavior by studying that output and by sampling entries in the resulting index file.
