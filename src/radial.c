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

struct _nexrad_radial {
    nexrad_radial_packet *  packet;
    enum nexrad_radial_type type;
    nexrad_radial_ray *     current;

    size_t bytes_read;
    size_t rays_left;

    uint16_t  bins;
    uint8_t * values;
};

struct _nexrad_radial_buffer {
    uint16_t rays, bins, first, _unused;
};

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

nexrad_radial_buffer *nexrad_radial_packet_unpack(nexrad_radial_packet *packet) {
    nexrad_radial_buffer *buffer;
    nexrad_radial *radial;
    nexrad_radial_ray *ray;

    size_t size;

    uint16_t rays,
             bins,
             first;

    uint8_t *values;

    if (packet == NULL) {
        return NULL;
    }

    first = be16toh(packet->rangebin_first);
    bins  = be16toh(packet->rangebin_count);
    rays  = be16toh(packet->rays);

    size = sizeof(nexrad_radial_buffer) +
        NEXRAD_RADIAL_BUFFER_RAY_WIDTH * rays * bins;

    if ((buffer = malloc(size)) == NULL) {
        goto error_malloc;
    }

    if ((radial = nexrad_radial_packet_open(packet)) == NULL) {
        goto error_radial_packet_open;
    }

    while ((ray = nexrad_radial_read_ray(radial, &values)) != NULL) {
        int start = (int)be16toh(ray->angle_start),
            delta = (int)be16toh(ray->angle_delta);

        int j, b;

        for (j=start; j<start+delta; j++) {
            for (b=first; b<bins; b++) {
                ((uint8_t *)(buffer + 1))[bins*j+b] = values[b];
            }
        }
    }

    nexrad_radial_close(radial);

    buffer->first = first;
    buffer->bins  = bins;
    buffer->rays  = rays;

    return buffer;

error_radial_packet_open:
    free(buffer);

error_malloc:
    return NULL;
}

nexrad_radial *nexrad_radial_packet_open(nexrad_radial_packet *packet) {
    nexrad_radial *radial;
    enum nexrad_radial_type type;
    uint16_t bins;
    uint8_t *values;

    if (packet == NULL) {
        return NULL;
    }

    type = be16toh(packet->type);

    if (!_valid_packet(packet, type)) {
        return NULL;
    }

    if ((radial = malloc(sizeof(*radial))) == NULL) {
        goto error_malloc_radial;
    }

    bins = be16toh(packet->rangebin_count);

    if ((values = malloc(bins)) == NULL) {
        goto error_malloc_values;
    }

    radial->packet     = packet;
    radial->type       = type;
    radial->bytes_read = sizeof(nexrad_radial_packet);
    radial->rays_left  = be16toh(packet->rays);
    radial->current    = (nexrad_radial_ray *)((char *)packet + sizeof(nexrad_radial_packet));
    radial->bins       = bins;
    radial->values     = values;

    return radial;

error_malloc_values:
    free(radial);

error_malloc_radial:
    return NULL;
}

size_t nexrad_radial_bytes_read(nexrad_radial *radial) {
    if (radial == NULL)
        return 0;

    return radial->bytes_read;
}

void nexrad_radial_reset(nexrad_radial *radial) {
    if (radial == NULL)
        return;

    radial->bytes_read = 0;
    radial->rays_left  = be16toh(radial->packet->rays);
    radial->current    = (nexrad_radial_ray *)((char *)radial->packet + sizeof(nexrad_radial_packet));
}

void nexrad_radial_close(nexrad_radial *radial) {
    if (radial == NULL)
        return;

    if (radial->values)
        free(radial->values);

    memset(radial, '\0', sizeof(*radial));

    free(radial);
}

void nexrad_radial_destroy(nexrad_radial *radial) {
    if (radial == NULL)
        return;

    if (radial->values)
        free(radial->values);

    if (radial->packet)
        free(radial->packet);

    memset(radial, '\0', sizeof(*radial));

    free(radial);
}

static inline nexrad_radial_ray *_radial_ray_by_index(nexrad_radial *radial, uint16_t azimuth) {
    uint16_t bins = be16toh(radial->packet->rangebin_count);

    return (nexrad_radial_ray *)((char *)radial->packet
        + sizeof(nexrad_radial_packet)
        + azimuth * (sizeof(nexrad_radial_ray) + bins));
}

