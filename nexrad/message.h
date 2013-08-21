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
 * Convenience methods for accessing the three major data blocks within a
 * product message file
 */
nexrad_chunk *   nexrad_message_open_symbology_block(nexrad_message *message);
nexrad_chunk *   nexrad_message_open_graphic_block(nexrad_message *message);
nexrad_tabular_text *
                 nexrad_message_open_tabular_block(nexrad_message *message);

#endif /* _NEXRAD_MESSAGE_H */
