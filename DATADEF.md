# Just an informal structure of the current data layout

```
.BLKINFO
uint8_t  block_num        /* How many sections of data */
char     block_names[8][] /* Names of the blocks */
uint64_t block_offsets[]  /* Offset of each block from the beginning of the file */

.MAPINFO
uint32_t lt_num           /* Layer and tile amounts (l:8 t:24) */
uint32_t tiles_x          /* Columns in layer 0 */
uint32_t tiles_y          /* Rows in layery 0 */

.TILES
/* An array of layers */
{
uint8_t  id               /* Layer id, or what layer the layer is */
float    parallax         /* Parallax weight of the layer, the formula is (total_tiles / pow())
}
/* An array of tiles */
{
uint16_t idt              /* Id and texture of a tile (12:id 4:t) */
}

.ZONES
uint16_t zone_num         /* Number of zones */
/* An array of zones */
{
uint32_t tile_start       /* Top left tile in the zone */
uint32_t tile_end         /* Bottom right tile in the zone */
uint32_t flags            /* Bit array of zone flags */
char     id[4]            /* Name/id of the zone */
}

.SCRIPT
char     script[406]      /* Script */
```

To update the file format, update the files to data functions first, create a new .jrmp file, then update the data to files
functions.
