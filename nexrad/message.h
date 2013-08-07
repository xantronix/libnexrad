#ifndef _NEXRAD_MESSAGE_H
#define _NEXRAD_MESSAGE_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/message/format.h>

#pragma pack(push)
#pragma pack(1)

#define NEXRAD_BLOCK_DIVIDER -1
#define NEXRAD_VERSION        1

enum nexrad_block_id {
    NEXRAD_BLOCK_MESSAGE_HEADER,
    NEXRAD_BLOCK_PRODUCT_DESCRIPTION,
    NEXRAD_BLOCK_SYMBOLOGY,
    NEXRAD_BLOCK_GRAPHIC,
    NEXRAD_BLOCK_TABULAR
};

enum nexrad_product_id {
    NEXRAD_PRODUCT_NHI = 59
};

enum nexrad_packet_type_id {
    NEXRAD_PACKET_TYPE_TEXT   =  8,
    NEXRAD_PACKET_TYPE_VECTOR = 10,
    NEXRAD_PACKET_TYPE_HAIL   = 19
};

enum nexrad_radar_mode_id {
    NEXRAD_RADAR_MODE_PRECIP = 2
};

typedef struct _nexrad_product {
    enum nexrad_product_id id;
    char                   name[8];
} nexrad_product;

typedef nexrad_packet_header nexrad_packet;

typedef struct _nexrad_packet_type {
    int    id;
    char   name[8];
    size_t size;
} nexrad_packet_type;

typedef struct _nexrad_message {
    size_t size;
    size_t page_size;
    size_t mapped_size;
    int    fd;
    void * data;

    nexrad_file_header *         file_header;
    nexrad_message_header *      message_header;
    nexrad_product_description * description;
    nexrad_symbology_block *     symbology;
    nexrad_graphic_block *       graphic;
    nexrad_tabular_block *       tabular;
} nexrad_message;

enum nexrad_chunk_type_id {
    NEXRAD_CHUNK_SYMBOLOGY_BLOCK  = 1,
    NEXRAD_CHUNK_GRAPHIC_BLOCK    = 2,
    NEXRAD_CHUNK_TABULAR_BLOCK    = 3,
    NEXRAD_CHUNK_SYMBOLOGY_LAYER  = 4,
    NEXRAD_CHUNK_SYMBOLOGY_PACKET = 5,
    NEXRAD_CHUNK_GRAPHIC_PAGE     = 6,
    NEXRAD_CHUNK_GRAPHIC_PACKET   = 7,
    NEXRAD_CHUNK_TABULAR_PAGE     = 8,
    NEXRAD_CHUNK_TABULAR_PACKET   = 9
};

typedef struct _nexrad_chunk_iterator {
    enum nexrad_chunk_type_id parent_type_id;
    enum nexrad_chunk_type_id child_type_id;

    void * parent;          /* The parent chunk */
    void * first;           /* The first chunk within the parent chunk */
    void * current;         /* The current chunk within the parent chunk */
    int    count;           /* Number of chunks within the parent */
    size_t bytes_remaining; /* Number of bytes left in parent chunk */
} nexrad_chunk_iterator;

/*
 * Methods for facilitating file I/O
 */
nexrad_message * nexrad_message_open(const char *path);
void             nexrad_message_close(nexrad_message *message);

/*
 * Generic interface for reading radar product data chunks
 */
ssize_t                 nexrad_chunk_size(void *chunk, enum nexrad_chunk_type_id type);
nexrad_chunk_iterator * nexrad_chunk_open(void *chunk, enum nexrad_chunk_type_id type);
void *                  nexrad_chunk_read(nexrad_chunk_iterator *chunk, size_t *size);
void                    nexrad_chunk_close(nexrad_chunk_iterator *iterator);

#pragma pack(pop)

#endif /* _NEXRAD_MESSAGE_H */
