#ifndef _NEXRAD_MESSAGE_BLOCK_H
#define _NEXRAD_MESSAGE_BLOCK_H

#include <stdint.h>

#include <nexrad/message/types.h>

#pragma pack(push)
#pragma pack(1)

#define NEXRAD_BLOCK_DIVIDER -1

#define nexrad_block_after(data, prev) (void *)((char *)data + sizeof(prev))

enum nexrad_block_id {
    NEXRAD_BLOCK_SYMBOLOGY           = 1,
    NEXRAD_BLOCK_GRAPHIC             = 2,
    NEXRAD_BLOCK_TABULAR             = 3,
    NEXRAD_BLOCK_MESSAGE_HEADER      = 4,
    NEXRAD_BLOCK_PRODUCT_DESCRIPTION = 5
};

typedef struct _nexrad_block_header {
     int16_t divider; /* Divider indicating start of block */
    uint16_t id;      /* Block ID */
    uint32_t size;    /* Size of block in entirety, including full header */
} nexrad_block_header;

#pragma pack(pop)

#endif /* _NEXRAD_MESSAGE_BLOCK_H */
