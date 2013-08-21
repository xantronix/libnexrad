#ifndef _NEXRAD_MESSAGE_SYMBOLOGY_H
#define _NEXRAD_MESSAGE_SYMBOLOGY_H

#include <stdint.h>

#include <nexrad/message/block.h>

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

#endif /* _NEXRAD_MESSAGE_SYMBOLOGY_H */
