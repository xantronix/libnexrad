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
#include <errno.h>
#include "util.h"

#include <nexrad/raster.h>

struct _nexrad_raster {
    nexrad_raster_packet * packet;
    size_t                 bytes_read;
    uint16_t               lines_left;
    nexrad_raster_line *   current;
};

static int _valid_packet(nexrad_raster_packet *packet) {
    if (packet == NULL) {
        return 0;
    }

    switch (be16toh(packet->type)) {
        case 0xba0f:
        case 0xba07: {
            break;
        }

        default: {
            return 0;
        }
    }

    if (
               be16toh(packet->flags_1) != 0x8000 ||
               be16toh(packet->flags_2) != 0x00c0 ||
      (int16_t)be16toh(packet->i)        >   2047 ||
      (int16_t)be16toh(packet->i)        <  -2048 ||
      (int16_t)be16toh(packet->j)        >   2047 ||
      (int16_t)be16toh(packet->j)        <  -2048 ||
               be16toh(packet->x_scale)  <      1 ||
               be16toh(packet->x_scale)  >     67 ||
               be16toh(packet->y_scale)  <      1 ||
               be16toh(packet->y_scale)  >     67 ||
               be16toh(packet->lines)    <      1 ||
               be16toh(packet->lines)    >    464
    ) {
        return 0;
    }

    return 1;
}

nexrad_raster *nexrad_raster_packet_open(nexrad_raster_packet *packet) {
    nexrad_raster *raster;

    if (!_valid_packet(packet)) {
        return NULL;
    }

    if ((raster = malloc(sizeof(nexrad_raster))) == NULL) {
        goto error_malloc;
    }

    raster->packet     = packet;
    raster->bytes_read = sizeof(nexrad_raster_packet);
    raster->lines_left = be16toh(packet->lines);
    raster->current    = (nexrad_raster_line *)((char *)packet + sizeof(nexrad_raster_packet));

    return raster;

error_malloc:
    return NULL;
}

static uint16_t _raster_line_width(nexrad_raster_line *line) {
    nexrad_raster_run *runs = (nexrad_raster_run *)((char *)line + sizeof(nexrad_raster_line));

    uint16_t width = 0;
    uint16_t i, nruns = be16toh(line->runs);

    for (i=0; i<nruns; i++) {
        width += runs[i].length;
    }

    return width;
}

nexrad_raster_line *nexrad_raster_read_line(nexrad_raster *raster, void **data, uint16_t *runsp) {
    nexrad_raster_line *line;
    uint16_t runs;

    if (raster == NULL) {
        return NULL;
    }

    if (raster->lines_left == 0) {
        return NULL;
    }

    line = raster->current;
    runs = sizeof(nexrad_raster_line) + be16toh(line->runs);

    /*
     * Advance the current line pointer beyond the line to follow.
     */
    raster->current = (nexrad_raster_line *)((char *)line + runs);

    /*
     * Increase the number of bytes read in the current raster packet.
     */
    raster->bytes_read += runs;

    /*
     * Decrement the number of lines left in the current raster packet.
     */
    raster->lines_left--;

    /*
     * If the caller provided a pointer to an address to store the resultant
     * raster line run count, then populate that address with that value.
     */
    if (runsp)
        *runsp = runs;

    /*
     * If the caller provided a pointer to an address to store a pointer to
     * the raster line's RLE-encoded data, then provide that address.
     */
    if (data)
        *data = (char *)line + sizeof(nexrad_raster_line);

    return line;
}

size_t nexrad_raster_bytes_read(nexrad_raster *raster) {
    if (raster == NULL) {
        return 0;
    }

    return raster->bytes_read;
}

void nexrad_raster_close(nexrad_raster *raster) {
    if (raster == NULL) {
        return;
    }

    raster->packet     = NULL;
    raster->bytes_read = 0;
    raster->lines_left = 0;
    raster->current    = NULL;

    free(raster);
}

int nexrad_raster_get_info(nexrad_raster *raster, uint16_t *widthp, uint16_t *heightp) {
    if (raster == NULL) {
        return -1;
    }

    /*
     * If we cannot identify that there is at least one raster line available
     * in the current raster packet, then do not attempt to populate data.
     */
    if (be16toh(raster->packet->lines) < 1) {
        return -1;
    }

    if (widthp) {
        nexrad_raster_line *line = (nexrad_raster_line *)((char *)raster->packet + sizeof(nexrad_raster_packet));

        *widthp = _raster_line_width(line);
    }

    if (heightp)
        *heightp = be16toh(raster->packet->lines);

    return 0;
}

static int _raster_unpack_rle(nexrad_raster *raster, nexrad_image *image, nexrad_color *entries) {
    nexrad_raster_line *line;
    nexrad_raster_run  *data;
    unsigned char *buf;

    uint16_t y = 0;
    uint16_t runs;
    uint16_t width, height;

    if ((buf = nexrad_image_get_buf(image, NULL)) == NULL) {
        goto error_image_get_buf;
    }

    if (nexrad_image_get_info(image, &width, &height) < 0) {
        goto error_image_get_info;
    }

    while ((line = nexrad_raster_read_line(raster, (void **)&data, &runs)) != NULL) {
        uint16_t r, x = 0;

        for (r=0; r<runs; r++) {
            uint8_t level  = data[r].level * NEXRAD_RASTER_RLE_FACTOR;
            uint8_t length = data[r].length;

            nexrad_color color = entries[level];

            if (color.a)
                nexrad_image_draw_run(image, color, x, y, length);

            x += length;

            if (x >= width) break;
        }

        y++;

        if (y >= height) break;
    }

    return 0;

error_image_get_info:
error_image_get_buf:
    return -1;
}

nexrad_image *nexrad_raster_create_image(nexrad_raster *raster, nexrad_color_table *table) {
    nexrad_image *image;
    nexrad_color *entries;
    uint16_t width, height;

    if (raster == NULL || table == NULL) {
        return NULL;
    }

    if (nexrad_raster_get_info(raster, &width, &height) < 0) {
        goto error_raster_get_info;
    }

    if ((entries = nexrad_color_table_get_entries(table, NULL)) == NULL) {
        goto error_color_table_get_entries;
    }

    if ((image = nexrad_image_create(width, height)) == NULL) {
        goto error_image_create;
    }

    if (_raster_unpack_rle(raster, image, entries) < 0) {
        goto error_image_unpack;
    }

    return image;

error_image_unpack:
    nexrad_image_destroy(image);

error_image_create:
error_color_table_get_entries:
error_raster_get_info:
    return NULL;
}
