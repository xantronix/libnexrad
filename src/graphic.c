#include <stdlib.h>

#include <nexrad/graphic.h>

nexrad_chunk *nexrad_graphic_block_open(nexrad_graphic_block *block) {
    return nexrad_chunk_open(block, NEXRAD_CHUNK_GRAPHIC_BLOCK);
}

nexrad_chunk *nexrad_graphic_block_read_page(nexrad_chunk *block) {
    return nexrad_chunk_read_block_layer(block, NEXRAD_CHUNK_GRAPHIC_PAGE);
}

void nexrad_graphic_page_next_packet(nexrad_chunk *page, size_t size) {
    nexrad_chunk_next(page, size);
}

nexrad_packet *nexrad_graphic_page_peek_packet(nexrad_chunk *page, size_t *size) {
    return nexrad_chunk_peek(page, size, NULL, NULL);
}

nexrad_packet *nexrad_graphic_page_read_packet(nexrad_chunk *page, size_t *size) {
    return nexrad_chunk_read(page, size, NULL, NULL);
}

void nexrad_graphic_page_close(nexrad_chunk *page) {
    nexrad_chunk_close(page);
}

void nexrad_graphic_block_close(nexrad_chunk *block) {
    nexrad_chunk_close(block);
}

