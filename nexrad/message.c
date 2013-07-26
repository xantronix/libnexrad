#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <endian.h>

#include <nexrad/message.h>

static inline int _mapped_size(size_t size, size_t page_size) {
    return size + (page_size - (size % page_size));
}

static inline off_t _offset_from_halfword(uint16_t value) {
    return sizeof(nexrad_file_header) + (value * 2);
}

static inline void *_symbology_block(nexrad_message_header *header, nexrad_product_description *description) {
    uint16_t offset = be16toh(description->symbology_offset);

    return (nexrad_symbology_block *)((void *)header) + offset;
}

static inline void *_graphic_block(nexrad_message_header *header, nexrad_product_description *description) {
    uint16_t offset = be16toh(description->graphic_offset);

    return (nexrad_graphic_block *)((void *)header) + offset;
}

static inline void *_tabular_block(nexrad_message_header *header, nexrad_product_description *description) {
    uint16_t offset = be16toh(description->tabular_offset);

    return (nexrad_tabular_block *)((void *)header) + offset;
}

static int _index_message(nexrad_message *message) {
    nexrad_file_header *file_header = message->data;

    nexrad_message_header *      message_header;
    nexrad_product_description * description;
    nexrad_symbology_block *     symbology;
    nexrad_graphic_block *       graphic;
    nexrad_tabular_block *       tabular;

    if (file_header->_whitespace1 != ' ') {
        goto error_invalid_file_header;
    }

    message_header = (nexrad_message_header *)((void *)(file_header) + sizeof(nexrad_file_header));

    if (be16toh(message_header->blocks) > 5) {
        goto error_invalid_message_header;
    }

    description = (nexrad_product_description *)(((void *)message_header) + sizeof(nexrad_message_header));

    symbology = _symbology_block(message_header, description);
    graphic   = _graphic_block(message_header, description);
    tabular   = _tabular_block(message_header, description);

    message->file_header    = file_header;
    message->message_header = message_header;
    message->description    = description;
    message->symbology      = symbology;
    message->graphic        = graphic;
    message->tabular        = tabular;

    return 0;

error_invalid_message_header:
error_invalid_file_header:
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
