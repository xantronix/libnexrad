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

#include <nexrad/geo.h>

int main(int argc, char **argv) {
    nexrad_geo_spheroid *spheroid;
    nexrad_geo_projection *proj;

    nexrad_geo_cartesian radar = {
        35.333, -97.278
    };

    spheroid = nexrad_geo_spheroid_create();

    if ((proj = nexrad_geo_projection_create_equirect("equirect.proj", spheroid, &radar, 346, 1000, 0.00815)) == NULL) {
        perror("nexrad_geo_projection_create_equirect()");
        exit(1);
    }

    if (nexrad_geo_projection_save(proj) < 0) {
        perror("nexrad_geo_projection_save()");
        exit(1);
    }

    nexrad_geo_projection_close(proj);

    if ((proj = nexrad_geo_projection_create_mercator("mercator.proj", spheroid, &radar, 346, 1000, 8)) == NULL) {
        perror("nexrad_geo_projection_create_mercator()");
        exit(1);
    }

    if (nexrad_geo_projection_save(proj) < 0) {
        perror("nexrad_geo_projection_save()");
        exit(1);
    }

    nexrad_geo_projection_close(proj);

    return 0;
}
