#ifndef _NEXRAD_MESSAGE_SYMBOLOGY_H
#define _NEXRAD_MESSAGE_SYMBOLOGY_H

#include <stdint.h>

#include <nexrad/block.h>
#include <nexrad/chunk.h>

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

nexrad_chunk * nexrad_symbology_block_open(nexrad_symbology_block *block);

#endif /* _NEXRAD_MESSAGE_SYMBOLOGY_H */
