#ifndef _NEXRAD_MESSAGE_GRAPHIC_H
#define _NEXRAD_MESSAGE_GRAPHIC_H

#include <stdint.h>

#include <nexrad/message/block.h>
#include <nexrad/message/chunk.h>

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_graphic_block {
    nexrad_block_header header;

    uint16_t pages; /* Number of alphanumeric pages */
} nexrad_graphic_block;

typedef struct _nexrad_graphic_page {
    uint16_t page;
    uint16_t size;
} nexrad_graphic_page;

#pragma pack(pop)

#endif /* _NEXRAD_MESSAGE_GRAPHIC_H */
