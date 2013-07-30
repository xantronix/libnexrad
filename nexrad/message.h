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

    nexrad_packet * current_symbology_packet;
    nexrad_packet * current_graphic_packet;
    nexrad_packet * current_tabular_packet;
} nexrad_message;

/*
 * Methods for facilitating file I/O
 */
nexrad_message * nexrad_message_open(const char *path);
void             nexrad_message_close(nexrad_message *message);

/*
 * Methods for reading product packets
 */
nexrad_packet * nexrad_message_read_symbology_packet(nexrad_message *message, size_t *size);
nexrad_packet * nexrad_message_read_graphic_packet(nexrad_message *message, size_t *size);
nexrad_packet * nexrad_message_read_tabular_packet(nexrad_message *message, size_t *size);

nexrad_packet_type * nexrad_packet_lookup_type(nexrad_packet *packet);

#pragma pack(pop)

#endif /* _NEXRAD_MESSAGE_H */
