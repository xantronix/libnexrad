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

static void _unpack_rle(nexrad_radial *radial, nexrad_radial_packet *packet) {
    size_t offset = 0;

    uint16_t rays,
             bins;
             
    int ray;

    if (radial == NULL || packet == NULL) {
        return;
    }

    rays = be16toh(packet->rays);
    bins = be16toh(packet->rangebin_count);

    for (ray=0; ray<rays; ray++) {
        nexrad_radial_ray *current = (nexrad_radial_ray *)
            (((uint8_t *)(packet + 1)) + offset);

        nexrad_radial_run *runs = (nexrad_radial_run *)(current + 1);

        uint16_t count = be16toh(current->size),
                 azimuth;

        uint16_t start = be16toh(current->angle_start),
                 delta = be16toh(current->angle_delta);

        for (azimuth=start; azimuth<start+delta; azimuth++) {
            uint16_t r, b;

            for (r=0, b=0; r<count; r++) {
                uint16_t i;

                for (i=0; i<runs[r].length && b<bins; i++) {
                    ((uint8_t *)(radial + 1))[bins*azimuth+b] =
                        NEXRAD_RADIAL_RLE_FACTOR* runs[r].level;
                }
            }
        }

        offset += sizeof(nexrad_radial_ray)
            + (2 * count * sizeof(nexrad_radial_run));
    }

    return;
}

static void _unpack_digital(nexrad_radial *radial, nexrad_radial_packet *packet) {
    size_t offset = 0;

    uint16_t rays,
             bins;

    int ray;

    if (radial == NULL || packet == NULL) {
        return;
    }

    rays = be16toh(packet->rays);
    bins = be16toh(packet->rangebin_count);

    for (ray=0; ray<rays; ray++) {
        nexrad_radial_ray *current = (nexrad_radial_ray *)
            (((uint8_t *)(packet + 1)) + offset);

        uint16_t start = be16toh(current->angle_start),
                 delta = be16toh(current->angle_delta);

        uint16_t azimuth;

        size_t size = be16toh(current->size);

        for (azimuth=start; azimuth<start+delta; azimuth++) {
            size_t dest = azimuth * bins;

            memcpy((uint8_t *)(radial + 1) + dest, current + 1, size);
        }

        offset += sizeof(nexrad_radial_ray) + size;

        if (offset & 1)
            offset++;
    }

    return;
}

nexrad_radial *nexrad_radial_packet_unpack(nexrad_radial_packet *packet) {
    nexrad_radial *radial;
    enum nexrad_radial_type type;
    size_t size;

    uint16_t rays,
             bins;

    if (packet == NULL) {
        return NULL;
    }

    type = be16toh(packet->type);

    if (!_valid_packet(packet, type)) {
        return NULL;
    }

    rays = be16toh(packet->rays);
    bins = be16toh(packet->rangebin_count);
    size = sizeof(nexrad_radial) + NEXRAD_RADIAL_BUFFER_RAY_WIDTH * rays * bins;

    if ((radial = malloc(size)) == NULL) {
        goto error_malloc_radial;
    }

    radial->bins  = bins;
    radial->first = be16toh(packet->rangebin_first);

    switch (type) {
        case NEXRAD_RADIAL_RLE:
            _unpack_rle(radial, packet); break;

        case NEXRAD_RADIAL_DIGITAL:
            _unpack_digital(radial, packet); break;
    }

    return radial;

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
