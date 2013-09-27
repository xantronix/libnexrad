#ifndef _NEXRAD_COLOR_H
#define _NEXRAD_COLOR_H

#include <stdint.h>

#define NEXRAD_COLOR_TABLE_MAGIC "CLUT"

typedef struct _nexrad_color_table {
    char    magic[4];   /* Always "CLUT" */
    uint8_t entries;    /* Number of entries in table (powers of 2) */
    uint8_t entry_size; /* Number of bytes per entry */
} nexrad_color_table;

nexrad_color_table *nexrad_color_table_open(const char *path);

void nexrad_color_table_close(nexrad_color_table *table);

#endif /* _NEXRAD_COLOR_H */
