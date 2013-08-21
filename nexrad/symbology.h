#ifndef _NEXRAD_MESSAGE_SYMBOLOGY_H
#define _NEXRAD_MESSAGE_SYMBOLOGY_H

#include <stdint.h>

#include <nexrad/block.h>
#include <nexrad/chunk.h>
#include <nexrad/packet.h>

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_symbology_block {
    nexrad_block_header header;

    uint16_t layers; /* Number of layers following */
} nexrad_symbology_block;

typedef struct _nexrad_symbology_layer {
     int16_t divider; /* Layer divider */
    uint32_t size;    /* Size of data to follow */
} nexrad_symbology_layer;

#pragma pack(pop)

nexrad_chunk *  nexrad_symbology_block_open(nexrad_symbology_block *block);
nexrad_chunk *  nexrad_symbology_block_read_layer(nexrad_chunk *block);
nexrad_packet * nexrad_symbology_layer_read_packet(nexrad_chunk *layer, size_t *size);
void            nexrad_symbology_layer_close(nexrad_chunk *layer);
void            nexrad_symbology_block_close(nexrad_chunk *block);

#endif /* _NEXRAD_MESSAGE_SYMBOLOGY_H */
