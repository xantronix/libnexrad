#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <endian.h>

#include <nexrad/message.h>

#define nexrad_block_after(data, prev) ((void *)(data) + sizeof(prev))

static enum nexrad_chunk_type_id nexrad_chunk_child_types[] = {
    /* none                          => */ 0,
    /* NEXRAD_CHUNK_SYMBOLOGY_BLOCK  => */ NEXRAD_CHUNK_SYMBOLOGY_LAYER,
    /* NEXRAD_CHUNK_GRAPHIC_BLOCK    => */ NEXRAD_CHUNK_GRAPHIC_PAGE,
    /* NEXRAD_CHUNK_TABULAR_BLOCK    => */ 0,
    /* NEXRAD_CHUNK_SYMBOLOGY_LAYER  => */ NEXRAD_CHUNK_SYMBOLOGY_PACKET,
    /* NEXRAD_CHUNK_SYMBOLOGY_PACKET => */ 0,
    /* NEXRAD_CHUNK_GRAPHIC_PAGE     => */ NEXRAD_CHUNK_GRAPHIC_PACKET,
    /* NEXRAD_CHUNK_GRAPHIC_PACKET   => */ 0
};

/*
 * A table corresponding to the IDs listed in enum nexrad_chunk_type_id which
 * allows quick lookup of chunk header sizes.
 */
static size_t nexrad_chunk_header_sizes[] = {
    /* none => */ 0,
    /* NEXRAD_CHUNK_SYMBOLOGY_BLOCK  => */ sizeof(nexrad_symbology_block),
    /* NEXRAD_CHUNK_GRAPHIC_BLOCK    => */ sizeof(nexrad_graphic_block),
    /* NEXRAD_CHUNK_TABULAR_BLOCK    => */ sizeof(nexrad_tabular_block),
    /* NEXRAD_CHUNK_SYMBOLOGY_LAYER  => */ sizeof(nexrad_symbology_layer),
    /* NEXRAD_CHUNK_SYMBOLOGY_PACKET => */ sizeof(nexrad_packet_header),
    /* NEXRAD_CHUNK_GRAPHIC_PAGE     => */ sizeof(nexrad_graphic_page),
    /* NEXRAD_CHUNK_GRAPHIC_PACKET   => */ sizeof(nexrad_packet_header)
};

static ssize_t find_chunk_size(void *chunk, enum nexrad_chunk_type_id type) {
    switch (type) {
        case NEXRAD_CHUNK_SYMBOLOGY_BLOCK:
        case NEXRAD_CHUNK_GRAPHIC_BLOCK:
        case NEXRAD_CHUNK_TABULAR_BLOCK: {
            nexrad_block_header *header = chunk;

            if ((int16_t)be16toh(header->divider) != -1
              || be16toh(header->id)   != type
              || be32toh(header->size) <= nexrad_chunk_header_sizes[type]) {
                goto error_bad_header;
            }

            return be32toh(header->size) - nexrad_chunk_header_sizes[type];
        }

        case NEXRAD_CHUNK_SYMBOLOGY_PACKET:
        case NEXRAD_CHUNK_GRAPHIC_PACKET: {
            nexrad_packet_header *header = chunk;

            return be16toh(header->size);
        }

        case NEXRAD_CHUNK_SYMBOLOGY_LAYER: {
            nexrad_symbology_layer *header = chunk;

            if ((int16_t)be16toh(header->divider) != -1) goto error_bad_header;

            return be32toh(header->size);
        }

        case NEXRAD_CHUNK_GRAPHIC_PAGE: {
            nexrad_graphic_page *header = chunk;

            return be16toh(header->size);
        }

        case NEXRAD_CHUNK_UNKNOWN: {
            break;
        }
    }

error_bad_header:
    return -1;
}

