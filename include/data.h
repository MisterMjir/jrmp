#ifndef JRMP_DATA_H
#define JRMP_DATA_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * DATA
 *
 * This struct keeps track of the blocks and offsets in a .jrmp file,
 * and also keeps a FILE pointer to that .jrmp file
 */

struct JRMP_Data {
  FILE     *file; /* The .jrmp file */

  uint8_t   blocks_num; /* Number of blocks */
  char    (*blocks_name)[8]; /* Block names */
  uint64_t *blocks_offset; /* Block offsets */
};

#define JRMP_DATA_INIT {NULL, 0, NULL, NULL}

/*
 * FUNCTIONS
 *
 * create     | Initializes data
 * destory    | Frees/cleans up data
 * block_seek | Goes to a block for read/write
 * read       | Reads data
 * write      | Writes data
 */
int JRMP_data_create(struct JRMP_Data *data, const char *fpath);
int JRMP_data_destory(struct JRMP_Data *data);

int JRMP_data_block_seek(struct JRMP_Data *data, char block[8]);

#endif
