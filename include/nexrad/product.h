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

#ifndef _NEXRAD_PRODUCT_H
#define _NEXRAD_PRODUCT_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/date.h>

#pragma pack(push)
#pragma pack(1)

#define NEXRAD_VERSION 1

#define NEXRAD_PRODUCT_COORD_MAGNITUDE 0.001
#define NEXRAD_PRODUCT_ALT_FACTOR      0.3048
#define NEXRAD_PRODUCT_AVSET_MAGNITUDE 0.1

enum nexrad_product_compression_type {
    NEXRAD_PRODUCT_COMPRESSION_NONE  = 0,
    NEXRAD_PRODUCT_COMPRESSION_BZIP2 = 1
};

enum nexrad_product_type {
    NEXRAD_PRODUCT_NHI =  59,
    NEXRAD_PRODUCT_N0R =  94,
    NEXRAD_PRODUCT_DVL = 134,
    NEXRAD_PRODUCT_EET = 135,
    NEXRAD_PRODUCT_N0X = 159
};

enum nexrad_radar_mode {
    NEXRAD_RADAR_MODE_PRECIP = 2
};

typedef struct _nexrad_product_attributes {
    uint16_t p1_2[2];
     int16_t elevation;
    uint16_t p3;
    uint16_t thresholds[16];
    uint16_t p4_7[4];
    uint16_t compression;
    uint16_t p9_10[2];
} nexrad_product_attributes;

typedef struct _nexrad_compression_attributes {
    uint16_t p1_24[24];
    uint16_t method;
    uint32_t size;
} nexrad_compression_attributes;

typedef struct _nexrad_dvil_attributes {
    uint16_t p1;
    uint16_t p2;
     int16_t elevation;
     int16_t avset_angle; /* Magnitude 0.1, -1.0 to +45.0 */
    uint16_t thresholds[16];
    uint16_t max_dvil;
    uint16_t edited_radials;
    uint16_t p6;
    uint16_t p7;
    uint16_t compression;
    uint32_t size;
} nexrad_dvil_attributes;

typedef struct _nexrad_product_description {
     int16_t    divider;          /* Start of block */
     int32_t    station_lat;      /* Radar site latitude */
     int32_t    station_lon;      /* Radar site longitude */
     int16_t    station_altitude; /* Radar site altitude */
    uint16_t    type;             /* NOAA product ID */
    uint16_t    mode;             /* Radar operational mode */
    uint16_t    vcp;              /* Radar volume coverage pattern */
     int16_t    seq;              /* Request sequence number */
    uint16_t    scan;             /* Volume scan number */
    nexrad_date scan_date;        /* Start of current scan */
    nexrad_date gen_date;         /* Time of radar product generation */

    /*
     * Product-specific attributes
     */
    union {
        nexrad_product_attributes     generic;
        nexrad_compression_attributes compression;
        nexrad_dvil_attributes        dvil;
    } attributes;

     uint8_t version;  /* Version */
     uint8_t blanking; /* Spot blanking */

    /*
     * An interesting and mildly annoying note regarding these offsets: NOAA
     * seems to think that it is a good idea to use "halfword" (16-bit) size
     * increments to describe these offsets, so to obtain the appropriate
     * offset, multiply the offset by two and add the product to the physical
     * offset of the start of the Product Message Header.
     */
    uint32_t symbology_offset; /* Offset to product symbology block */
    uint32_t graphic_offset;   /* Offset to graphic product message block */
    uint32_t tabular_offset;   /* Offset to tabular alphanumeric block */
} nexrad_product_description;

#pragma pack(pop)

int nexrad_product_type_supports_compression(enum nexrad_product_type type);

enum nexrad_product_type nexrad_product_get_type(nexrad_product_description *product);

int nexrad_product_read_dvil_attributes(nexrad_product_description *product,
    int *avset_angle,
    int *max_dvil,
    int *edited_radials,
    int *compression,
    size_t *size
);

#endif /* _NEXRAD_PRODUCT_H */
