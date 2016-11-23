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

#ifndef _NEXRAD_RASTER_H
#define _NEXRAD_RASTER_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/image.h>

#define NEXRAD_RASTER_RLE_FACTOR 16

#pragma pack(1)
#pragma pack(push)

typedef struct _nexrad_raster_packet {
    uint16_t type;         /* 0xba0f or 0xba07 */
    uint16_t flags_1;      /* 0x8000, so I'm told */
    uint16_t flags_2;      /* 0x00c0, so they say */
     int16_t i;            /* Start of raster data (upper left corner) */
     int16_t j;            /* Start of raster data (upper left corner) */
    uint16_t x_scale;      /* X axis scaling factor */
    uint16_t x_fractional; /* Reserved */
    uint16_t y_scale;      /* Y axis scaling factor */
    uint16_t y_fractional; /* Reserved */
    uint16_t lines;        /* Number of raster lines */
    uint16_t packing;      /* Packing format (seemingly always 2) */
} nexrad_raster_packet;

typedef struct _nexrad_raster_line {
    uint16_t runs; /* Number of RLE-encoded runs in raster line */
} nexrad_raster_line;

typedef struct _nexrad_raster_run {
    unsigned int level  :4; /* Level (color code) of run */
    unsigned int length :4; /* Length of run */
} nexrad_raster_run;

#pragma pack(pop)

typedef struct _nexrad_raster nexrad_raster;

nexrad_raster *nexrad_raster_packet_open(nexrad_raster_packet *packet);

size_t nexrad_raster_bytes_read(nexrad_raster *raster);

void nexrad_raster_close(nexrad_raster *raster);

nexrad_raster_line *nexrad_raster_read_line(nexrad_raster *raster,
    void **data,
    uint16_t *runsp
);

int nexrad_raster_get_info(nexrad_raster *raster,
    uint16_t *widthp,
    uint16_t *heightp
);

nexrad_image *nexrad_raster_create_image(nexrad_raster *raster,
    nexrad_color_table *table
);

#endif /* _NEXRAD_RASTER_H */
