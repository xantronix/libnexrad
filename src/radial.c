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
#include <math.h>
#include <errno.h>
#include "util.h"

#include <nexrad/radial.h>

#define NEXRAD_RADIAL_BUFFER_RAY_WIDTH 10

static int _valid_rle_packet(nexrad_radial_packet *packet) {
    if (
      be16toh(packet->rangebin_first) >   460 ||
      be16toh(packet->rangebin_count) >   460 ||
      be16toh(packet->scale)          >  8000 ||
      be16toh(packet->scale)          <     1 ||
      be16toh(packet->rays)           >   400
    ) {
        return 0;
    }

    return 1;
}

static int _valid_digital_packet(nexrad_radial_packet *packet) {
    if (
      be16toh(packet->rangebin_first) >   230 ||
      be16toh(packet->rangebin_count) >  1840 ||
      be16toh(packet->scale)          >  1000 ||
      be16toh(packet->scale)          <     1 ||
      be16toh(packet->rays)           >   720
    ) {
        return 0;
    }

    return 1;
}

static int _valid_packet(nexrad_radial_packet *packet, enum nexrad_radial_type type) {
    switch (type) {
        case NEXRAD_RADIAL_RLE:     return _valid_rle_packet(packet);
        case NEXRAD_RADIAL_DIGITAL: return _valid_digital_packet(packet);

        default: {
            break;
        }
    }

    return 0;
}

static int _unpack_rle(nexrad_radial *radial, nexrad_radial_packet *packet, size_t *bytes_read, size_t max) {
    size_t offset = sizeof(nexrad_radial_packet);

    uint16_t rays,
             bins;
             
    int ray;

    if (radial == NULL || packet == NULL) {
        return -1;
    }

    rays = be16toh(packet->rays);
    bins = be16toh(packet->rangebin_count);

    for (ray=0; ray<rays; ray++) {
        nexrad_radial_ray *current = (nexrad_radial_ray *)
            ((uint8_t *)packet + offset);

        nexrad_radial_run *runs = (nexrad_radial_run *)(current + 1);

        uint16_t count = 2 * be16toh(current->size),
                 azimuth;

        uint16_t start = be16toh(current->angle_start),
                 delta = be16toh(current->angle_delta);

        offset += sizeof(nexrad_radial_ray) + count;

        if (offset > max) {
            goto error_unexpected;
        }

        for (azimuth=start; azimuth<start+delta; azimuth++) {
            uint16_t a = azimuth,
                     r, b;

            if (a >= 3600)
                a -= 3600;

            for (r=0, b=0; r<count; r++) {
                uint16_t i;

                for (i=0; i<runs[r].length && b<bins; i++, b++) {
                    ((uint8_t *)(radial + 1))[bins*a+b] =
                        NEXRAD_RADIAL_RLE_FACTOR * runs[r].level;
                }
            }
        }
    }

    *bytes_read = offset;

    return 0;

error_unexpected:
    return -1;
}

static int _unpack_digital(nexrad_radial *radial, nexrad_radial_packet *packet, size_t *bytes_read, size_t max) {
    size_t offset = sizeof(nexrad_radial_packet);

    uint16_t rays,
             bins;

    int ray;

    if (radial == NULL || packet == NULL) {
        return -1;
    }

    rays = be16toh(packet->rays);
    bins = be16toh(packet->rangebin_count);

    for (ray=0; ray<rays; ray++) {
        nexrad_radial_ray *current = (nexrad_radial_ray *)
            ((uint8_t *)packet + offset);

        uint16_t start = be16toh(current->angle_start),
                 delta = be16toh(current->angle_delta);

        uint16_t azimuth;

        size_t size = be16toh(current->size);

        offset += sizeof(nexrad_radial_ray) + size;

        if (offset & 1) {
            offset++;
        }

        if (offset > max) {
            goto error_unexpected;
        }

        for (azimuth=start; azimuth<start+delta; azimuth++) {
            uint16_t a = azimuth;

            size_t dest;

            if (a >= 3600)
                a -= 3600;

            dest = a * bins;

            memcpy((uint8_t *)(radial + 1) + dest, current + 1, size);
        }
    }

    *bytes_read = offset;

    return 0;

error_unexpected:
    return -1;
}

nexrad_radial *nexrad_radial_packet_unpack(nexrad_radial_packet *packet, size_t *bytes_read, size_t max) {
    nexrad_radial *radial;
    enum nexrad_radial_type type;
    size_t size;

    uint16_t rays,
             bins;

    ssize_t result;

    if (packet == NULL) {
        return NULL;
    }

    type = be16toh(packet->type);

    if (!_valid_packet(packet, type)) {
        return NULL;
    }

    rays = be16toh(packet->rays);
    bins = be16toh(packet->rangebin_count);
    size = sizeof(nexrad_radial)
        + NEXRAD_RADIAL_BUFFER_RAY_WIDTH * rays * bins;

    if ((radial = malloc(size)) == NULL) {
        goto error_malloc_radial;
    }

    radial->bins  = bins;
    radial->first = be16toh(packet->rangebin_first);

    switch (type) {
        case NEXRAD_RADIAL_RLE:
            result = _unpack_rle(radial, packet, bytes_read, max); break;

        case NEXRAD_RADIAL_DIGITAL:
            result = _unpack_digital(radial, packet, bytes_read, max); break;
    }

    if (result < 0) {
        goto error_unexpected;
    }

    return radial;

error_unexpected:
    free(radial);

error_malloc_radial:
    return NULL;
}

void nexrad_radial_destroy(nexrad_radial *radial) {
    free(radial);
}

int nexrad_radial_get_rangebin(nexrad_radial *radial, float azimuth, int index) {
    uint16_t j = roundf(10 * azimuth);

    if (radial == NULL) {
        return -1;
    }

    if (index > be16toh(radial->bins)) {
        return 0;
    }

    return ((uint8_t *)(radial + 1))[j*radial->bins+index];
}
