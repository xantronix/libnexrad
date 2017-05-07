/*
 * Copyright (c) 2016 Dynamic Weather Solutions, Inc. Distributed under the
 * terms of the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <bzlib.h>
#include "util.h"

#include <nexrad/message.h>

#define NEXRAD_MESSAGE_UNKNOWN_FOOTER "\x0d\x0d\x0a\x03"

struct _nexrad_message {
    size_t size;
    size_t page_size;
    size_t mapped_size;
    int    fd;
    void * data;
    void * body;

    nexrad_unknown_header *      unknown_header;
    nexrad_wmo_header *          wmo_header;
    nexrad_message_header *      message_header;
    nexrad_product_description * description;
    nexrad_symbology_block *     symbology;
    nexrad_graphic_block *       graphic;
    nexrad_tabular_block *       tabular;

    enum nexrad_product_compression_type compression;
};

static inline int _header_size() {
    return sizeof(nexrad_message_header) + sizeof(nexrad_product_description);
}

static inline int _mapped_size(size_t size, size_t page_size) {
    return size + (page_size - (size % page_size));
}

static inline off_t _halfword_body_offset(uint32_t value) {
    return (be32toh(value) * 2) - _header_size();
}

static void *_block_pointer(nexrad_message *message, uint32_t raw_offset, enum nexrad_block_id type) {
    uint32_t offset = _halfword_body_offset(raw_offset);
    nexrad_block_header *header;

    /*
     * Prevent an opportunity for segmentation fault by limiting the message
     * offset to exist within the physical size of the message.
     */
    if (offset + _header_size() > message->size) {
        return NULL;
    }

    /*
     * If the block divider and ID found at the usual locations appear to be
     * invalid, then return null.
     */
    header = (nexrad_block_header *)((char *)message->body + offset);

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

static size_t _message_get_body_size(nexrad_message *message) {
    size_t ret = message->size;

    if (message->unknown_header)
        ret -= sizeof(nexrad_unknown_header);

    if (message->wmo_header)
        ret -= sizeof(nexrad_wmo_header);

    ret -= sizeof(nexrad_message_header);
    ret -= sizeof(nexrad_product_description);

    return ret;
}

static void *_message_get_body(nexrad_message *message, nexrad_product_description *description, enum nexrad_product_compression_type *compp) {
    enum nexrad_product_compression_type compression;
    void *dest;

    /*
     * Locate the body of the NEXRAD data after the message header and product
     * description blocks.
     */
    void *body = nexrad_block_after(description, nexrad_product_description);

    /*
     * If the current product does not support compression, then simply return
     * the current body pointer.
     */
    if (!nexrad_product_type_supports_compression(be16toh(description->type))) {
        return body;
    }

    compression = be16toh(
        description->attributes.compression.method
    );

    switch (compression) {
        case NEXRAD_PRODUCT_COMPRESSION_NONE: {
            break;
        }

        case NEXRAD_PRODUCT_COMPRESSION_BZIP2: {
            unsigned int destlen = be32toh(description->attributes.compression.size);
            size_t bodylen = _message_get_body_size(message);

            if (destlen > NEXRAD_MESSAGE_MAX_BODY_SIZE) {
                goto error_decompress_size;
            }

            if ((dest = malloc(destlen)) == NULL) {
                goto error_decompress_malloc;
            }

            if (BZ2_bzBuffToBuffDecompress(dest, &destlen, body, bodylen, 0, 0) < 0) {
                goto error_decompress;
            }

            body = dest;

            break;
        }

        /*
         * If some unsupported compression scheme is indicated here, then return
         * null.
         */
        default: {
            return NULL;
        }
    }

    *compp = compression;

    return body;

error_decompress:
    free(dest);

error_decompress_malloc:
error_decompress_size:
    return NULL;
}

/*
 * Perform an initial parse of the NEXRAD Radar Product Generator Message and
 * produce a high-level table-of-contents indicating the locations of the five
 * blocks within the message.
 */
