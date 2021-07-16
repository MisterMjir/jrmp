#ifndef JRMP_CLEANUP_STACK_H
#define JRMP_CLEANUP_STACK_H

#include <stdlib.h>

/*
 * CLEANUP STACK
 *
 * When opening files and malloc'ing memory during file
 * io, the things that need to be cleaned up stack up,
 * so this will automate cleaning it up
 *
 * Example
 *  - 1st I just return -1 on any error
 *  - After I successfully open a file, I need to close it on error
 *  - After mallocing a buffer, I need to free the buffer and close the file
 *  - After another file, close that file, free the buffer, close the first file
 */

struct CleanupStack {
  char   *format;
  void  **pointers;
  size_t  allocated;
  size_t  num;
};


int  cleanup_create (struct CleanupStack *);
void cleanup_destory(struct CleanupStack *);
int  cleanup_push   (struct CleanupStack *, char, void *);
int  cleanup_pop    (struct CleanupStack *);

void cleanup(struct CleanupStack *);

#endif
