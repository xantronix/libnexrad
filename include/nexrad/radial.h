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

#ifndef _NEXRAD_RADIAL_H
#define _NEXRAD_RADIAL_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/image.h>

#define NEXRAD_RADIAL_RLE_FACTOR           16
#define NEXRAD_RADIAL_AZIMUTHAL_RESOLUTION  0.1
#define NEXRAD_RADIAL_RANGE_FACTOR          0.001
#define NEXRAD_RADIAL_BEAMWIDTH             1.0

enum nexrad_radial_type {
    NEXRAD_RADIAL_RLE     = 0xaf1f,
    NEXRAD_RADIAL_DIGITAL = 16
};

/*!
 * \file nexrad/radial.h
 * \brief Interface to radial radar data in NEXRAD Level III product files
 *
 * The primary interface to reading and handling radial radar data encoded in
 * NEXRAD Level III product files.  Is able to understand radar data encoded
 * both in a plain, 8-bits-per-rangebin (digital) manner, as well as a run
 * length-encoded format.
 */

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_radial_packet {
    uint16_t type;           /* 16 or 0xaf1f */
    uint16_t rangebin_first; /* Index to first range bin (whatever the hell that means) */
    uint16_t rangebin_count; /* Number of range bins per radial (resolution) */
     int16_t i;              /* I coordinate of center of sweep */
     int16_t j;              /* J coordinate of center of sweep */
    uint16_t scale;          /* Scale factor in units of 0.001 */
    uint16_t rays;           /* Number of rays in product */
} nexrad_radial_packet;

typedef struct _nexrad_radial_ray {
    uint16_t size;        /* Number of halfwords (0xaf1f) or bytes (16) in current ray */
    uint16_t angle_start; /* Scan angle of current ray */
    uint16_t angle_delta; /* Angle delta from previous ray */
} nexrad_radial_ray;

typedef struct _nexrad_radial_run { /* For 0xaf1f */
    unsigned int level  :4; /* Level (color code) of run */
    unsigned int length :4; /* Length of run */
} nexrad_radial_run;

#pragma pack(pop)

typedef struct _nexrad_radial {
    size_t rangebin_count;       /* Number of rangebins per ray */
    float  azimuthal_resolution; /* Azimuthal resolution of data */
    float  beamwidth;            /* Nominal beamwidth for each ray */
} nexrad_radial;

/*!
 * \defgroup radial NEXRAD Level III radial data handling routines
 */

/*!
 * \ingroup radial
 * \brief Unpack any radial packet into raster buffer with 0.1° accuracy
 * \param packet A RLE or digitally-encoded radial packet
 * \param bytes_read Pointer to store number of bytes read from packet
 * \param max The maximum number of bytes to read from packet
 * \return An expanded radial buffer
 *
 * Given an arbitrary radial packet, whether RLE- or digitally-encoded, will
 * generate a buffer with 8-bit data values, which can be used for O(1) lookups
 * with polar coordinates accurate to 0.1°.  RLE-encoded values are scaled from
 * rangebin values of 0-15 to 0-255.
 */
nexrad_radial *nexrad_radial_packet_unpack(nexrad_radial_packet *packet,
                                           size_t *bytes_read,
                                           size_t max);

/*!
 * \ingroup radial
 * \brief Close and destroy all state in a NEXRAD Level III radial object
 * \param radial A `nexrad_radial` object
 *
 * Destroys and free()s all state used to read a NEXRAD Level III radial packet.
 */
void nexrad_radial_destroy(nexrad_radial *radial);

/*!
 * \ingroup radial
 * \brief Determine a rangebin value for a given azimuth and range
 * \param radial A radial packet reader object
 * \param azimuth Azimuth 0-359.9
 * \param index Index of rangebin in radial
 * \return An integer 0-255 denoting the observed value, or -1 on failure
 *
 * Determine the value of a rangebin at a given azimuth and index.
 */
int nexrad_radial_get_rangebin(nexrad_radial *radial,
    float azimuth,
    int index);

#endif /* _NEXRAD_RADIAL_H */
