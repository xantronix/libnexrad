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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nexrad/message.h>
#include <nexrad/raster.h>
#include <nexrad/radial.h>

#include "../src/util.h"

static nexrad_radial_packet *create_radial_packet() {
    int rangebins = 234;
    int rays      = 360;
    int i;

    size_t size = sizeof(nexrad_radial_packet)
        + rays * (sizeof(nexrad_radial_ray) + rangebins);

    nexrad_radial_packet *packet;

    if ((packet = malloc(size)) == NULL) {
        return NULL;
    }

    packet->type           = htobe16(16);
    packet->rangebin_first = htobe16(0);
    packet->rangebin_count = htobe16(rangebins);
    packet->i              = htobe16(0);
    packet->j              = htobe16(0);
    packet->scale          = htobe16(999);
    packet->rays           = htobe16(rays);

    for (i=0; i<rays; i++) {
        nexrad_radial_ray *ray = (nexrad_radial_ray *)((char *)packet
            + sizeof(nexrad_radial_packet)
            + i * (sizeof(nexrad_radial_ray) + rangebins));

        uint8_t *bins = (uint8_t *)ray + sizeof(nexrad_radial_ray);
        int r;

        ray->size        = htobe16(rangebins);
        ray->angle_start = htobe16(10 * i);
        ray->angle_delta = htobe16(10);

        memset(bins, '\0', rangebins);

        for (r=0; r<128; r++) {
            if (i < 180)
                bins[r] = r;
            else
                bins[r] = r + 128;
        }

        if (i % 45 == 0) {
            memset(bins, 0xff, rangebins);
        }
    }

    return packet;
}

int main(int argc, char **argv) {
    nexrad_radial_packet *packet = create_radial_packet();

    write(1, packet, 86414);

    return 0;
}
