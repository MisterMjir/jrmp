#include "jrmp.h"
#include <stdio.h>

int main(int argc, char *args[])
{
  struct JRMP_Data data;

  JRMP_data_create(&data, "test_data.jrmp");

  printf("Found %d blocks\n", data.blocks_num);

  for (int i = 0; i < data.blocks_num; ++i) {
    printf("%s at offset %lu\n", data.blocks_name[i], data.blocks_offset[i]);
  }

  JRMP_data_to_files(&data);

  JRMP_data_destory(&data);

  return 0;
}
