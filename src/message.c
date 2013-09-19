#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <endian.h>
#include "util.h"

#include <nexrad/message.h>

struct _nexrad_message {
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
};

static inline int _mapped_size(size_t size, size_t page_size) {
    return size + (page_size - (size % page_size));
}

static inline off_t _halfword_offset(uint32_t value) {
    return sizeof(nexrad_file_header) + (be32toh(value) * 2);
}

static inline void *_block_pointer(nexrad_message *message, uint32_t raw_offset, enum nexrad_block_id type) {
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

nexrad_chunk *nexrad_message_open_graphic_block(nexrad_message *message) {
    return nexrad_graphic_block_open(message->graphic);
}

nexrad_tabular_text *nexrad_message_open_tabular_block(nexrad_message *message) {
    return nexrad_tabular_block_open(message->tabular);
}

nexrad_symbology_block *nexrad_message_get_symbology_block(nexrad_message *message) {
    if (message == NULL) {
        return NULL;
    }

    return message->symbology;
}

nexrad_graphic_block *nexrad_message_get_graphic_block(nexrad_message *message) {
    if (message == NULL) {
        return NULL;
    }

    return message->graphic;
}

nexrad_tabular_block *nexrad_message_get_tabular_block(nexrad_message *message) {
    if (message == NULL) {
        return NULL;
    }

    return message->tabular;
}

time_t nexrad_message_get_scan_timestamp(nexrad_message *message) {
    if (message == NULL) {
        return -1;
    }

    return nexrad_date_timestamp(&message->description->scan_date);
}

time_t nexrad_message_get_gen_timestamp(nexrad_message *message) {
    if (message == NULL) {
        return -1;
    }

    return nexrad_date_timestamp(&message->description->gen_date);
}

int nexrad_message_get_product_id(nexrad_message *message) {
    if (message == NULL) {
        return -1;
    }

    return be16toh(message->description->product_id);
}

int nexrad_message_find_product_code(nexrad_message *message, char **code, size_t *len) {
    if (message == NULL) return -1;

    if (code && len) {
        *code = message->file_header->product_code;
        *len  = sizeof(message->file_header->product_code);
    }

    return 0;
}

int nexrad_message_find_region(nexrad_message *message, char **region, size_t *len) {
    if (message == NULL) return -1;

    if (region && len) {
        *region = message->file_header->region;
        *len    = sizeof(message->file_header->region);
    }

    return 0;
}

int nexrad_message_find_office(nexrad_message *message, char **office, size_t *len) {
    if (message == NULL) return -1;

    if (office && len) {
        *office = message->file_header->office;
        *len    = sizeof(message->file_header->office);
    }

    return 0;
}

int nexrad_message_find_station_suffix(nexrad_message *message, char **suffix, size_t *len) {
    if (message == NULL) {
        return -1;
    }

    if (suffix && len) {
        *suffix = message->file_header->station;
        *len    = sizeof(message->file_header->station);
    }

    return 0;
}

int nexrad_message_read_station(nexrad_message *message, char *dest, size_t destlen) {
    char station[5];

    if (message == NULL) {
        return -1;
    }

    station[0] = message->file_header->office[0];
    memcpy(station + 1, message->file_header->station, 3);
    station[4] = '\0';

    return safecpy(
        dest,    station,
        destlen, sizeof(station) - 1
    );
}

int nexrad_message_read_station_coords(nexrad_message *message, double *lat, double *lon) {
    if (message == NULL || lat == NULL || lon == NULL) {
        return -1;
    }

    *lat = NEXRAD_PRODUCT_COORD_MAGNITUDE * (int32_t)be32toh(message->description->station_lat);
    *lon = NEXRAD_PRODUCT_COORD_MAGNITUDE * (int32_t)be32toh(message->description->station_lon);

    return 0;
}
