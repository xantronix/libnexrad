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

#include <nexrad/packet.h>
#include <nexrad/raster.h>

static int _valid_packet(nexrad_raster_packet *packet) {
    if (packet == NULL) {
        return 0;
    }

    switch (be16toh(packet->type)) {
        case NEXRAD_PACKET_RASTER_BA0F:
        case NEXRAD_PACKET_RASTER_BA07:
            break;

        default:
            return 0;
    }

    if (       be16toh(packet->flags_1) != 0x8000 ||
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
               be16toh(packet->lines)    >    464) {
        return 0;
    }

    return 1;
}

static size_t _rle_width(nexrad_raster_packet *packet) {
    nexrad_raster_line *line = (nexrad_raster_line *)
        (packet + 1);

    nexrad_raster_run *runs = (nexrad_raster_run *)
        (line + 1);

    size_t count = be16toh(line->runs),
           width = 0,
           i;

    for (i=0; i<count; i++) {
        width += runs[i].length;
    }

    return width;
}

int _unpack_rle(nexrad_raster *raster, nexrad_raster_packet *packet, size_t *bytes_read, size_t max) {
    size_t offset = sizeof(nexrad_raster_packet);

    size_t x, y;

    for (y=0; y<raster->height; y++) {
        nexrad_raster_line *line = (nexrad_raster_line *)
            ((uint8_t *)packet + offset);

        nexrad_raster_run *runs = (nexrad_raster_run *)
            (line + 1);

        size_t count = be16toh(line->runs),
               r;

        offset += sizeof(nexrad_raster_line) + count;

        if (offset > max) {
            goto error_unexpected;
        }

        for (r=0, x=0; r<count; r++) {
            size_t i;

            for (i=0; i<runs[r].length; i++) {
                ((uint8_t *)(raster + 1))[y*raster->width+x] =
                    NEXRAD_RASTER_RLE_FACTOR * runs[r].level;

                x++;
            }
        }
    }

    if (bytes_read != NULL) {
        *bytes_read = offset;
    }

    return 0;

error_unexpected:
    return -1;
}

nexrad_raster *nexrad_raster_packet_unpack(nexrad_raster_packet *packet, size_t *bytes_read, size_t max) {
    nexrad_raster *raster;

    size_t size,
           width, height;

    if (!_valid_packet(packet)) {
        goto error_invalid;
    }

    width  = _rle_width(packet);
    height = be16toh(packet->lines);
    size   = sizeof(nexrad_raster) + width * height;

    if ((raster = malloc(size)) == NULL) {
        goto error_malloc;
    }

    raster->width  = width;
    raster->height = height;

    if (_unpack_rle(raster, packet, bytes_read, max) < 0) {
        goto error_unpack;
    }

    return raster;

error_unpack:
    free(raster);

error_malloc:
error_invalid:
    return NULL;
}

void nexrad_raster_destroy(nexrad_raster *raster) {
    free(raster);
}

nexrad_image *nexrad_raster_create_image(nexrad_raster *raster, nexrad_color *colors) {
    nexrad_image *image;

    size_t x, y;

    if (raster == NULL || colors == NULL) {
        return NULL;
    }

    if ((image = nexrad_image_create(raster->width, raster->height)) == NULL) {
        goto error_image_create;
    }

    for (y=0; y<raster->height; y++) {
        for (x=0; x<raster->width; x++) {
            size_t index = y * raster->width + x;

            uint8_t v = ((uint8_t *)(raster + 1))[index];

            ((nexrad_color *)(image + 1))[index] = colors[v];
        }
    }

    return image;

error_image_create:
    return NULL;
}
