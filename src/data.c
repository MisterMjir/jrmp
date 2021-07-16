#include "data.h"
#include <string.h>
#include <stdlib.h>

#define IO(mode, var, count) if (f##mode(var, sizeof(*var), count, data->file) != count) return -1;

#define DATA_CHECK(data) \
  if (!data) return -1; \
  if (!data->file) return -1;

/*
 * @desc
 *   NOTE: Don't double create data
 *   NOTE: Can overwrite file if it exists
 */
int JRMP_data_create(struct JRMP_Data *data, const char *fpath)
{
  if (!data) return -1;
  if (!fpath) return -1;

  if (!(data->file = fopen(fpath, "r+"))) return -1;

  /* Read the blocks */
  IO(read, &data->blocks_num, 1);
  if (!(data->blocks_name   = malloc(data->blocks_num * sizeof(*data->blocks_name  )))) return -1;
  if (!(data->blocks_offset = malloc(data->blocks_num * sizeof(*data->blocks_offset)))) return -1;
  IO(read, data->blocks_name,   data->blocks_num);
  IO(read, data->blocks_offset, data->blocks_num);

  return 0;
}

/*
 *
 */
int JRMP_data_destory(struct JRMP_Data *data)
{
  DATA_CHECK(data);

  free(data->blocks_name);
  free(data->blocks_offset);
  fclose(data->file);

  return 0;
}

/*
 * 
 */
int JRMP_data_block_seek(struct JRMP_Data *data, char block[8])
{
  DATA_CHECK(data);

  uint8_t i = 0;

  while (strcmp(data->blocks_name[i], block)) {
    if (++i > data->blocks_num) return -1;
  }

  return fseek(data->file, data->blocks_offset[i], SEEK_SET);
}