nexrad_radial_ray *nexrad_radial_get_ray(nexrad_radial *radial, int azimuth, uint8_t **values) {
    nexrad_radial_ray *ray;
    uint16_t rays, a;

    if (radial == NULL) {
        return NULL;
    }

    while (azimuth >= 360) azimuth -= 360;
    while (azimuth <    0) azimuth += 360;

    rays = be16toh(radial->packet->rays);

    /*
     * Do not allow this operation on RLE-encoded radials.
     */
    if (nexrad_radial_get_type(radial) != NEXRAD_RADIAL_DIGITAL || azimuth >= rays) {
        errno = EINVAL;
        return NULL;
    }

    /*
     * If the ray in the position in memory directly correlating to the current
     * azimuth is of a different angle than the azimuth, then brute forcedly
     * locate the correct ray.
     */
    ray = _radial_ray_by_index(radial, azimuth);

    if (azimuth == (int)round(NEXRAD_RADIAL_AZIMUTH_FACTOR * be16toh(ray->angle_start))) {
        goto found;
    }

    for (a=0; a<rays; a++) {
        ray = _radial_ray_by_index(radial, a);

        if (azimuth == (int)round(NEXRAD_RADIAL_AZIMUTH_FACTOR * be16toh(ray->angle_start))) {
            goto found;
        }
    }

    return NULL;

found:
    if (values)
        *values = (uint8_t *)ray + sizeof(nexrad_radial_ray);
        
    return ray;
}

int nexrad_radial_ray_get_azimuth(nexrad_radial_ray *ray) {
    if (ray == NULL) {
        return -1;
    }

    return (int)round(NEXRAD_RADIAL_AZIMUTH_FACTOR * be16toh(ray->angle_start));
}

int nexrad_radial_get_rangebin(nexrad_radial *radial, int azimuth, int range) {
    nexrad_radial_ray *ray;
    uint8_t *data;

    if (radial == NULL) {
        return -1;
    }

    if (range > be16toh(radial->packet->rangebin_count)) {
        return 0;
    }

    while (azimuth >= 360) azimuth -= 360;
    while (azimuth <    0) azimuth += 360;

    if ((ray = nexrad_radial_get_ray(radial, azimuth, &data)) == NULL) {
        goto error_radial_get_ray;
    }

    return (int)data[range];

error_radial_get_ray:
    return -1;
}

nexrad_radial_ray *nexrad_radial_read_ray(nexrad_radial *radial, uint8_t **values) {
    nexrad_radial_ray *ray;
    size_t size;

    if (radial == NULL || radial->rays_left == 0) {
        return NULL;
    }

    ray = radial->current;

    if (radial->type == NEXRAD_RADIAL_RLE) {
        uint16_t count = be16toh(ray->size) * 2;
        uint16_t bins  = be16toh(radial->packet->rangebin_count);

        nexrad_radial_run *runs = (nexrad_radial_run *)(ray + 1);

        uint16_t r, b;

        for (r=0, b=0; r<count; r++) {
            uint16_t i;

            for (i=0; i<runs[r].length && b<bins; i++, b++) {
                radial->values[b] = NEXRAD_RADIAL_RLE_FACTOR * runs[r].level;
            }
        }

        size = sizeof(nexrad_radial_ray) + count;
    } else if (radial->type == NEXRAD_RADIAL_DIGITAL) {
        uint16_t bins = be16toh(ray->size);
        uint8_t *data = (uint8_t *)ray + sizeof(nexrad_radial_ray);

        memcpy(radial->values, data, radial->bins > bins? bins: radial->bins);

        size = sizeof(nexrad_radial_ray) + bins;

        if (size % 2) size++;
    } else {
        return NULL;
    }

    /*
     * Advance the current ray pointer beyond the ray to follow.
     */
    radial->current = (nexrad_radial_ray *)((char *)ray + size);

    /*
     * Increase the number of bytes read in the current radial packet.
     */
    radial->bytes_read += size;

    /*
     * Decrement the number of rays left to read.
     */
    radial->rays_left--;

    if (values)
        *values = radial->values;

    return ray;
}

enum nexrad_radial_type nexrad_radial_get_type(nexrad_radial *radial) {
    if (radial == NULL) {
        return -1;
    }

    return be16toh(radial->packet->type);
}

