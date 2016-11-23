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
#include <math.h>

#include <nexrad/poly.h>
#include <nexrad/geo.h>
#include "util.h"

static inline size_t _poly_point_size() {
    return sizeof(nexrad_poly_point);
}

static inline size_t _poly_ring_size(int points) {
    return sizeof(nexrad_poly_ring)
        + points * _poly_point_size();
}

static inline size_t _poly_size(int points) {
    return sizeof(nexrad_poly) + _poly_ring_size(points);
}

static inline size_t _poly_multi_size(int rangebins) {
    return sizeof(nexrad_poly_multi)
        + rangebins * _poly_size(NEXRAD_POLY_POINTS + 1);
}

static void _poly_multi_set_poly_at_index(nexrad_poly_multi *multi, int index, nexrad_geo_cartesian *ipoints) {
    nexrad_poly *poly;
    nexrad_poly_ring *ring;
    nexrad_poly_point *points;
    int i;

    poly = (nexrad_poly *)((char *)multi
        + sizeof(nexrad_poly_multi)
        + index * _poly_size(NEXRAD_POLY_POINTS + 1));

    poly->byte_order = NEXRAD_POLY_BYTE_ORDER_LE;
    poly->type       = htole32(NEXRAD_POLY_TYPE);
    poly->rings      = htole32(NEXRAD_POLY_RINGS);

    ring = (nexrad_poly_ring *)((char *)poly
        + sizeof(nexrad_poly));

    ring->points = htole32(NEXRAD_POLY_POINTS + 1);

    points = (nexrad_poly_point *)((char *)ring
        + sizeof(nexrad_poly_ring));

    for (i=0; i<NEXRAD_POLY_POINTS; i++) {
        points[i].lat = htole64(ipoints[i].lat);
        points[i].lon = htole64(ipoints[i].lon);
    }

    /*
     * As WKB requires polygons to be closed, repeat the first point in the
     * final slot allocated.
     */
    points[NEXRAD_POLY_POINTS].lat = htole64(ipoints[0].lat);
    points[NEXRAD_POLY_POINTS].lon = htole64(ipoints[0].lon);
}

struct poly_context {
    nexrad_geo_spheroid *  spheroid;
    nexrad_geo_cartesian * radar;
    nexrad_geo_cartesian * cartesian_points;
    nexrad_geo_polar     * polar_points;
};

static void _poly_multi_set_rangebin(nexrad_poly_multi *multi, int index, int azimuth, int range, struct poly_context *ctx) {
    int i;

    ctx->polar_points[0].azimuth = (double)azimuth - 0.5;
    ctx->polar_points[0].range   = (double)range / NEXRAD_RADIAL_RANGE_FACTOR;

    ctx->polar_points[1].azimuth = (double)azimuth - 0.5;
    ctx->polar_points[1].range   = (double)range / NEXRAD_RADIAL_RANGE_FACTOR + 1000.0;

    ctx->polar_points[2].azimuth = (double)azimuth + 0.5;
    ctx->polar_points[2].range   = (double)range / NEXRAD_RADIAL_RANGE_FACTOR + 1000.0;

    ctx->polar_points[3].azimuth = (double)azimuth + 0.5;
    ctx->polar_points[3].range   = (double)range / NEXRAD_RADIAL_RANGE_FACTOR;

    for (i=0; i<NEXRAD_POLY_POINTS; i++) {
        nexrad_geo_find_cartesian_dest(ctx->spheroid,
            ctx->radar, &ctx->cartesian_points[i], &ctx->polar_points[i]
        );

        if (ctx->cartesian_points[i].lon >= -80.0)
            fprintf(stderr, "Found a weird longitude\n");

        if (ctx->cartesian_points[i].lat >= 40.0)
            fprintf(stderr, "Found a weird latitude\n");
    }

    _poly_multi_set_poly_at_index(multi, index, ctx->cartesian_points);
}

int nexrad_poly_multi_size_for_radial(nexrad_radial *radial, int min, int max, size_t *sizep, int *rangebinsp) {
    nexrad_radial_ray *ray;
    uint8_t *values;
    uint16_t bins;
    int rangebins = 0;

    if (radial == NULL || sizep == NULL || rangebinsp == NULL) {
        return -1;
    }

    if (min < 0 || min > max || max > 255) {
        return -1;
    }

    if (nexrad_radial_get_info(radial, NULL, &bins, NULL, NULL, NULL, NULL) < 0) {
        goto error_radial_get_info;
    }

    nexrad_radial_reset(radial);

    while ((ray = nexrad_radial_read_ray(radial, &values)) != NULL) {
        int range;

        for (range=0; range<bins; range++) {
            int v = values[range];

            if (v < min || v > max)
                continue;

            rangebins++;
        }
    }

    *sizep      = _poly_multi_size(rangebins);
    *rangebinsp = rangebins;

    return 0;

error_radial_get_info:
    return -1;
}

