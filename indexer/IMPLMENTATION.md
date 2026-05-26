CS50 TSE Indexer

Implementation Spec
In this document we reference the Requirements Specification and Design Specification and focus on the implementation-specific decisions. The knowledge unit noted that an implementation spec may include many topics; not all are relevant to the TSE or the Indexer. Here we focus on the core subset:

Data structures
Control flow: pseudo code for overall flow, and for each of the functions
Detailed function prototypes and their parameters
Error handling and recovery
Testing plan

Data structures
We use one primary data structure: an index, which is an inverted index mapping each word to a set of (docID, count) pairs. The index is implemented as a hashtable keyed by word, where each item is a counters keyed by docID and storing the number of occurrences of that word in that document.
Both the hashtable and counters start empty. The size of the hashtable (slots) is impossible to determine in advance when building a fresh index, so we use 500. When loading an index from a file, we count the number of lines in the file (one word per line) and use that count to choose an appropriate hashtable size.
Control flow
The Indexer is implemented in one file indexer.c, with four functions.
main
The main function simply calls parseArgs and indexBuild, then exits zero.
parseArgs
Given arguments from the command line, extract them into the function parameters; return only if successful.
for pageDirectory, verify it is a directory produced by the Crawler by calling pagedir_validate()
for indexFilename, verify it can be opened for writing
if any trouble is found, print an error to stderr and exit non-zero.
indexBuild
Do the real work of building the in-memory index from documents in pageDirectory, then write that index to indexFilename. Pseudocode:
create a new 'index' object
loop over document ID numbers, counting from 1
	load a webpage from the file 'pageDirectory/id'
	if successful,
		pass the webpage and docID to indexPage
		delete the webpage
	else
		break out of the loop
write the index to indexFilename
delete the index

indexPage
This function implements the indexPage mentioned in the design. Given a webpage and a docID, scan the page to extract every word, ignoring words shorter than three characters, normalize each word, and add the (word, docID) occurrence into the index. Pseudocode:
while there is another word in the page
	if the word has length >= 3
		normalize the word (convert to lower case)
		look up the word in the index
			if not found, add it (with a new counters object)
		increment the counter for this docID
	free the word

Other modules
pagedir
We extend the re-usable module pagedir.c to support the Indexer's need to validate a pageDirectory (i.e., confirm it was produced by the Crawler) and to load a webpage from a file in that directory. We chose to extend this module, in ../common, so that all knowledge about pageDirectory structure remains in one place... anticipating future use by the Querier.
Pseudocode for pagedir_validate:
construct the pathname for the .crawler file in that directory
open the file for reading; on error, return false.
close the file and return true.

Pseudocode for pagedir_load:
construct the pathname for the page file in pageDirectory
open that file for reading; if NULL, return NULL.
read the URL from the first line
read the depth from the second line
read the HTML from the rest of the file
close the file
construct a new webpage_t with that URL, depth, and HTML
return the webpage

index
We create a new re-usable module index.c in ../common to provide an abstract index_t data type, with functions to create, delete, search, save, and load an index. The module is essentially a wrapper around a hashtable of counters objects, with extra knowledge about the index file format.
Pseudocode for index_save:
open the indexFilename for writing
for each (word, counters) pair in the hashtable
	print the word to the file
	for each (docID, count) pair in the counters
		print " docID count" to the file
	print a newline
close the file

Pseudocode for index_load:
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

word
We create a new tiny module word.c in ../common to provide a single function normalizeWord, which converts every letter in a string to lower case (in place). We chose this as a separate module because it is shared between the Indexer and the future Querier.
libcs50
We leverage the modules of libcs50, most notably hashtable, counters, webpage, and file. See that directory for module interfaces. The webpage module allows us to represent pages as webpage_t objects and to scan a (loaded) page for words via webpage_getNextWord; in that regard, it serves as the word scanner described in the design. The file module provides convenient functions for reading lines and counting lines in a file.
Function prototypes
indexer
Detailed descriptions of each function's interface is provided as a paragraph comment prior to each function's implementation in indexer.c and is not repeated here.
int main(const int argc, char* argv[]);
static void parseArgs(const int argc, char* argv[],
                      char** pageDirectory, char** indexFilename);
static void indexBuild(const char* pageDirectory, const char* indexFilename);
static void indexPage(index_t* index, webpage_t* page, const int docID);

pagedir
Detailed descriptions of each function's interface is provided as a paragraph comment prior to each function's declaration in pagedir.h and is not repeated here.
bool pagedir_init(const char* pageDirectory);
void pagedir_save(const webpage_t* page, const char* pageDirectory, const int docID);
bool pagedir_validate(const char* pageDirectory);
webpage_t* pagedir_load(const char* pageDirectory, const int docID);

index
Detailed descriptions of each function's interface is provided as a paragraph comment prior to each function's declaration in index.h and is not repeated here.
index_t* index_new(const int numSlots);
void index_delete(index_t* index);
counters_t* index_find(index_t* index, const char* word);
bool index_insert(index_t* index, const char* word, counters_t* counters);
bool index_save(const index_t* index, const char* indexFilename);
index_t* index_load(const char* indexFilename);

word
void normalizeWord(char* word);

Error handling and recovery
All the command-line parameters are rigorously checked before any data structures are allocated or work begins; problems result in a message printed to stderr and a non-zero exit status.
Out-of-memory errors are handled by variants of the mem_assert functions, which result in a message printed to stderr and a non-zero exit status. We anticipate out-of-memory errors to be rare and thus allow the program to crash (cleanly) in this way.
All code uses defensive-programming tactics to catch and exit (using variants of the mem_assert functions), e.g., if a function receives bad parameters.
That said, certain errors are caught and handled internally: for example, pagedir_validate returns false if the directory is not a Crawler-produced directory, allowing the Indexer to decide what to do; pagedir_load returns NULL when a page file cannot be opened, which the Indexer uses as the signal to stop iterating over document IDs.
Testing plan
Here is an implementation-specific testing plan.
Unit testing
The indextest program serves as a unit test for the index module: it reads an index file into the internal index_t data structure using index_load, then writes the index out to a new file using index_save. The other modules (pagedir and word) are tiny; they could be tested using small C drivers, but it is likely sufficient to observe their behavior during the system test.
Regression testing
The indexer's output file can be compared against a known-good index file using the provided indexcmp tool, which handles the fact that lines and (docID, count) pairs may appear in any order. For routine regression tests, we run the indexer on the letters pageDirectory at moderate depth and compare the resulting index file against a saved reference.
Integration/system testing
We write a script testing.sh that invokes the indexer (and indextest) several times, with a variety of command-line arguments. First, a sequence of invocations with erroneous arguments, testing each of the possible mistakes that can be made. Second, a run with valgrind over both indexer and indextest on a moderate-sized test case (such as toscrape at depth 1), to verify no memory errors or leaks. Third, runs of indexer over several CS50 pageDirectories (such as letters at depths 0, 1, 2; toscrape at depth 1; wikipedia at depth 1), followed by indextest on each resulting index file, and finally indexcmp to verify that the round-tripped index is equivalent to the original.
Run that script with bash -v testing.sh so the output of indexer and indextest is intermixed with the commands used to invoke them. Verify correct behavior by studying the output, and by sampling the entries in the resulting index files.