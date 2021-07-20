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
#define BLK_NAME_SIZE 8
#define SCRIPT_SIZE 4096
#define LAYER_NAME_SIZE 10

/* Exit on failure */
#define ERROR_EXIT {cleanup(&cs); return -1;}

/* Check data parameter */
#define DATA_CHECK \
  if (!data) return -1; \
  if (!data->file) return -1;

/* Start and end of functions */
#define FN_START struct CleanupStack cs; if (cleanup_create(&cs)) return -1;
#define FN_EXIT cleanup(&cs); return 0;

/* Push and pop from the cleanup stack, FN_EXIT will end up popping everything, really don't need to pop */
#define PUSH(type, var) cleanup_push(&cs, type, var)
#define POP             cleanup_pop(&cs)

/* File operations with checking */
#define IO(mode, data, count, file) if (f##mode(data, sizeof(*data), count, file) != count) ERROR_EXIT;
#define SEEK(file, offset) if (fseek(file, offset, SEEK_SET)) ERROR_EXIT;
#define TELL(var, file) if ((var = ftell(file)) == -1L) ERROR_EXIT;

/*
 * ========================================
 * DATA TO FILES
 * ========================================
 */

/* Write the header */
static int data_to_files_h(struct JRMP_Data *data)
{
  FILE    *file;
  uint32_t temp[3]; /* lt_num, tiles_x, and tiles_y */

  DATA_CHECK;
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
#define TILE_SIZE  (sizeof(uint16_t))
static int data_to_files_t(struct JRMP_Data *data)
{
  FILE    *table;                       /* Stores the names of the layer files */
  uint32_t lt_num;                      /* Layer and tile amount */
  uint64_t layer_current;               /* Position of the current layer */
  uint64_t tile_current;                /* Position of the current tile */
  uint8_t  layer_num;                   /* Amount of layers */
  char     layer_name[LAYER_NAME_SIZE]; /* Name of a layer */
  FILE    *layer;                       /* Stores layer properties and tile array */
  uint8_t  layer_id;                    /* Id of a layer */
  float    layer_parallax;              /* Parallax of a layer */
  uint16_t tile_idt;                    /* Tile's idt */
  uint16_t tile_write_num;              /* How many tiles to write */

  DATA_CHECK;
  FN_START;

  if (!(table = fopen(".jrmplt", "wb"))) ERROR_EXIT;
  PUSH('f', table);

  /* Get the total amount of layers and tiles */
  if (JRMP_data_block_seek(data, "MAPINFO")) ERROR_EXIT;
  IO(read, &lt_num, 1, data->file);

  /* Set the offsets */
  if (JRMP_data_block_seek(data, "TILES  ")) ERROR_EXIT;
  TELL(layer_current, data->file);
  TELL(tile_current, data->file);
  tile_current += LT_LAYERS(lt_num) * LAYER_SIZE;

  /* Loop through the layers */
  for (uint8_t i = 0; i < LT_LAYERS(lt_num); ++i) {
    snprintf(layer_name, LAYER_NAME_SIZE, "%d.jrmpl", i);

    /* Write the name into the table */
    if (fputs(layer_name, table) == EOF) ERROR_EXIT;
    if (putc('\n', table) != '\n') ERROR_EXIT;

    /* Create the layer file */
    if (!(layer = fopen(layer_name, "wb"))) ERROR_EXIT;
    PUSH('f', layer);

    /* Write the id and parallax */
    SEEK(data->file, layer_current);
    IO(read, &layer_id,       1, data->file);
    IO(read, &layer_parallax, 1, data->file);
    layer_current += LAYER_SIZE; 

    IO(write, &layer_id,       1, layer);
    IO(write, &layer_parallax, 1, layer);

    /* Write the tiles */
    /* total_tiles * (1 / 2^p)^2 or (1 / 2^2p) */
    for (uint64_t j = 0; j < (uint64_t) (LT_TILES(lt_num) / pow(2, layer_parallax * 2)); ++j) {
      /* Get the tile */
      SEEK(data->file, tile_current);
      IO(read, &tile_idt, 1, data->file);

      if (tile_idt == 0xFFFF) break; /* Shouldn't hit the terminating tile, but just in case */

      /* Check if the texture is 0 */
      if ((tile_idt & 0x000F) == 0) {
        tile_write_num = (tile_idt >> 4);
        tile_idt = 0x0001;
      }
      else {
        tile_write_num = 1;
      }

      /* Write the tile(s) */
      for (uint16_t k = 0; k < tile_write_num; ++k) {
        IO(write, &tile_idt, 1, layer);
      }

      j += tile_write_num - 1;
      tile_current += TILE_SIZE;
    }

    /* Write the terminating layer tile */
    tile_idt = 0xFFFF;
    IO(write, &tile_idt, 1, layer);

    POP; /* Pop the layer file */
  }

  FN_EXIT;
}

/* Write zones */
static int data_to_files_z(struct JRMP_Data *data)
{
  FILE    *file;
  uint16_t zone_num;
  uint32_t zone_tile_start;
  uint32_t zone_tile_end;
  uint32_t zone_flags;
  char     zone_id[4];

  DATA_CHECK;
  FN_START;

  if (!(file = fopen(".jrmpz", "wb"))) ERROR_EXIT;
  PUSH('f', file);

  /* Write zone num */
  if (JRMP_data_block_seek(data, "ZONES  ")) ERROR_EXIT;
  IO(read, &zone_num, 1, data->file);
  IO(write, &zone_num, 1, file);

  for (uint16_t i = 0; i < zone_num; ++i) {
    IO(read, &zone_tile_start, 1, data->file);
    IO(read, &zone_tile_end,   1, data->file);
    IO(read, &zone_flags,      1, data->file);
    IO(read, &zone_id,         1, data->file);
    
    IO(write, &zone_tile_start, 1, file);
    IO(write, &zone_tile_end,   1, file);
    IO(write, &zone_flags,      1, file);
    IO(write, &zone_id,         1, file);
  }

  FN_EXIT;
}

/* Write script */
static int data_to_files_s(struct JRMP_Data *data)
{
  FILE *file;
  char script[SCRIPT_SIZE];

  DATA_CHECK;
  FN_START;

  if (!(file = fopen(".jrmps", "wb"))) ERROR_EXIT;
  PUSH('f', file);

  JRMP_data_block_seek(data, "SCRIPT ");
  IO(read, script, SCRIPT_SIZE, data->file);
  IO(write, script, SCRIPT_SIZE, file);

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
  if (data_to_files_z(data)) return -1; /* Zone info */
  if (data_to_files_s(data)) return -1; /* Script info */

  return 0;
}

/*
 * ========================================
 * FILES TO DATA
 * ========================================
 */
#define WRITE_NAME(blk_name) \
  if (fseek(data, name_pos, SEEK_SET)) ERROR_EXIT; \
  strcpy(name, blk_name); \
  IO(write, name, BLK_NAME_SIZE, data);

/* Write header/map info */
int files_to_data_h(FILE *data, const uint64_t name_pos, const uint64_t current_pos)
{
  FILE    *file;
  uint32_t temp[3];
  char     name[BLK_NAME_SIZE];

  FN_START;

  if (!(file = fopen(".jrmph", "rb"))) ERROR_EXIT;
  PUSH('f', file);

  IO(read, temp, 3, file);

  WRITE_NAME("MAPINFO");

  SEEK(data, current_pos);
  IO(write, temp, 3, data);

  FN_EXIT;
}

/* Write tiles */
#define R_TILE IO(read, &temp_16, 1, layer);
#define W_TILE IO(write, &temp_16, 1, data);
#define RW_TILE \
  R_TILE; \
  W_TILE;
int files_to_data_t(FILE *data, const uint64_t name_pos, const uint64_t current_pos)
{
  FILE    *table;
  FILE    *layer;
  uint64_t layer_pos;
  uint64_t tile_pos;
  uint8_t  temp_8;
  uint16_t temp_16;
  uint32_t lt_num;
  float    layer_parallax;
  uint16_t repeated_tiles;
  char     layer_name[LAYER_NAME_SIZE + 1];
  char     name[BLK_NAME_SIZE];
  /* There is also a char * made later */

  FN_START;

  WRITE_NAME("TILES  ");
  
  SEEK(data, current_pos);

  /* Get the lt num (using table file for this) */
  if (!(table = fopen(".jrmph", "rb"))) ERROR_EXIT;
  IO(read, &lt_num, 1, table);
  fclose(table);

  /* Set layer and tile positions */
  TELL(layer_pos, data);
  IO(write, &temp_8, LT_LAYERS(lt_num), data); /* Write junk so tile position can be found */
  IO(write, &layer_parallax, LT_LAYERS(lt_num), data);
  TELL(tile_pos, data);
  
  if (!(table = fopen(".jrmplt", "rb"))) ERROR_EXIT;
  PUSH('f', table);

  for (uint8_t i = 0; i < LT_LAYERS(lt_num); ++i) {
    /* Get the layer file */
    if (fgets(layer_name, LAYER_NAME_SIZE + 1, table) != layer_name) ERROR_EXIT;
    /* Remove '\n' */
    {
      char *c = layer_name;
      while (*c != '\n') { ++c; if (*c == '\0') ERROR_EXIT; }
      *c = '\0';
    }
    if (!(layer = fopen(layer_name, "rb"))) ERROR_EXIT;
    PUSH('f', layer);
    /* Write the layer */
    IO(read, &temp_8, 1, layer);
    IO(read, &layer_parallax, 1, layer);
    SEEK(data, layer_pos);
    IO(write, &temp_8, 1, data);
    IO(write, &layer_parallax, 1, data);

    layer_pos += LAYER_SIZE;
    /* Write the tiles */
    SEEK(data, tile_pos);

    RW_TILE;
    while (temp_16 != 0xFFFF) {
      /* Repeated blank tiles */
      if ((temp_16 >> 4) == 0) {
        repeated_tiles = 0;

        while (repeated_tiles < 0x0FFF && (temp_16 >> 4) == 0) {
          ++repeated_tiles;
          R_TILE;
        }
        /* Deal with the tile just read (fseek back layer for a re-read) */
        if (fseek(layer, -1 * TILE_SIZE, SEEK_CUR)) ERROR_EXIT;
        /* Update the blank tile */
        SEEK(data, tile_pos);
        temp_16 = (repeated_tiles << 4);
        W_TILE; /* Flag will be junk value */
      }

      if (temp_16 != 0xFFFF) {
        tile_pos += TILE_SIZE;
        RW_TILE; /* Read the next tile */
      }
    }
    
    POP; /* Pop the layer file */
  }
  
  FN_EXIT;
}

/* Write zones */
int files_to_data_z(FILE *data, const uint64_t name_pos, const uint64_t current_pos)
{
  FILE    *file;
  char     name[BLK_NAME_SIZE];
  uint16_t temp_16;
  uint32_t temp_32[3];
  char     zone_id[4];

  FN_START;

  if (!(file = fopen(".jrmpz", "rb"))) ERROR_EXIT;
  PUSH('f', file);
  
  WRITE_NAME("ZONES  ");
  
  SEEK(data, current_pos);

  IO(read, &temp_16, 1, file);
  IO(write, &temp_16, 1, data);

  for (uint16_t i = 0; i < temp_16; ++i) {
    IO(read, temp_32, 3, file);
    IO(write, temp_32, 3, data);
    IO(read, zone_id, 4, file);
    IO(write, zone_id, 4, data);
  }

  FN_EXIT;
}

/* Write script */
int files_to_data_s(FILE *data, const uint64_t name_pos, const uint64_t current_pos)
{
  FILE *file;
  char name[BLK_NAME_SIZE];
  char buffer[SCRIPT_SIZE];

  FN_START;

  if (!(file = fopen(".jrmps", "rb"))) ERROR_EXIT;
  PUSH('f', file);

  IO(read, buffer, SCRIPT_SIZE, file);
  
  WRITE_NAME("SCRIPT ");
  
  SEEK(data, current_pos);
  IO(write, buffer, SCRIPT_SIZE, data);

  FN_EXIT;
}

/*
 * JRMP_files_to_data
 */
#define FTD_BLOCKS 4 /* Don't count BLKINFO */
typedef int (*ftd_func)(FILE *, const uint64_t, const uint64_t); /* Write name first, then go to current and put all the data */
int JRMP_files_to_data(const char *file)
{
  FILE    *data;
  uint64_t name_pos; /* Current position of names */
  uint64_t offset_pos; /* Current position of offsets */
  uint64_t current_pos; /* Will need to go back and forth between block info and current */
  /* For BLKINFO */
  uint8_t  block_num;
  char     block_name[BLK_NAME_SIZE];
  uint64_t block_offset;
  /* Block writing functions */
  ftd_func functions[FTD_BLOCKS] = {
    files_to_data_h,
    files_to_data_t,
    files_to_data_z,
    files_to_data_s,
  };

  FN_START;

  if (!(data = fopen(file, "wb"))) ERROR_EXIT;
  PUSH('f', data);

  /* Write the block info block */
  block_num = FTD_BLOCKS + 1;
  strcpy(block_name, "BLKINFO");
  block_offset = 0;
  /* Write block num */
  IO(write, &block_num, 1, data);
  /* Write block name and 'write' all block names */
  IO(write, block_name, BLK_NAME_SIZE, data);
  TELL(name_pos, data);
  if (fseek(data, FTD_BLOCKS * sizeof(char [BLK_NAME_SIZE]), SEEK_CUR)) ERROR_EXIT;
  /* Write block offset and 'write' all block offsets */
  IO(write, &block_offset, 1, data);
  TELL(offset_pos, data);
  if (fseek(data, FTD_BLOCKS * sizeof(uint64_t), SEEK_CUR)) ERROR_EXIT;
  /* Set current position */
  TELL(current_pos, data);

  /* Write the other blocks */
  for (uint8_t i = 0; i < FTD_BLOCKS; ++i) {
    /* Write the offset */
    SEEK(data, offset_pos);
    IO(write, &current_pos, 1, data);
    SEEK(data, current_pos);
    offset_pos += sizeof(uint64_t);
    /* Write the block */
    if (functions[i](data, name_pos, current_pos)) ERROR_EXIT;
    TELL(current_pos, data);
    /* Name has been written, move pos */ 
    name_pos += sizeof(char [BLK_NAME_SIZE]);
  }

  FN_EXIT;
}
