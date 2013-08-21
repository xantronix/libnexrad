#ifndef _NEXRAD_MESSAGE_H
#define _NEXRAD_MESSAGE_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/header.h>
#include <nexrad/chunk.h>
#include <nexrad/product.h>
#include <nexrad/symbology.h>
#include <nexrad/graphic.h>
#include <nexrad/tabular.h>
#include <nexrad/packet.h>

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

/*
 * Methods for facilitating file I/O
 */
nexrad_message * nexrad_message_open(const char *path);
void             nexrad_message_close(nexrad_message *message);

/*
 * Specialized interface for reading product symbology data
 */
nexrad_chunk *  nexrad_symbology_block_open(nexrad_message *message);
nexrad_chunk *  nexrad_symbology_block_read_layer(nexrad_chunk *block);
nexrad_packet * nexrad_symbology_layer_read_packet(nexrad_chunk *layer, size_t *size);
void            nexrad_symbology_layer_close(nexrad_chunk *layer);
void            nexrad_symbology_block_close(nexrad_chunk *block);

nexrad_chunk *  nexrad_graphic_block_open(nexrad_message *message);
nexrad_chunk *  nexrad_graphic_block_read_page(nexrad_chunk *block);
nexrad_packet * nexrad_graphic_page_read_packet(nexrad_chunk *page, size_t *size);
void            nexrad_graphic_page_close(nexrad_chunk *page);
void            nexrad_graphic_block_close(nexrad_chunk *block);

nexrad_tabular_text *
                nexrad_tabular_block_open(nexrad_message *message);
ssize_t         nexrad_tabular_block_read_line(nexrad_tabular_text *text, char **data, int *page, int *line);
void            nexrad_tabular_block_close(nexrad_tabular_text *block);

#endif /* _NEXRAD_MESSAGE_H */
