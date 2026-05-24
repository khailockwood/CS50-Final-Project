/* 
 * counters.c - CS50 module to support a set of counters
 *
 * see counters.h for more information.
 *
 * David Kotz, April 2016, 2017, 2019, 2021
 * Xia Zhou, July 2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "counters.h"
#include "mem.h"

/**************** file-local global variables ****************/
/* none */

/**************** local types ****************/
typedef struct countersnode {
  int key;                    // search key for this counter
  int count;                  // value of this counter
  struct countersnode *next;  // pointer to next counter in list
} countersnode_t;

/**************** global types ****************/
typedef struct counters {
  struct countersnode *head;  // head of the counters (UNSORTED list)
} counters_t;

/**************** global functions ****************/
/* that is, visible outside this file */
/* see counters.h for comments about exported functions */

/**************** local functions ****************/
/* not visible outside this file */
static countersnode_t* countersnode_new(const int key);

/**************** counters_new() ****************/
/* see counters.h for description */
counters_t*
counters_new(void)
{
  counters_t* ctrs = mem_malloc(sizeof(counters_t));

  if (ctrs == NULL) {
    return NULL;              // error allocating counters
  } else {
    // initialize contents of counters structure
    ctrs->head = NULL;
    return ctrs;
  }
}

/**************** counters_add() ****************/
/* see counters.h for description */
int
counters_add(counters_t* ctrs, const int key)
{
  if (ctrs == NULL || key < 0) {
    return 0;                 // bad ctrs or bad key
  }

  if (ctrs->head == NULL) {   // empty list: add new counter
    ctrs->head = countersnode_new(key);
    return 1;                 // return count value
  }           

  // non-empty list, look for the counter in the list
  // it is an unsorted list, so scan until we find it, or reach the end
  countersnode_t* prev = ctrs->head; // which we know not to be NULL
  for (countersnode_t* node = prev; node != NULL; node = prev->next)
    {
      if (key == node->key) {
        // counter exists: increment it
        node->count++;
        return node->count; 
      }
      prev = node;
    }
  // not on the list; insert new counter node at end of list
  prev->next = countersnode_new(key);
  return 1; 

#ifdef MEMTEST
  mem_report(stdout, "After counters_add");
#endif
}


/**************** countersnode_new ****************/
/* Allocate and initialize a countersnode */
static countersnode_t*  // not visible outside this file
countersnode_new(const int key)
{
  countersnode_t* node = mem_malloc(sizeof(countersnode_t));

  if (node == NULL) {
    // error allocating memory for node; return error
    return NULL;
  } else {
    node->key = key;
    node->count = 1;
    node->next = NULL;
    return node;
  }
}

/**************** counters_get() ****************/
/* see counters.h for description */
int
counters_get(counters_t* ctrs, const int key)
{
  if (ctrs == NULL || key < 0) {
    return 0;                 // bad ctrs or bad key
  }

  // scan the counters
  for (countersnode_t* node = ctrs->head; node != NULL; node = node->next) {
    if (key == node->key) {
      return node->count;     // found!  return the node's count
    }
  }
  return 0; // not found!
}

/**************** counters_set() ****************/
/* see counters.h for description */
bool
counters_set(counters_t* ctrs, const int key, const int count)
{
  if (ctrs == NULL || key < 0 || count < 0) {
    return false;             // bad parameters
  }

  // scan the counters
  countersnode_t* prev = ctrs->head;
  for (countersnode_t* node = prev; node != NULL; node = prev->next) {
    if (key == node->key) {
      node->count = count;    // found!  update the node's count
      return true;
    }
    prev = node; 
  }

  // not found: make a new node and insert to the end of list
  countersnode_t* new = countersnode_new(key);
  if (new == NULL) {
    return false;
  }
  new->count = count; 
  if (prev == NULL) {         // if it's an empty list
    ctrs->head = new; 
  } else {
    prev->next = new; 
  }  

  /* OLD VERSION: insert at head of list.
     node->count = count;
     node->next = ctrs->head;
     ctrs->head = node;
  */
  return true;
}

/**************** counters_print() ****************/
/* see counters.h for description */
void
counters_print(counters_t* ctrs, FILE* fp)
{
  if (fp != NULL) {
    if (ctrs == NULL) {
      fputs("(null)", fp);
      return;                 // bad counters
    } else {
      // scan the counters
      fputc('{', fp);
      for (countersnode_t* node = ctrs->head; node != NULL; node = node->next) {
        // print the current node
        fprintf(fp, "%d=%d, ", node->key, node->count);
      }
      fputc('}', fp);
    }
  }
}

/**************** counters_iterate() ****************/
/* see counters.h for description */
void
counters_iterate(counters_t* ctrs, void* arg,
                 void (*itemfunc)(void* arg, const int key, const int count))
{
  if (ctrs != NULL && itemfunc != NULL) {
    // scan the counters
    for (countersnode_t* node = ctrs->head; node != NULL; node = node->next) {
      (*itemfunc)(arg, node->key, node->count);
    }
  }
}

/**************** counters_delete() ****************/
/* see counters.h for description */
void 
counters_delete(counters_t* ctrs)
{
  if (ctrs != NULL) {
    countersnode_t* node = ctrs->head;
    while (node != NULL) {
      countersnode_t* next = node->next; // remember what's next
      mem_free(node);                    // delete the current node
      node = next;                       // move on to next
    }
    // delete the overall structure
    mem_free(ctrs);
  }
#ifdef MEMTEST
  mem_report(stdout, "End of counters_delete");
#endif
}
