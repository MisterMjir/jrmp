#include "io.h"
#include <stdio.h>
#include <string.h>
#include "cleanup_stack.h"
#include <math.h>

/*
 * NOTES
 *
 * Yes I'm using macros, it reduces a lot of repeated code
 * If for some reason you need to calculate stack size, all the variables are declared
 * at the top of the function but there is also a struct CleanupStack which the macros
 * are handling
 */

#define LT_LAYERS(lt) (lt >> 24)
#define LT_TILES(lt) (lt & 0x00FFFFFF)

/* Exit on failure */
#define ERROR_EXIT {cleanup(&cs); return -1; }

/* Check data parameter */
#define DATA_CHECK \
  if (!data) return -1; \
  if (!data->file) return -1;

/* Start and end of functions */
#define FN_START DATA_CHECK; struct CleanupStack cs; if (cleanup_create(&cs)) return -1;
#define FN_EXIT cleanup(&cs); return 0;

/* Push and pop from the cleanup stack, FN_EXIT will end up popping everything, really don't need to pop */
#define PUSH(type, var) cleanup_push(&cs, type, var)
#define POP             cleanup_pop(&cs)

/* Read and write */
#define IO(mode, data, count, file) if (f##mode(data, sizeof(*data), count, file) != count) ERROR_EXIT;

/* Write the header */
static int data_to_files_h(struct JRMP_Data *data)
{
  FILE    *file;
  uint32_t temp[3]; /* lt_num, tiles_x, and tiles_y */

  FN_START;

  if (!(file = fopen(".jrmph", "wb"))) return -1;
  PUSH('f', file);

  if (JRMP_data_block_seek(data, "MAPINFO")) ERROR_EXIT;
  IO(read, temp, 3, data->file);
  IO(write, temp, 3, file);

  FN_EXIT;
}

/* Write the layers and tiles */
#define LAYER_SIZE (sizeof(uint8_t) + sizeof(float))
#define TILE_SIZE  (sizeof(uint16_t) + sizeof(uint8_t))
static int data_to_files_t(struct JRMP_Data *data)
{
  FILE    *table;          /* Stores the names of the layer files */
  uint32_t lt_num;         /* Layer and tile amount */
  uint64_t layer_current;  /* Position of the current layer */
  uint64_t tile_current;   /* Position of the current tile */
  uint8_t  layer_num;      /* Amount of layers */
  char     layer_name[10]; /* Name of a layer */
  FILE    *layer;          /* Stores layer properties and tile array */
  uint8_t  layer_id;       /* Id of a layer */
  float    layer_parallax; /* Parallax of a layer */
  uint16_t tile_idt;       /* Tile's idt */
  int8_t   tile_flags;     /* Tile's flags */
  uint16_t tile_write_num; /* How many tiles to write */

  FN_START;

  if (!(table = fopen(".jrmplt", "wb"))) ERROR_EXIT;
  PUSH('f', table);

  /* Get the total amount of layers and tiles */
  if (JRMP_data_block_seek(data, "MAPINFO")) ERROR_EXIT;
  IO(read, &lt_num, 1, data->file);

  /* Set the offsets */
  if (JRMP_data_block_seek(data, "TILES  ")) ERROR_EXIT;
  if ((layer_current = ftell(data->file)) == -1L) ERROR_EXIT;
  if ((tile_current  = ftell(data->file)) == -1L) ERROR_EXIT;
  tile_current += LT_LAYERS(lt_num) * LAYER_SIZE;

  /* Loop through the layers */
  for (uint8_t i = 0; i < LT_LAYERS(lt_num); ++i) {
    snprintf(layer_name, 10, "%d.jrmpl", i);

    /* Write the name into the table */
    if (fputs(layer_name, table) == EOF) ERROR_EXIT;
    if (putc('\n', table) != '\n') ERROR_EXIT;

    /* Create the layer file */
    if (!(layer = fopen(layer_name, "wb"))) ERROR_EXIT;
    PUSH('f', layer);

    /* Write the id and parallax */
    fseek(data->file, layer_current, SEEK_SET);
    IO(read, &layer_id,       1, data->file);
    IO(read, &layer_parallax, 1, data->file);
    layer_current += LAYER_SIZE; 

    IO(write, &layer_id,       1, layer);
    IO(write, &layer_parallax, 1, layer);

    /* Write the tiles */
    /* total_tiles * (1 / 2^p)^2 or (1 / 2^2p) */
    for (uint64_t j = 0; j < (uint64_t) (LT_TILES(lt_num) / pow(2, layer_parallax * 2)); ++j) {
      /* Get the tile */
      fseek(data->file, tile_current, SEEK_SET);
      IO(read, &tile_idt, 1, data->file);
      IO(read, &tile_flags, 1, data->file);

      if (tile_idt == 0xFFFF) break; /* Shouldn't hit the terminating tile, but just in case */

      /* Check if the texture is 0 */
      if ((tile_idt & 0x000F) == 0) {
        tile_write_num = (tile_idt >> 4);
        tile_idt = 0x0001;
        tile_flags = 0;
      }
      else {
        tile_write_num = 1;
      }

      /* Write the tile(s) */
      for (uint16_t k = 0; k < tile_write_num; ++k) {
        IO(write, &tile_idt,   1, layer);
        IO(write, &tile_flags, 1, layer);
      }

      j += tile_write_num - 1;
      tile_current += TILE_SIZE;
    }

    /* Write the terminating layer tile */
    tile_idt = 0xFFFF; tile_flags = 0;
    IO(write, &tile_idt,   1, layer);
    IO(write, &tile_flags, 1, layer);

    POP; /* Pop the layer file */
  }

  FN_EXIT;
}

/*
 * JRMP_data_to_files
 * @desc
 * @param
 * @return
 */
int JRMP_data_to_files(struct JRMP_Data *data)
{
  DATA_CHECK;

  if (data_to_files_h(data)) return -1; /* Map info (header) */
  if (data_to_files_t(data)) return -1; /* Tile info */

  return 0;
}

int JRMP_files_to_data(struct JRMP_Data *data)
{
  return 0;
}
