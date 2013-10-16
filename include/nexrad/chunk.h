#ifndef _NEXRAD_CHUNK_H
#define _NEXRAD_CHUNK_H

#include <stdint.h>
#include <sys/types.h>

enum nexrad_chunk_type {
    NEXRAD_CHUNK_UNKNOWN,
    NEXRAD_CHUNK_SYMBOLOGY_BLOCK,
    NEXRAD_CHUNK_GRAPHIC_BLOCK,
    NEXRAD_CHUNK_TABULAR_BLOCK,
    NEXRAD_CHUNK_SYMBOLOGY_LAYER,
    NEXRAD_CHUNK_SYMBOLOGY_PACKET,
    NEXRAD_CHUNK_GRAPHIC_PAGE,
    NEXRAD_CHUNK_GRAPHIC_PACKET
};

typedef struct _nexrad_chunk nexrad_chunk;

/*
 * Generic interface for reading radar product data chunks
 */
nexrad_chunk *nexrad_chunk_open(void *chunk, enum nexrad_chunk_type type);

void *nexrad_chunk_peek(nexrad_chunk *iterator,
    size_t *size,
    size_t *payload,
    void **data
);

void nexrad_chunk_next(nexrad_chunk *iterator, size_t size);

void *nexrad_chunk_read(nexrad_chunk *iterator,
    size_t *size,
    size_t *payload,
    void **data
);

nexrad_chunk *nexrad_chunk_read_block_layer(nexrad_chunk *block,
    enum nexrad_chunk_type type
);

void nexrad_chunk_close(nexrad_chunk *iterator);

#endif /* _NEXRAD_CHUNK_H */