int nexrad_radial_get_info(nexrad_radial *radial, uint16_t *rangebin_first, uint16_t *rangebin_count, int16_t *i, int16_t *j, uint16_t *scale, uint16_t *rays) {
    if (radial == NULL) {
        return -1;
    }

    if (rangebin_first)
        *rangebin_first = be16toh(radial->packet->rangebin_first);

    if (rangebin_count)
        *rangebin_count = be16toh(radial->packet->rangebin_count);

    if (i)
        *i = (int16_t)be16toh(radial->packet->i);

    if (j)
        *j = (int16_t)be16toh(radial->packet->j);

    if (scale)
        *scale = be16toh(radial->packet->scale);

    if (rays)
        *rays = be16toh(radial->packet->rays);

    return 0;
}

uint16_t nexrad_radial_rays_left(nexrad_radial *radial) {
    if (radial == NULL) {
        return 0;
    }

    return radial->rays_left;
}

nexrad_radial_packet *nexrad_radial_get_packet(nexrad_radial *radial) {
    if (radial == NULL) {
        return NULL;
    }

    return radial->packet;
}

nexrad_image *nexrad_radial_create_image(nexrad_radial *radial, nexrad_color_table *table) {
    nexrad_image *image;
    nexrad_color *entries;
    nexrad_radial_ray *ray;
    size_t width, height, radius;
    uint8_t *data;

    if (radial == NULL || table == NULL) {
        return NULL;
    }

    if ((entries = nexrad_color_table_get_entries(table, NULL)) == NULL) {
        goto error_color_table_get_entries;
    }

    radius = be16toh(radial->packet->rangebin_first) + be16toh(radial->packet->rangebin_count);
    width  = 2 * radius;
    height = 2 * radius;

    if ((image = nexrad_image_create(width, height)) == NULL) {
        goto error_image_create;
    }

    while ((ray = nexrad_radial_read_ray(radial, &data)) != NULL) {
        int angle_start_i = (int16_t)be16toh(ray->angle_start);
        int angle_delta_i = (int16_t)be16toh(ray->angle_delta);

        int angle_start = round(NEXRAD_RADIAL_AZIMUTH_FACTOR * angle_start_i);
        int angle_end   = round(NEXRAD_RADIAL_AZIMUTH_FACTOR * (angle_start_i + angle_delta_i));

        int radius = be16toh(radial->packet->rangebin_first);

        int b;

        for (b=0; b<radial->bins; b++) {
            nexrad_color color = entries[data[b]];

            if (color.a)
                nexrad_image_draw_arc_segment(image,
                    color,
                    angle_start, angle_end,
                    radius, radius+1
                );

            radius++;
        }
    }

    return image;

error_image_create:
error_color_table_get_entries:
    return NULL;
}

nexrad_image *nexrad_radial_create_projected_image(nexrad_radial *radial, nexrad_color_table *table, nexrad_geo_projection *proj) {
    nexrad_image *image;
    nexrad_color *entries;
    nexrad_radial_buffer *buffer;
    nexrad_geo_projection_point *points;
    uint16_t x, y, width, height, bins;

    if (radial == NULL || table == NULL || proj == NULL) {
        return NULL;
    }
    
    if ((buffer = nexrad_radial_packet_unpack(radial->packet)) == NULL) {
        goto error_radial_packet_unpack;
    }

    if (nexrad_radial_get_info(radial, NULL, &bins, NULL, NULL, NULL, NULL) < 0) {
        goto error_radial_get_info;
    }

    if ((entries = nexrad_color_table_get_entries(table, NULL)) == NULL) {
        goto error_color_table_get_entries;
    }

    if (nexrad_geo_projection_read_dimensions(proj, &width, &height) < 0) {
        goto error_geo_projection_read_dimensions;
    }

    if ((points = nexrad_geo_projection_get_points(proj)) == NULL) {
        goto error_geo_projection_get_points;
    }

    if ((image = nexrad_image_create(width, height)) == NULL) {
        goto error_image_create;
    }

    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
            nexrad_color color;
            int azimuth, range;
            uint8_t value;

            nexrad_geo_projection_point *point = &points[y*width+x];

            azimuth = (int)be16toh(point->azimuth);
            range   = (int)be16toh(point->range);

            if (range >= bins) {
                continue;
            }

            value = ((uint8_t *)(buffer + 1))
                [azimuth*10*buffer->bins+range];

            color = entries[value];

            if (color.a)
                nexrad_image_draw_pixel(image, color, x, y);
        }
    }

    free(buffer);

    return image;

error_image_create:
error_geo_projection_get_points:
error_geo_projection_read_dimensions:
error_color_table_get_entries:
error_radial_get_info:
    free(buffer);

error_radial_packet_unpack:
    return NULL;
}
