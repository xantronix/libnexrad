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

static enum nexrad_chunk_type_id nexrad_parent_chunk_types[] = {
    /* none                          => */ 0,
    /* NEXRAD_CHUNK_SYMBOLOGY_BLOCK  => */ 0,
    /* NEXRAD_CHUNK_GRAPHIC_BLOCK    => */ 0,
    /* NEXRAD_CHUNK_TABULAR_BLOCK    => */ 0,
    /* NEXRAD_CHUNK_SYMBOLOGY_LAYER  => */ NEXRAD_CHUNK_SYMBOLOGY_BLOCK,
    /* NEXRAD_CHUNK_SYMBOLOGY_PACKET => */ NEXRAD_CHUNK_SYMBOLOGY_LAYER,
    /* NEXRAD_CHUNK_GRAPHIC_PAGE     => */ NEXRAD_CHUNK_GRAPHIC_BLOCK,
    /* NEXRAD_CHUNK_GRAPHIC_PACKET   => */ NEXRAD_CHUNK_GRAPHIC_PAGE,
    /* NEXRAD_CHUNK_TABULAR_PAGE     => */ NEXRAD_CHUNK_TABULAR_BLOCK,
    /* NEXRAD_CHUNK_TABULAR_PACKET   => */ NEXRAD_CHUNK_TABULAR_PAGE
};

static enum nexrad_chunk_type_id nexrad_child_chunk_types[] = {
    /* none                          => */ 0,
    /* NEXRAD_CHUNK_SYMBOLOGY_BLOCK  => */ NEXRAD_CHUNK_SYMBOLOGY_LAYER,
    /* NEXRAD_CHUNK_GRAPHIC_BLOCK    => */ NEXRAD_CHUNK_GRAPHIC_PAGE,
    /* NEXRAD_CHUNK_TABULAR_BLOCK    => */ NEXRAD_CHUNK_TABULAR_PAGE,
    /* NEXRAD_CHUNK_SYMBOLOGY_LAYER  => */ NEXRAD_CHUNK_SYMBOLOGY_PACKET,
    /* NEXRAD_CHUNK_SYMBOLOGY_PACKET => */ 0,
    /* NEXRAD_CHUNK_GRAPHIC_PAGE     => */ NEXRAD_CHUNK_GRAPHIC_PACKET,
    /* NEXRAD_CHUNK_GRAPHIC_PACKET   => */ 0,
    /* NEXRAD_CHUNK_TABULAR_PAGE     => */ NEXRAD_CHUNK_TABULAR_PACKET,
    /* NEXRAD_CHUNK_TABULAR_PACKET   => */ 0
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
    /* NEXRAD_CHUNK_GRAPHIC_PACKET   => */ sizeof(nexrad_packet_header),
    /* NEXRAD_CHUNK_TABULAR_PAGE     => */ sizeof(nexrad_tabular_page),
    /* NEXRAD_CHUNK_TABULAR_PACKET   => */ sizeof(nexrad_packet_header)
};

ssize_t nexrad_chunk_size(void *chunk, enum nexrad_chunk_type_id type) {
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
        case NEXRAD_CHUNK_GRAPHIC_PACKET:
        case NEXRAD_CHUNK_TABULAR_PACKET: {
            nexrad_packet_header *header = chunk;

            return be16toh(header->size);
        }

        case NEXRAD_CHUNK_SYMBOLOGY_LAYER: {
            nexrad_symbology_layer *header = chunk;

            if ((int16_t)be16toh(header->divider) != -1) goto error_bad_header;

            return be16toh(header->size);
        }

        case NEXRAD_CHUNK_GRAPHIC_PAGE:
        case NEXRAD_CHUNK_TABULAR_PAGE: {
            nexrad_graphic_page *header = chunk;

            return be16toh(header->size);
        }
    }

error_bad_header:
    return -1;
}

nexrad_chunk *nexrad_chunk_open(void *chunk, enum nexrad_chunk_type_id type) {
    nexrad_chunk *iterator;
    size_t size;

    if ((size = nexrad_chunk_size(chunk, type)) < 0) {
        goto error_bad_chunk;
    }

    if ((chunk = malloc(sizeof(*chunk))) == NULL) {
        goto error_malloc;
    }

    iterator->parent_type_id  = type;
    iterator->child_type_id   = nexrad_child_chunk_types[type];
    iterator->parent          = chunk;
    iterator->first           = chunk + nexrad_chunk_header_sizes[type];
    iterator->current         = NULL;
    iterator->bytes_remaining = size;

    return iterator;

error_malloc:
error_bad_chunk:
    return NULL;
}

void *nexrad_chunk_read(nexrad_chunk *iterator, size_t *size) {
    if (iterator == NULL) return NULL;

    /*
     * If there are no more bytes remaining, then return null.
     */
    if (iterator->bytes_remaining == 0) {
        return NULL;
    }

    if (iterator->current == NULL) {
        /*
         * If the current chunk is null, then set it to the first.
         */
        iterator->current = iterator->first;
    } else {
        /*
         * Otherwise, update the current chunk to a pointer after the previously
         * set current.
         */
        iterator->current += nexrad_chunk_size(iterator->current, iterator->child_type_id);
    }

    /*
     * Decrement the number of bytes remaining appropriately.
     */
    iterator->bytes_remaining -= nexrad_chunk_size(iterator->current, iterator->child_type_id);

    /*
     * Return the current chunk.
     */
    return iterator->current;
}

void nexrad_chunk_close(nexrad_chunk *iterator) {
    if (iterator == NULL) return;

    iterator->parent_type_id  = 0;
    iterator->child_type_id   = 0;
    iterator->parent          = NULL;
    iterator->first           = NULL;
    iterator->current         = NULL;
    iterator->bytes_remaining = 0;

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

    return ((void *)message->data) + offset;
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
