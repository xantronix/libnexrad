#ifndef _NEXRAD_MESSAGE_CHUNK_H
#define _NEXRAD_MESSAGE_CHUNK_H

#include <stdint.h>
#include <sys/types.h>

enum nexrad_chunk_type_id {
    NEXRAD_CHUNK_UNKNOWN,
    NEXRAD_CHUNK_SYMBOLOGY_BLOCK,
    NEXRAD_CHUNK_GRAPHIC_BLOCK,
    NEXRAD_CHUNK_TABULAR_BLOCK,
    NEXRAD_CHUNK_SYMBOLOGY_LAYER,
    NEXRAD_CHUNK_SYMBOLOGY_PACKET,
    NEXRAD_CHUNK_GRAPHIC_PAGE,
    NEXRAD_CHUNK_GRAPHIC_PACKET
};

typedef struct _nexrad_chunk {
    enum nexrad_chunk_type_id type;

    void * current;    /* The current chunk within the parent chunk */
    size_t bytes_left; /* Number of bytes left in parent chunk */
} nexrad_chunk;

typedef struct _nexrad_text {
    char * current;    /* Current pointer */
    int    page;       /* Current page number */
    int    line;       /* Line number in current page */
    int    pages_left; /* Number of pages left in text */
    size_t bytes_left; /* Number of bytes left in text */
} nexrad_text;

/*
 * Generic interface for reading radar product data chunks
 */
nexrad_chunk * nexrad_chunk_open(void *chunk, enum nexrad_chunk_type_id type);
void *         nexrad_chunk_read(nexrad_chunk *iterator, size_t *total_size, size_t *data_size, void **data);
void           nexrad_chunk_close(nexrad_chunk *iterator);

#endif /* _NEXRAD_MESSAGE_CHUNK_H */
