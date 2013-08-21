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

static inline int _mapped_size(size_t size, size_t page_size) {
    return size + (page_size - (size % page_size));
}

static inline off_t _halfword_offset(uint32_t value) {
    return sizeof(nexrad_file_header) + (be32toh(value) * 2);
}

static inline void *_block_pointer(nexrad_message *message, uint32_t raw_offset, uint16_t type) {
    uint32_t offset = _halfword_offset(raw_offset);
    nexrad_block_header *header;

    /*
     * Prevent an opportunity for segmentation fault by limiting the message
     * offset to exist within the physical size of the message.
     */
    if (offset > message->size) {
        return NULL;
    }

    /*
     * If the block divider and ID found at the usual locations appear to be
     * invalid, then return null.
     */
    header = (nexrad_block_header *)((char *)message->data + offset);

    if ((int16_t)be16toh(header->divider) != -1 || be16toh(header->id) != type) {
        return NULL;
    }

    return (void *)header;
}

static inline nexrad_symbology_block *_symbology_block(nexrad_message *message, nexrad_product_description *description) {
    return (nexrad_symbology_block *)_block_pointer(message, description->symbology_offset, NEXRAD_BLOCK_SYMBOLOGY);
}

static inline nexrad_graphic_block *_graphic_block(nexrad_message *message, nexrad_product_description *description) {
    return (nexrad_graphic_block *)_block_pointer(message, description->graphic_offset, NEXRAD_BLOCK_GRAPHIC);
}

static inline nexrad_tabular_block *_tabular_block(nexrad_message *message, nexrad_product_description *description) {
    return (nexrad_tabular_block *)_block_pointer(message, description->tabular_offset, NEXRAD_BLOCK_TABULAR);
}

/*
 * Perform an initial parse of the NEXRAD Radar Product Generator Message and
 * produce a high-level table-of-contents indicating the locations of the five
 * blocks within the message.
 */
static int _index_message(nexrad_message *message) {
    nexrad_file_header *file_header = message->data;

    nexrad_message_header *      message_header;
    nexrad_product_description * description;

    nexrad_symbology_block * symbology = NULL;
    nexrad_graphic_block *   graphic   = NULL;
    nexrad_tabular_block *   tabular   = NULL;

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

    if (message->size != sizeof(nexrad_file_header) + be32toh(message_header->size)) {
        goto error_invalid_product_description;
    }

    description = (nexrad_product_description *)nexrad_block_after(message_header, nexrad_message_header);

    if ((int16_t)be16toh(description->divider) != -1) {
        goto error_invalid_product_description;
    }

    if (description->symbology_offset != 0 && (symbology = _symbology_block(message, description)) == NULL) {
        goto error_invalid_symbology_block_offset;
    }

    if (description->graphic_offset != 0 && (graphic = _graphic_block(message, description)) == NULL) {
        goto error_invalid_graphic_block_offset;
    }

    if (description->tabular_offset != 0 && (tabular = _tabular_block(message, description)) == NULL) {
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

nexrad_chunk *nexrad_message_open_symbology_block(nexrad_message *message) {
    return nexrad_symbology_block_open(message->symbology);
}

nexrad_chunk *nexrad_graphic_block_open(nexrad_graphic_block *block) {
    return nexrad_chunk_open(block, NEXRAD_CHUNK_GRAPHIC_BLOCK);
}

nexrad_chunk *nexrad_message_open_graphic_block(nexrad_message *message) {
    return nexrad_graphic_block_open(message->graphic);
}

nexrad_chunk *nexrad_graphic_block_read_page(nexrad_chunk *block) {
    return nexrad_chunk_read_block_layer(block, NEXRAD_CHUNK_GRAPHIC_PAGE);
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

nexrad_tabular_text *nexrad_tabular_block_open(nexrad_tabular_block *block) {
    nexrad_tabular_text *text;

    if (block == NULL) return NULL;

    if ((text = malloc(sizeof(*text))) == NULL) {
        goto error_malloc;
    }

    text->current    = (char *)block + sizeof(nexrad_tabular_block);
    text->page       = 1;
    text->line       = 1;
    text->pages_left = be16toh(block->pages);
    text->bytes_left = be32toh(block->header.size);

    return text;

error_malloc:
    return NULL;
}

nexrad_tabular_text *nexrad_message_open_tabular_block(nexrad_message *message) {
    return nexrad_tabular_block_open(message->tabular);
}

ssize_t nexrad_tabular_block_read_line(nexrad_tabular_text *block, char **data, int *page, int *line) {
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
        block->current = (char *)block->current + sizeof(int16_t);
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
    block->current = (char *)block->current + sizeof(uint16_t) + chars;

    /*
     * Return the number of characters in the current line.
     */
    return chars;
}

void nexrad_tabular_block_close(nexrad_tabular_text *block) {
    if (block == NULL) return;

    block->current    = NULL;
    block->page       = 0;
    block->line       = 0;
    block->pages_left = 0;
    block->bytes_left = 0;

    free(block);
}
