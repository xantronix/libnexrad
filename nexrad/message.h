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
    NEXRAD_BLOCK_SYMBOLOGY           = 1,
    NEXRAD_BLOCK_GRAPHIC             = 2,
    NEXRAD_BLOCK_TABULAR             = 3,
    NEXRAD_BLOCK_MESSAGE_HEADER      = 4,
    NEXRAD_BLOCK_PRODUCT_DESCRIPTION = 5
};

enum nexrad_product_id {
    NEXRAD_PRODUCT_NHI = 59
};

enum nexrad_packet_type_id {
    NEXRAD_PACKET_TYPE_UNKNOWN =  0,
    NEXRAD_PACKET_TYPE_TEXT    =  8,
    NEXRAD_PACKET_TYPE_VECTOR  = 10,
    NEXRAD_PACKET_TYPE_HAIL    = 19
};

enum nexrad_radar_mode_id {
    NEXRAD_RADAR_MODE_PRECIP = 2
};

typedef struct _nexrad_product {
    enum nexrad_product_id id;
    char                   name[8];
} nexrad_product;

typedef nexrad_packet_header nexrad_packet;

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
 * Methods for facilitating file I/O
 */
nexrad_message * nexrad_message_open(const char *path);
void             nexrad_message_close(nexrad_message *message);

/*
 * Generic interface for reading radar product data chunks
 */
nexrad_chunk * nexrad_chunk_open(void *chunk, enum nexrad_chunk_type_id type);
void *         nexrad_chunk_read(nexrad_chunk *iterator, size_t *total_size, size_t *data_size, void **data);
void           nexrad_chunk_close(nexrad_chunk *iterator);

/*
 * Specialized interface for reading product symbology data
 */
nexrad_chunk *  nexrad_symbology_block_open(nexrad_message *message);
nexrad_chunk *  nexrad_symbology_block_read_layer(nexrad_chunk *block);
nexrad_packet * nexrad_symbology_layer_read_packet(nexrad_chunk *layer, size_t *total_size, size_t *data_size, void **data);
void            nexrad_symbology_layer_close(nexrad_chunk *layer);
void            nexrad_symbology_block_close(nexrad_chunk *block);

nexrad_chunk *  nexrad_graphic_block_open(nexrad_message *message);
nexrad_chunk *  nexrad_graphic_block_read_page(nexrad_chunk *block);
nexrad_packet * nexrad_graphic_page_read_packet(nexrad_chunk *page, size_t *total_size, size_t *data_size, void **data);
void            nexrad_graphic_page_close(nexrad_chunk *page);
void            nexrad_graphic_block_close(nexrad_chunk *block);

nexrad_text *  nexrad_tabular_block_open(nexrad_message *message);
ssize_t        nexrad_tabular_block_read_line(nexrad_text *text, char **data, int *page, int *line);
void           nexrad_tabular_block_close(nexrad_text *block);

enum nexrad_packet_type_id
               nexrad_packet_type(nexrad_packet *packet);

#pragma pack(pop)

#endif /* _NEXRAD_MESSAGE_H */