nexrad_chunk *nexrad_chunk_open(void *chunk, enum nexrad_chunk_type_id type) {
    nexrad_chunk *iterator;
    size_t size;

    if ((size = find_chunk_size(chunk, type)) < 0) {
        goto error_bad_chunk;
    }

    if ((iterator = malloc(sizeof(*iterator))) == NULL) {
        goto error_malloc;
    }

    iterator->type       = nexrad_chunk_child_types[type];
    iterator->current    = (char *)chunk + nexrad_chunk_header_sizes[type];
    iterator->bytes_left = size;

    return iterator;

error_malloc:
error_bad_chunk:
    return NULL;
}

void *nexrad_chunk_read(nexrad_chunk *iterator, size_t *total_size, size_t *data_size, void **data) {
    size_t chunk_size;
    size_t header_size = nexrad_chunk_header_sizes[iterator->type];
    void *ret;

    if (iterator == NULL) return NULL;

    /*
     * If there are no more bytes remaining, then return null.
     */
    if (iterator->bytes_left == 0) {
        return NULL;
    }

    chunk_size = find_chunk_size(iterator->current, iterator->type);
    ret        = iterator->current;

    iterator->current += chunk_size + header_size;

    /*
     * Decrement the number of bytes remaining appropriately.
     */
    iterator->bytes_left -= chunk_size + header_size;

    /*
     * If a pointer was provided to store the resultant total chunk size in,
     * then provide that value.
     */
    if (total_size != NULL) {
        *total_size = chunk_size + header_size;
    }

    /*
     * If a pointer was provided to store the resultant chunk size in, then
     * provide that value.
     */
    if (data_size != NULL) {
        *data_size = chunk_size;
    }

    /*
     * If a pointer was provided to store the chunk data address in, then
     * provide that value as well.
     */
    if (data != NULL) {
        *data = (char *)iterator->current + nexrad_chunk_header_sizes[iterator->type];
    }

    /*
     * Return the requested chunk.
     */
    return ret;
}

void nexrad_chunk_close(nexrad_chunk *iterator) {
    if (iterator == NULL) return;

    iterator->type       = 0;
    iterator->current    = NULL;
    iterator->bytes_left = 0;

    free(iterator);
}

static inline int _mapped_size(size_t size, size_t page_size) {
    return size + (page_size - (size % page_size));
}

static inline off_t _halfword_offset(uint32_t value) {
    return sizeof(nexrad_file_header) + (be32toh(value) * 2);
}

static inline void *_block_pointer(nexrad_message *message, uint32_t raw_offset) {
    uint32_t offset = _halfword_offset(raw_offset);

    /*
     * Prevent an opportunity for segmentation fault by limiting the message
     * offset to exist within the physical size of the message.
     */
    if (offset > message->size) {
        return NULL;
    }

    return (void *)((char *)message->data + offset);
}

static inline nexrad_symbology_block *_symbology_block(nexrad_message *message, nexrad_product_description *description) {
    return (nexrad_symbology_block *)_block_pointer(message, description->symbology_offset);
}

static inline nexrad_graphic_block *_graphic_block(nexrad_message *message, nexrad_product_description *description) {
    return (nexrad_graphic_block *)_block_pointer(message, description->graphic_offset);
}

static inline nexrad_tabular_block *_tabular_block(nexrad_message *message, nexrad_product_description *description) {
    return (nexrad_tabular_block *)_block_pointer(message, description->tabular_offset);
}

