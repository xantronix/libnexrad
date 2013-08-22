#ifndef _NEXRAD_MESSAGE_GRAPHIC_H
#define _NEXRAD_MESSAGE_GRAPHIC_H

#include <stdint.h>

#include <nexrad/block.h>
#include <nexrad/chunk.h>
#include <nexrad/packet.h>

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

nexrad_chunk *  nexrad_graphic_block_open(nexrad_graphic_block *block);
nexrad_chunk *  nexrad_graphic_block_read_page(nexrad_chunk *block);
void            nexrad_graphic_page_next_packet(nexrad_chunk *page, size_t size);
nexrad_packet * nexrad_graphic_page_peek_packet(nexrad_chunk *page, size_t *size);
nexrad_packet * nexrad_graphic_page_read_packet(nexrad_chunk *page, size_t *size);
void            nexrad_graphic_page_close(nexrad_chunk *page);
void            nexrad_graphic_block_close(nexrad_chunk *block);

#endif /* _NEXRAD_MESSAGE_GRAPHIC_H */
