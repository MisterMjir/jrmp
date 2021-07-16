#ifndef JRMP_IO_H
#define JRMP_IO_H

#include "data.h"

int JRMP_data_to_files(struct JRMP_Data *data);
int JRMP_files_to_data(struct JRMP_Data *data);

#endif
