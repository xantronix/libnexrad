#include <stdlib.h>
#include <errno.h>
#include <endian.h>

#include <nexrad/header.h>
#include <nexrad/product.h>
#include <nexrad/symbology.h>
#include <nexrad/graphic.h>
#include <nexrad/tabular.h>
#include <nexrad/packet.h>
#include <nexrad/chunk.h>

static enum nexrad_chunk_type_id nexrad_chunk_child_types[] = {
    /* none                          => */ 0,
    /* NEXRAD_CHUNK_SYMBOLOGY_BLOCK  => */ NEXRAD_CHUNK_SYMBOLOGY_LAYER,
    /* NEXRAD_CHUNK_GRAPHIC_BLOCK    => */ NEXRAD_CHUNK_GRAPHIC_PAGE,
    /* NEXRAD_CHUNK_TABULAR_BLOCK    => */ 0,
    /* NEXRAD_CHUNK_SYMBOLOGY_LAYER  => */ NEXRAD_CHUNK_SYMBOLOGY_PACKET,
    /* NEXRAD_CHUNK_SYMBOLOGY_PACKET => */ 0,
    /* NEXRAD_CHUNK_GRAPHIC_PAGE     => */ NEXRAD_CHUNK_GRAPHIC_PACKET,
    /* NEXRAD_CHUNK_GRAPHIC_PACKET   => */ 0
};

/*
 * A table corresponding to the IDs listed in enum nexrad_chunk_type_id which
 * allows quick lookup of chunk header sizes.
 */
static size_t nexrad_chunk_header_sizes[] = {
    /* none => */                          0,
    /* NEXRAD_CHUNK_SYMBOLOGY_BLOCK  => */ sizeof(nexrad_symbology_block),
    /* NEXRAD_CHUNK_GRAPHIC_BLOCK    => */ sizeof(nexrad_graphic_block),
    /* NEXRAD_CHUNK_TABULAR_BLOCK    => */ sizeof(nexrad_tabular_block),
    /* NEXRAD_CHUNK_SYMBOLOGY_LAYER  => */ sizeof(nexrad_symbology_layer),
    /* NEXRAD_CHUNK_SYMBOLOGY_PACKET => */ sizeof(nexrad_packet_header),
    /* NEXRAD_CHUNK_GRAPHIC_PAGE     => */ sizeof(nexrad_graphic_page),
    /* NEXRAD_CHUNK_GRAPHIC_PACKET   => */ sizeof(nexrad_packet_header)
};

static ssize_t find_chunk_size(void *chunk, enum nexrad_chunk_type_id type) {
    switch (type) {
        case NEXRAD_CHUNK_SYMBOLOGY_BLOCK:
        case NEXRAD_CHUNK_GRAPHIC_BLOCK:
        case NEXRAD_CHUNK_TABULAR_BLOCK: {
            nexrad_block_header *header = chunk;

            if ((int16_t)be16toh(header->divider) != -1
              || be16toh(header->id)   != type
              || be32toh(header->size) <= nexrad_chunk_header_sizes[type]) {
                goto error_bad_header;
            }

            return be32toh(header->size) - nexrad_chunk_header_sizes[type];
        }

        case NEXRAD_CHUNK_SYMBOLOGY_PACKET:
        case NEXRAD_CHUNK_GRAPHIC_PACKET: {
            nexrad_packet_header *header = chunk;

            return be16toh(header->size);
        }

        case NEXRAD_CHUNK_SYMBOLOGY_LAYER: {
            nexrad_symbology_layer *header = chunk;

            if ((int16_t)be16toh(header->divider) != -1) goto error_bad_header;

            return be32toh(header->size);
        }

        case NEXRAD_CHUNK_GRAPHIC_PAGE: {
            nexrad_graphic_page *header = chunk;

            return be16toh(header->size);
        }

        case NEXRAD_CHUNK_UNKNOWN: {
            break;
        }
    }

error_bad_header:
    return -1;
}

nexrad_chunk *nexrad_chunk_open(void *chunk, enum nexrad_chunk_type_id type) {
    nexrad_chunk *iterator;
    size_t size;

    if ((size = find_chunk_size(chunk, type)) < 0) {
        goto error_bad_chunk;
    }

    if ((iterator = malloc(sizeof(*iterator))) == NULL) {
        goto error_malloc;
    }

    iterator->type       = nexrad_chunk_child_types[type];
    iterator->current    = (char *)chunk + nexrad_chunk_header_sizes[type];
    iterator->bytes_left = size;

    return iterator;

error_malloc:
error_bad_chunk:
    return NULL;
}

void *nexrad_chunk_peek(nexrad_chunk *iterator) {
    if (iterator == NULL) return NULL;

    /*
     * If there are no more bytes remaining, then return null.
     */
    if (iterator->bytes_left == 0) {
        return NULL;
    }

    return iterator->current;
}

void nexrad_chunk_next(nexrad_chunk *iterator, size_t *total_size, size_t *data_size, void **data) {
    size_t chunk_size;
    size_t header_size = nexrad_chunk_header_sizes[iterator->type];

    /*
     * Determine the size of the current chunk based on the child type currently
     * being iterated over.
     */
    chunk_size = find_chunk_size(iterator->current, iterator->type);

    /*
     * Advance the current pointer beyond the current chunk size, plus the size
     * of the chunk header (0 when chunk-declared sizes are inclusive of their
     * respective header sizes).
     */
    iterator->current = (char *)iterator->current + chunk_size + header_size;

    /*
     * Decrement the number of bytes remaining appropriately.
     */
    iterator->bytes_left -= chunk_size + header_size;

    /*
     * If a pointer was provided to store the resultant total chunk size in,
     * then provide that value.
     */
    if (total_size != NULL) {
        *total_size = chunk_size + header_size;
    }

    /*
     * If a pointer was provided to store the resultant chunk size in, then
     * provide that value.
     */
    if (data_size != NULL) {
        *data_size = chunk_size;
    }

    /*
     * If a pointer was provided to store the chunk data address in, then
     * provide that value as well.
     */
    if (data != NULL) {
        *data = (char *)iterator->current + nexrad_chunk_header_sizes[iterator->type];
    }
}

void *nexrad_chunk_read(nexrad_chunk *iterator, size_t *total_size, size_t *data_size, void **data) {
    void *ret;

    /*
     * Mark the current data child for return, if any data is still available.
     */
    if ((ret = nexrad_chunk_peek(iterator)) == NULL) {
        return NULL;
    }

    /*
     * Move to the next chunk.
     */
    nexrad_chunk_next(iterator, total_size, data_size, data);

    return ret;
}

nexrad_chunk *nexrad_chunk_read_block_layer(nexrad_chunk *block, enum nexrad_chunk_type_id type) {
    void *layer;

    if ((layer = nexrad_chunk_read(block, NULL, NULL, NULL)) == NULL) {
        goto error_chunk_read;
    }

    return nexrad_chunk_open(layer, type);

error_chunk_read:
    return NULL;
}

void nexrad_chunk_close(nexrad_chunk *iterator) {
    if (iterator == NULL) return;

    iterator->type       = 0;
    iterator->current    = NULL;
    iterator->bytes_left = 0;

    free(iterator);
}