static int _index_message(nexrad_message *message) {
    nexrad_file_header *file_header = message->data;

    nexrad_message_header *      message_header;
    nexrad_product_description * description;

    nexrad_symbology_block * symbology;
    nexrad_graphic_block *   graphic;
    nexrad_tabular_block *   tabular;

    if (message->size < sizeof(nexrad_file_header)) {
        goto error_invalid_file_header;
    }

    if (file_header->_whitespace1 != ' ') {
        goto error_invalid_file_header;
    }

    if (message->size < sizeof(nexrad_file_header) + sizeof(nexrad_message_header)) {
        goto error_invalid_message_header;
    }

    message_header = (nexrad_message_header *)nexrad_block_after(file_header, nexrad_file_header);

    if (be16toh(message_header->blocks) > 5) {
        goto error_invalid_message_header;
    }

    if (message->size < sizeof(nexrad_file_header)
      + sizeof(nexrad_message_header)
      + sizeof(nexrad_product_description)) {
        goto error_invalid_product_description;
    }

    description = (nexrad_product_description *)nexrad_block_after(message_header, nexrad_message_header);

    if ((symbology = _symbology_block(message, description)) == NULL) {
        goto error_invalid_symbology_block_offset;
    }

    if ((graphic = _graphic_block(message, description)) == NULL) {
        goto error_invalid_graphic_block_offset;
    }

    if ((tabular = _tabular_block(message, description)) == NULL) {
        goto error_invalid_tabular_block_offset;
    }

    message->file_header    = file_header;
    message->message_header = message_header;
    message->description    = description;
    message->symbology      = symbology;
    message->graphic        = graphic;
    message->tabular        = tabular;

    return 0;

error_invalid_tabular_block_offset:
error_invalid_graphic_block_offset:
error_invalid_symbology_block_offset:
error_invalid_product_description:
error_invalid_message_header:
error_invalid_file_header:
    errno = EINVAL;

    return -1;
}

nexrad_message *nexrad_message_open(const char *path) {
    nexrad_message *message;
    struct stat st;

    if ((message = malloc(sizeof(nexrad_message))) == NULL) {
        goto error_malloc;
    }

    if (stat(path, &st) < 0) {
        goto error_stat;
    }

    message->size        = st.st_size;
    message->page_size   = (size_t)sysconf(_SC_PAGESIZE);
    message->mapped_size = _mapped_size(st.st_size, message->page_size);

    if ((message->fd = open(path, O_RDONLY)) < 0) {
        goto error_open;
    }

    if ((message->data = mmap(NULL, message->mapped_size, PROT_READ, MAP_PRIVATE, message->fd, 0)) == NULL) {
        goto error_mmap;
    }

    if (_index_message(message) < 0) {
        goto error_index_message;
    }

    return message;

error_index_message:
    munmap(message->data, message->mapped_size);

error_mmap:
    close(message->fd);

error_open:
    free(message);

error_stat:
error_malloc:
    return NULL;
}

void nexrad_message_close(nexrad_message *message) {
    if (message == NULL) {
        return;
    }

    if (message->data && message->mapped_size > 0) {
        munmap(message->data, message->mapped_size);

        message->data        = 0;
        message->mapped_size = 0;
    }

    if (message->fd) {
        close(message->fd);

        message->fd = 0;
    }

    message->size           = 0;
    message->page_size      = 0;
    message->file_header    = NULL;
    message->message_header = NULL;
    message->description    = NULL;
    message->symbology      = NULL;
    message->graphic        = NULL;
    message->tabular        = NULL;

    free(message);

    return;
}

static nexrad_chunk *block_read_layer(nexrad_chunk *block, enum nexrad_chunk_type_id type) {
    void *layer;

    if ((layer = nexrad_chunk_read(block, NULL, NULL, NULL)) == NULL) {
        goto error_chunk_read;
    }

    return nexrad_chunk_open(layer, type);

error_chunk_read:
    return NULL;
}

nexrad_chunk *nexrad_symbology_block_open(nexrad_message *message) {
    return nexrad_chunk_open(message->symbology, NEXRAD_CHUNK_SYMBOLOGY_BLOCK);
}

nexrad_chunk *nexrad_symbology_block_read_layer(nexrad_chunk *block) {
    return block_read_layer(block, NEXRAD_CHUNK_SYMBOLOGY_LAYER);
}

nexrad_packet *nexrad_symbology_layer_read_packet(nexrad_chunk *layer, size_t *total_size, size_t *data_size, void **data) {
    return nexrad_chunk_read(layer, total_size, data_size, data);
}