static int _message_index(nexrad_message *message) {
    nexrad_message_header *      message_header;
    nexrad_product_description * description;

    enum nexrad_product_compression_type compression = NEXRAD_PRODUCT_COMPRESSION_NONE;

    nexrad_symbology_block * symbology = NULL;
    nexrad_graphic_block *   graphic   = NULL;
    nexrad_tabular_block *   tabular   = NULL;

    size_t message_offset        = 0;
    size_t message_size_expected = message->size;

    message->unknown_header = NULL;
    message->wmo_header     = NULL;
    message->body           = NULL;
    message->compression    = NEXRAD_PRODUCT_COMPRESSION_NONE;

    if (memcmp((char *)message->data + message_offset, NEXRAD_HEADER_UNKNOWN_SIGNATURE, 4) == 0) {
        message->unknown_header = (nexrad_unknown_header *)((char *)message->data + message_offset);

        message_offset += sizeof(nexrad_unknown_header);
    }

    if (memcmp((char *)message->data + message_offset, NEXRAD_HEADER_WMO_SIGNATURE, 4) == 0) {
        message->wmo_header = (nexrad_wmo_header *)((char *)message->data + message_offset);

        if (message->wmo_header->_whitespace1 != ' ') {
            goto error_invalid_message_header;
        }

        message_offset += sizeof(nexrad_wmo_header);
    } else {
        goto error_invalid_message_header;
    }

    message_size_expected -= message_offset;

    if (memcmp((char *)message->data + message->size - 4, NEXRAD_MESSAGE_UNKNOWN_FOOTER, 4) == 0) {
        message_size_expected -= 4;
    }

    if (message_size_expected < sizeof(nexrad_message_header)) {
        goto error_invalid_message_header;
    }

    message_header = (nexrad_message_header *)((char *)message->data + message_offset);

    if (be16toh(message_header->blocks) > 5) {
        goto error_invalid_message_header;
    }

    if (be32toh(message_header->size) != message_size_expected) {
        goto error_invalid_product_description;
    }

    description = (nexrad_product_description *)nexrad_block_after(message_header, nexrad_message_header);

    if ((int16_t)be16toh(description->divider) != -1) {
        goto error_invalid_product_description;
    }

    if ((message->body = _message_get_body(message, description, &compression)) == NULL) {
        goto error_message_get_body;
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

    message->message_header = message_header;
    message->description    = description;
    message->compression    = compression;
    message->symbology      = symbology;
    message->graphic        = graphic;
    message->tabular        = tabular;

    return 0;

error_invalid_tabular_block_offset:
error_invalid_graphic_block_offset:
error_invalid_symbology_block_offset:
error_message_get_body:
error_invalid_product_description:
error_invalid_message_header:
    errno = EINVAL;

    return -1;
}

nexrad_message *nexrad_message_open_buf(void *buf, size_t len) {
    nexrad_message *message;

    if ((message = malloc(sizeof(nexrad_message))) == NULL) {
        goto error_malloc;
    }

    message->size        = len;
    message->page_size   = 0;
    message->mapped_size = 0;
    message->fd          = 0;
    message->data        = buf;

    if (_message_index(message) < 0) {
        goto error_message_index;
    }

    return message;

error_message_index:
    nexrad_message_destroy(message);

error_malloc:
    return NULL;
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

    if (st.st_size > NEXRAD_MESSAGE_MAX_SIZE) {
        errno = EFBIG;

        goto error_efbig;
    }

    if (st.st_size < sizeof(nexrad_message)) {
        errno = EINVAL;

        goto error_einval;
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

    if (_message_index(message) < 0) {
        goto error_message_index;
    }

    return message;

error_message_index:
error_mmap:
error_open:
    nexrad_message_destroy(message);

    return NULL;

error_einval:
error_efbig:
error_stat:
    free(message);

error_malloc:
    return NULL;
}

void nexrad_message_destroy(nexrad_message *message) {
    if (message == NULL) {
        return;
    }

    if (message->data && message->mapped_size > 0) {
        munmap(message->data, message->mapped_size);

        message->data        = NULL;
        message->mapped_size = 0;
    }

    if (message->fd) {
        close(message->fd);

        message->fd = 0;
    }

    if (message->body && message->compression != NEXRAD_PRODUCT_COMPRESSION_NONE) {
        message->compression = NEXRAD_PRODUCT_COMPRESSION_NONE;
        free(message->body);
    }

    message->size           = 0;
    message->page_size      = 0;
    message->body           = NULL;
    message->unknown_header = NULL;
    message->wmo_header     = NULL;
    message->message_header = NULL;
    message->description    = NULL;
    message->symbology      = NULL;
    message->graphic        = NULL;
    message->tabular        = NULL;

    free(message);

    return;
}

void nexrad_message_close(nexrad_message *message) {
    nexrad_message_destroy(message);
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

nexrad_message_header *nexrad_message_get_header(nexrad_message *message) {
    if (message == NULL) {
        return NULL;
    }

    return message->message_header;
}

nexrad_product_description *nexrad_message_get_product_description(nexrad_message *message) {
    if (message == NULL) {
        return NULL;
    }

    return message->description;
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

int nexrad_message_get_product_type(nexrad_message *message) {
    if (message == NULL) {
        return -1;
    }

    return be16toh(message->message_header->product_type);
}

int nexrad_message_find_product_code(nexrad_message *message, char **code, size_t *len) {
    if (message == NULL) return -1;

    if (code && len) {
        *code = message->wmo_header->product_code;
        *len  = sizeof(message->wmo_header->product_code);
    }

    return 0;
}

int nexrad_message_find_region(nexrad_message *message, char **region, size_t *len) {
    if (message == NULL) return -1;

    if (region && len) {
        *region = message->wmo_header->region;
        *len    = sizeof(message->wmo_header->region);
    }

    return 0;
}

int nexrad_message_find_office(nexrad_message *message, char **office, size_t *len) {
    if (message == NULL) return -1;

    if (office && len) {
        *office = message->wmo_header->office;
        *len    = sizeof(message->wmo_header->office);
    }

    return 0;
}

char nexrad_message_get_station_prefix(nexrad_message *message) {
    if (message == NULL) {
        return '\0';
    }

    return message->wmo_header->office[0];
}

int nexrad_message_find_station_suffix(nexrad_message *message, char **suffix, size_t *len) {
    if (message == NULL) {
        return -1;
    }

    if (suffix && len) {
        *suffix = message->wmo_header->station;
        *len    = sizeof(message->wmo_header->station);
    }

    return 0;
}

int nexrad_message_read_station(nexrad_message *message, char *dest, size_t destlen) {
    char station[5];

    if (message == NULL) {
        return -1;
    }

    station[0] = message->wmo_header->office[0];
    memcpy(station + 1, message->wmo_header->station, 3);
    station[4] = '\0';

    return safecpy(
        dest,    station,
        destlen, sizeof(station) - 1
    );
}

void nexrad_message_read_station_location(nexrad_message *message, nexrad_map_point *point, float *alt) {
    point->lat = NEXRAD_PRODUCT_COORD_MAGNITUDE * (int32_t)be32toh(message->description->station_lat);
    point->lon = NEXRAD_PRODUCT_COORD_MAGNITUDE * (int32_t)be32toh(message->description->station_lon);

    if (alt)
        *alt = NEXRAD_PRODUCT_ALT_FACTOR * (int16_t)be16toh(message->description->station_altitude);
}

void nexrad_message_read_tilt(nexrad_message *message, float *tilt) {
    *tilt = 0.1f *
        (int16_t)be16toh(message->description->attributes.generic.tilt);
}

nexrad_packet *nexrad_message_find_symbology_packet_by_type(nexrad_message *message, enum nexrad_packet_type type) {
    nexrad_symbology_block *symbology;
    nexrad_chunk *block, *layer;
    nexrad_packet *ret = NULL;

    if (message == NULL) {
        return NULL;
    }

    if ((symbology = nexrad_message_get_symbology_block(message)) == NULL) {
        goto error_get_symbology_block;
    }

    if ((block = nexrad_symbology_block_open(symbology)) == NULL) {
        goto error_symbology_block_open;
    }

    while ((layer = nexrad_symbology_block_read_layer(block)) != NULL) {
        nexrad_packet *packet;
        size_t size;

        while ((packet = nexrad_symbology_layer_peek_packet(layer, &size)) != NULL) {
            if (nexrad_packet_get_type(packet) != type) {
                nexrad_symbology_layer_next_packet(layer, size);

                continue;
            }

            ret = packet;

            nexrad_symbology_layer_close(layer);

            goto done;
        }

        nexrad_symbology_layer_close(layer);
    }

done:
    nexrad_symbology_block_close(block);

    return ret;

error_symbology_block_open:
error_get_symbology_block:
    return NULL;
}