void _poly_multi_init(nexrad_poly_multi *multi, int rangebins) {
    if (multi == NULL) {
        return;
    }
    
    multi->byte_order = NEXRAD_POLY_BYTE_ORDER_LE;
    multi->type       = htole32(NEXRAD_POLY_MULTI_TYPE);
    multi->polys      = htole32(rangebins);
}

int nexrad_poly_multi_write_from_radial(nexrad_radial *radial, int min, int max, int rangebins, nexrad_poly_multi *multi, size_t size, nexrad_geo_cartesian *radar, nexrad_geo_spheroid *spheroid) {
    nexrad_geo_polar *polar_points;
    nexrad_geo_cartesian *cartesian_points;

    nexrad_radial_ray *ray;
    uint8_t *values;
    uint16_t bins;
    int rangebin = 0;

    struct poly_context *ctx;

    if (radial == NULL || multi == NULL || size == 0 || radar == NULL || spheroid == NULL) {
        return -1;
    }

    if (min < 0 || min > max || max > 255) {
        return -1;
    }

    if (nexrad_radial_get_info(radial, NULL, &bins, NULL, NULL, NULL, NULL) < 0) {
        goto error_radial_get_info;
    }

    if ((cartesian_points = malloc(NEXRAD_POLY_POINTS * sizeof(nexrad_geo_cartesian))) == NULL) {
        goto error_malloc_cartesian_points;
    }

    if ((polar_points = malloc(NEXRAD_POLY_POINTS * sizeof(nexrad_geo_polar))) == NULL) {
        goto error_malloc_polar_points;
    }

    if ((ctx = malloc(sizeof(*ctx))) == NULL) {
        goto error_malloc_ctx;
    }

    ctx->radar            = radar;
    ctx->spheroid         = spheroid;
    ctx->cartesian_points = cartesian_points;
    ctx->polar_points     = polar_points;

    _poly_multi_init(multi, rangebins);

    nexrad_radial_reset(radial);

    while ((ray = nexrad_radial_read_ray(radial, &values)) != NULL) {
        int azimuth, range;

        if ((azimuth = nexrad_radial_ray_get_azimuth(ray)) < 0) {
            goto error_radial_ray_get_azimuth;
        }

        for (range=0; range<bins; range++) {
            int v = values[range];

            if (v < min || v > max)
                continue;

            _poly_multi_set_rangebin(multi, rangebin++, azimuth, range, ctx);
        }
    }

    free(ctx);
    free(polar_points);
    free(cartesian_points);

    return 0;

    free(ctx);

error_radial_ray_get_azimuth:
error_malloc_ctx:
    free(polar_points);

error_malloc_polar_points:
    free(cartesian_points);

error_malloc_cartesian_points:
error_radial_get_info:
    return -1;
}

nexrad_poly_multi *nexrad_poly_multi_create_from_radial(nexrad_radial *radial, int min, int max, nexrad_geo_cartesian *radar, nexrad_geo_spheroid *spheroid, size_t *sizep) {
    nexrad_poly_multi *multi;
    size_t size;
    int rangebins;

    if (radial == NULL || sizep == NULL || radar == NULL || spheroid == NULL) {
        return NULL;
    }

    if (min < 0 || min > max || max > 255) {
        return NULL;
    }

    if (nexrad_poly_multi_size_for_radial(radial, min, max, &size, &rangebins) < 0) {
        goto error_poly_multi_size_for_radial;
    }

    if ((multi = malloc(size)) == NULL) {
        goto error_malloc_multi;
    }

    if (nexrad_poly_multi_write_from_radial(radial, min, max, rangebins, multi, size, radar, spheroid) < 0) {
        goto error_poly_multi_write_from_radial;
    }

    *sizep = size;

    return multi;

error_poly_multi_write_from_radial:
    free(multi);

error_malloc_multi:
error_poly_multi_size_for_radial:
    return NULL;
}

void nexrad_poly_multi_destroy(nexrad_poly_multi *multi) {
    if (multi == NULL)
        return;

    free(multi);
}