void nexrad_symbology_layer_close(nexrad_chunk *layer) {
    nexrad_chunk_close(layer);
}

void nexrad_symbology_block_close(nexrad_chunk *block) {
    nexrad_chunk_close(block);
}

nexrad_chunk *nexrad_graphic_block_open(nexrad_message *message) {
    return nexrad_chunk_open(message->graphic, NEXRAD_CHUNK_GRAPHIC_BLOCK);
}

nexrad_chunk *nexrad_graphic_block_read_page(nexrad_chunk *block) {
    return block_read_layer(block, NEXRAD_CHUNK_GRAPHIC_PAGE);
}

nexrad_packet *nexrad_graphic_page_read_packet(nexrad_chunk *page, size_t *total_size, size_t *data_size, void **data) {
    return nexrad_chunk_read(page, total_size, data_size, data);
}

void nexrad_graphic_page_close(nexrad_chunk *page) {
    nexrad_chunk_close(page);
}

void nexrad_graphic_block_close(nexrad_chunk *block) {
    nexrad_chunk_close(block);
}

nexrad_text *nexrad_tabular_block_open(nexrad_message *message) {
    nexrad_text *block;

    if (message == NULL) return NULL;

    if ((block = malloc(sizeof(*block))) == NULL) {
        goto error_malloc;
    }

    block->current    = (char *)message->tabular + sizeof(nexrad_tabular_block);
    block->page       = 1;
    block->line       = 1;
    block->pages_left = be16toh(message->tabular->pages);
    block->bytes_left = be32toh(message->tabular->header.size);

    return block;

error_malloc:
    return NULL;
}

#define NEXRAD_TABULAR_BLOCK_MAX_LINE_SIZE 80

ssize_t nexrad_tabular_block_read_line(nexrad_text *block, char **data, int *page, int *line) {
    size_t chars;

    if (block == NULL) return -1;

    /*
     * Return a zero if there are no pages or bytes left in the tabular block.
     */
    if (block->pages_left == 0 || block->bytes_left == 0) {
        return 0;
    }

    /*
     * Check and see if we have arrived at an end-of-page flag.  Also, if we
     * have finished reading the final page, return 0 to indicate the last line
     * has been read.
     */
    if ((int16_t)be16toh(*((int16_t *)block->current)) == -1) {
        block->page++;
        block->line = 1;

        if (block->pages_left-- == 0) {
            return 0;
        }

        /*
         * Increment the current pointer beyond the end-of-page flag.
         */
        block->current += sizeof(int16_t);
    }

    /*
     * Detect the number of characters in the current line.  If this number
     * seems implausible, then return -1 to indicate an error.
     */
    if ((chars = (size_t)be16toh(*((uint16_t *)block->current))) > NEXRAD_TABULAR_BLOCK_MAX_LINE_SIZE) {
        return -1;
    }

    /*
     * If the caller has provided a pointer to an address to store the start of
     * the text line, then populate that.
     */
    if (data != NULL) {
        *data = block->current + sizeof(uint16_t);
    }

    /*
     * If the caller has provided a pointer to an address to store the current
     * page number, then supply that.
     */
    if (page != NULL) {
        *page = block->page;
    }

    /*
     * If the caller has provided a pointer to an address to store the current
     * line number, then supply that.
     */
    if (line != NULL) {
        *line = block->line;
    }

    /*
     * Increment the current line number.
     */
    block->line++;

    /*
     * Advance the current data pointer beyond the current line.
     */
    block->current += sizeof(uint16_t) + chars;

    /*
     * Return the number of characters in the current line.
     */
    return chars;
}

void nexrad_tabular_block_close(nexrad_text *block) {
    if (block == NULL) return;

    block->current    = NULL;
    block->page       = 0;
    block->line       = 0;
    block->pages_left = 0;
    block->bytes_left = 0;

    free(block);
}
