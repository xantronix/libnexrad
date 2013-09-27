#ifndef _NEXRAD_COLOR_H
#define _NEXRAD_COLOR_H

#include <stdint.h>

#define NEXRAD_COLOR_TABLE_MAGIC "CLUT"

typedef struct _nexrad_color_table {
    char   magic[4]; /* Always "CLUT" */
    size_t size;     /* Number of entries in table (powers of 2) */
} nexrad_color_table;

typedef struct _nexrad_color_table_entry {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} nexrad_color_table_entry;

nexrad_color_table *nexrad_color_table_create(size_t size);

void nexrad_color_table_store_entry(nexrad_color_table *table,
    uint8_t index, uint8_t r, uint8_t g, uint8_t b
);

nexrad_color_table *nexrad_color_table_open(const char *path);

nexrad_color_table_entry *nexrad_color_table_get_entries(nexrad_color_table *table, size_t *size);

int nexrad_color_table_save(nexrad_color_table *table, const char *path);

void nexrad_color_table_close(nexrad_color_table *table);

#endif /* _NEXRAD_COLOR_H */
