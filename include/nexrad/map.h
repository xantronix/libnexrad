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

#ifndef _NEXRAD_MAP_H
#define _NEXRAD_MAP_H

#include <nexrad/image.h>
#include <nexrad/radial.h>

#define NEXRAD_MAP_EARTH_RADIUS     6378137.0
#define NEXRAD_MAP_EARTH_REFRACTION (4.0f/3.0f)

#define NEXRAD_MAP_MAX_LAT    85.05112878
#define NEXRAD_MAP_TILE_SIZE 256

typedef struct _nexrad_map_point {
    float lat, lon;
} nexrad_map_point;

typedef struct _nexrad_map_heading {
    float azimuth, range;
} nexrad_map_heading;

float nexrad_map_range_factor(float tilt,
                              float resolution,
                              float refraction);

void nexrad_map_find_point(nexrad_map_point start,
                           nexrad_map_heading heading,
                           nexrad_map_point *end);

void nexrad_map_find_heading(nexrad_map_point start,
                             nexrad_map_point end,
                             nexrad_map_heading *heading);

nexrad_image *nexrad_map_project_radial(nexrad_radial *radial,
                                        nexrad_map_point *radar,
                                        nexrad_map_point *extents,
                                        nexrad_color *colors,
                                        float factor,
                                        int zoom);

#endif /* _NEXRAD_MAP_H */
