#include "cleanup_stack.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INIT_ALLOC 8
#define ALLOC_FACTOR 2

/*
 * @desc
 *   Allocates more room for the stack
 */
static int cleanup_allocate(struct CleanupStack *cs, size_t num)
{
  char  *format_new;
  void **pointers_new;
  
  if (num <= cs->num) {
    return 1;
  }

  /* Format allocation */
  if (!(format_new = malloc((num + 1) * sizeof(char)))) {
    return -1;
  }
  memcpy(format_new, cs->format, (num + 1) * sizeof(char));
  free(cs->format);
  cs->format = format_new;

  /* Pointers allocation */
  if (!(pointers_new = malloc(num * sizeof(void *)))) {
    free(format_new);
    return -1;
  }
  memcpy(pointers_new, cs->pointers, num * sizeof(void *));
  free(cs->pointers);
  cs->pointers = pointers_new;

  cs->allocated = num;

  return 0;
}

/*
 * @desc
 *   Creates/initializes a new stack
 */
int cleanup_create(struct CleanupStack *cs)
{
  /* +1 for '\0' */
  if (!(cs->format = malloc(sizeof(char) * (INIT_ALLOC + 1)))) {
    return -1;
  }
  cs->format[0] = '\0';

  if (!(cs->pointers = malloc(sizeof(void *) * (INIT_ALLOC)))) {
    free(cs->format);
    return -1;
  }

  cs->allocated = INIT_ALLOC;
  cs->num = 0;

  return 0;
}

/*
 * @desc
 *   Destroys/quits/cleanups a stack
 */
void cleanup_destory(struct CleanupStack *cs)
{
  free(cs->format);
  free(cs->pointers);
}

/*
 * @desc
 *   Pushes onto the stack
 */
int cleanup_push(struct CleanupStack *cs, char f, void *p)
{
  if (cs->num == cs->allocated) {
    if (!cleanup_allocate(cs, cs->allocated * ALLOC_FACTOR)) {
      return -1;
    }
  }

  cs->format[cs->num] = f;
  cs->pointers[cs->num] = p;

  ++cs->num;

  return 0;
}

#define CLEANUP_SWITCH \
  switch (cs->format[i]) { \
    case 'f': \
    { \
      FILE *f = cs->pointers[i]; \
      fclose(f); \
      break; \
    } \
    case 'p': \
    { \
      void *p = cs->pointers[i]; \
      free(p); \
      break; \
    } \
  }

/*
 * @desc
 *   Pops from the stack, and frees it
 */
int cleanup_pop(struct CleanupStack *cs)
{
  if (cs->num > 0) --cs->num;
  else return -1;

  /* Cleanup the pointer */
  size_t i = cs->num;
  CLEANUP_SWITCH;

  /* Remove the format */
  cs->format[cs->num] = '\0';

  return 0;
}

/*
 * @desc
 *   Frees files and pointers
 */
void cleanup(struct CleanupStack *cs)
{
  size_t i = 0;
  while (cs->format[i] != '\0') {
    CLEANUP_SWITCH;

    ++i;
  }

  cleanup_destory(cs);
}
