#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "util.h"

#include <nexrad/radial.h>

struct _nexrad_radial {
    nexrad_radial_packet *  packet;
    enum nexrad_radial_type type;
    size_t                  bytes_read;
    size_t                  rays_left;
    nexrad_radial_ray *     current;
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

nexrad_radial_packet *nexrad_radial_packet_unpack(nexrad_radial_packet *rle, size_t *sizep) {
    nexrad_radial_packet *digital;
    nexrad_radial *radial;
    nexrad_radial_ray *rle_ray;
    nexrad_radial_run *runs;
    size_t packet_size, ray_size;
    uint16_t rays, bins, nruns, scale;

    if (rle == NULL) {
        return NULL;
    }

    if (be16toh(rle->type) != NEXRAD_RADIAL_RLE) {
        return NULL;
    }

    if (!_valid_rle_packet(rle)) {
        goto error_invalid_rle_packet;
    }

    rays  = be16toh(rle->rays);
    bins  = be16toh(rle->rangebin_count);
    scale = be16toh(rle->scale);

    ray_size    = sizeof(nexrad_radial_ray)    + bins;
    packet_size = sizeof(nexrad_radial_packet) + rays * ray_size;

    if ((digital = malloc(packet_size)) == NULL) {
        goto error_malloc;
    }

    if ((radial = nexrad_radial_packet_open(rle)) == NULL) {
        goto error_radial_packet_open;
    }

    while ((rle_ray = nexrad_radial_read_ray(radial, (void **)&runs, &nruns, NULL)) != NULL) {
        uint16_t azimuth = (uint16_t)round(NEXRAD_RADIAL_AZIMUTH_FACTOR * be16toh(rle_ray->angle_start));
        uint16_t b = 0, r;

        nexrad_radial_ray *digital_ray = (nexrad_radial_ray *)((char *)digital + sizeof(nexrad_radial_packet) + azimuth * ray_size);

        uint8_t *data = (uint8_t *)digital_ray + sizeof(nexrad_radial_ray);

        digital_ray->size        = htobe16(bins);
        digital_ray->angle_start = htobe16(azimuth / NEXRAD_RADIAL_AZIMUTH_FACTOR);
        digital_ray->angle_delta = htobe16(1 / NEXRAD_RADIAL_AZIMUTH_FACTOR);

        for (r=0; r<nruns; r++) {
            uint16_t i;

            for (i=0; i<runs[r].length; i++) {
                data[b++] = NEXRAD_RADIAL_RLE_FACTOR * runs[r].level;
            }
        }
    }

    nexrad_radial_close(radial);

    digital->type           = htobe16(NEXRAD_RADIAL_DIGITAL);
    digital->rangebin_first = 0;
    digital->rangebin_count = htobe16(bins);
    digital->i              = 0;
    digital->j              = 0;
    digital->scale          = htobe16(scale);
    digital->rays           = htobe16(rays);

    if (sizep)
        *sizep = packet_size;

    return digital;

error_radial_packet_open:
    free(digital);

error_malloc:
error_invalid_rle_packet:
    return NULL;
}

nexrad_radial *nexrad_radial_packet_open(nexrad_radial_packet *packet) {
    nexrad_radial *radial;
    enum nexrad_radial_type type;

    if (packet == NULL) {
        return NULL;
    }

    type = be16toh(packet->type);

    if (!_valid_packet(packet, type)) {
        return NULL;
    }

    if ((radial = malloc(sizeof(*radial))) == NULL) {
        goto error_malloc;
    }

    radial->packet     = packet;
    radial->type       = type;
    radial->bytes_read = sizeof(nexrad_radial_packet);
    radial->rays_left  = be16toh(packet->rays);
    radial->current    = (nexrad_radial_ray *)((char *)packet + sizeof(nexrad_radial_packet));

    return radial;

error_malloc:
    return NULL;
}

size_t nexrad_radial_bytes_read(nexrad_radial *radial) {
    if (radial == NULL) {
        return 0;
    }

    return radial->bytes_read;
}

void nexrad_radial_reset(nexrad_radial *radial) {
    if (radial == NULL) return;

    radial->bytes_read = 0;
    radial->rays_left  = be16toh(radial->packet->rays);
    radial->current    = (nexrad_radial_ray *)((char *)radial->packet + sizeof(nexrad_radial_packet));
}

void nexrad_radial_close(nexrad_radial *radial) {
    if (radial == NULL) return;

    radial->packet     = NULL;
    radial->bytes_read = 0;
    radial->rays_left  = 0;
    radial->current    = NULL;

    free(radial);
}

static inline nexrad_radial_ray *_radial_ray_by_index(nexrad_radial *radial, uint16_t azimuth) {
    uint16_t bins = be16toh(radial->packet->rangebin_count);
    size_t offset = sizeof(nexrad_radial_packet) + azimuth * (sizeof(nexrad_radial_ray) + bins);

    return (nexrad_radial_ray *)((char *)radial->packet + offset);
}

nexrad_radial_ray *nexrad_radial_get_ray(nexrad_radial *radial, int azimuth, void **data, uint16_t *runsp, uint16_t *binsp) {
    nexrad_radial_ray *ray;
    uint16_t rays, a, runs = 0, bins = 0;

    if (radial == NULL) {
        return NULL;
    }

    rays = be16toh(radial->packet->rays);

    /*
     * Do not allow this operation on RLE-encoded radials.
     */
    if (nexrad_radial_get_type(radial) != NEXRAD_RADIAL_DIGITAL || azimuth > rays) {
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
        goto success;
    }

    for (a=0; a<rays; a++) {
        ray = _radial_ray_by_index(radial, a);

        if (azimuth == (int)round(NEXRAD_RADIAL_AZIMUTH_FACTOR * be16toh(ray->angle_start))) {
            goto success;
        }
    }

    return NULL;

success:
    if (radial->type == NEXRAD_RADIAL_RLE) {
        runs = be16toh(ray->size) * 2;
        bins = be16toh(radial->packet->rangebin_count);
    } else if (radial->type == NEXRAD_RADIAL_DIGITAL) {
        runs = 0;
        bins = be16toh(ray->size);
    }

    if (data)
        *data = (void *)((char *)ray + sizeof(nexrad_radial_ray));

    if (runsp)
        *runsp = runs;

    if (binsp)
        *binsp = bins;
        
    return ray;
}

int nexrad_radial_ray_get_azimuth(nexrad_radial_ray *ray) {
    if (ray == NULL) {
        return -1;
    }

    return (int)round(NEXRAD_RADIAL_AZIMUTH_FACTOR * be16toh(ray->angle_start));
}

int nexrad_radial_get_rangebin(nexrad_radial *radial, uint16_t azimuth, uint16_t range) {
    nexrad_radial_ray *ray;
    uint8_t *data;

    if (radial == NULL) {
        return -1;
    }

    if (range > be16toh(radial->packet->rangebin_count)) {
        goto error_invalid_range;
    }

    if ((ray = nexrad_radial_get_ray(radial, azimuth, (void **)&data, NULL, NULL)) == NULL) {
        goto error_radial_get_ray;
    }

    return (int)data[range];

error_invalid_range:
error_radial_get_ray:
    return -1;
}

nexrad_radial_ray *nexrad_radial_read_ray(nexrad_radial *radial, void **data, uint16_t *runsp, uint16_t *binsp) {
    nexrad_radial_ray *ray;
    uint16_t runs, bins;
    size_t size;

    if (radial == NULL || radial->rays_left == 0) {
        return NULL;
    }

    ray = radial->current;

    if (radial->type == NEXRAD_RADIAL_RLE) {
        runs = be16toh(ray->size) * 2;
        bins = be16toh(radial->packet->rangebin_count);
        size = sizeof(nexrad_radial_ray) + runs;
    } else if (radial->type == NEXRAD_RADIAL_DIGITAL) {
        runs = 0;
        bins = be16toh(ray->size);
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

    /*
     * If the caller provided a pointer to an address in which to store the
     * number of runs in any given ray, then populate that value.
     */
    if (runsp)
        *runsp = runs;

    /*
     * If the caller provided a pointer to an address in which to store the
     * number of range bins of the current ray, then populate that value.
     */
    if (binsp)
        *binsp = bins;   

    /*
     * If the caller provided a pointer to an address to populate with a pointer
     * to the bins within the current ray, then provide that.
     */
    if (data)
        *data = (nexrad_radial_run *)((char *)ray + sizeof(nexrad_radial_ray));

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

static int _image_unpack_rle(nexrad_image *image, nexrad_radial *radial, nexrad_color_table_entry *entries) {
    nexrad_radial_ray *ray;
    nexrad_radial_run *data;

    uint16_t width, runs;

    if (nexrad_image_get_info(image, &width, NULL) < 0) {
        goto error_image_get_info;
    }

    while ((ray = nexrad_radial_read_ray(radial, (void **)&data, &runs, NULL)) != NULL) {
        int angle_start_i = (int16_t)be16toh(ray->angle_start);
        int angle_delta_i = (int16_t)be16toh(ray->angle_delta);

        int angle_start = round(NEXRAD_RADIAL_AZIMUTH_FACTOR * angle_start_i);
        int angle_end   = round(NEXRAD_RADIAL_AZIMUTH_FACTOR * (angle_start_i + angle_delta_i));

        int radius = be16toh(radial->packet->rangebin_first);

        int r;

        for (r=0; r<runs; r++) {
            uint8_t level = data[r].level * NEXRAD_RADIAL_RLE_FACTOR;

            if (level > 0) {
                nexrad_image_draw_arc_segment(image,
                    entries[level].r, entries[level].g, entries[level].b,
                    angle_start, angle_end,
                    radius, radius + data[r].length
                );
            }

            radius += data[r].length;
        }
    }

    return 0;

error_image_get_info:
    return -1;
}

static int _image_unpack_digital(nexrad_image *image, nexrad_radial *radial, nexrad_color_table_entry *entries) {
    nexrad_radial_ray *ray;
    unsigned char *data;

    uint16_t bins;

    while ((ray = nexrad_radial_read_ray(radial, (void **)&data, NULL, &bins)) != NULL) {
        int angle_start_i = (int16_t)be16toh(ray->angle_start);
        int angle_delta_i = (int16_t)be16toh(ray->angle_delta);

        int angle_start = round(NEXRAD_RADIAL_AZIMUTH_FACTOR * angle_start_i);
        int angle_end   = round(NEXRAD_RADIAL_AZIMUTH_FACTOR * (angle_start_i + angle_delta_i));

        int radius = be16toh(radial->packet->rangebin_first);

        int b;

        for (b=0; b<bins; b++) {
            uint8_t level = ((uint8_t *)data)[b];

            if (level > 0x0f) {
                nexrad_image_draw_arc_segment(image,
                    entries[level].r, entries[level].g, entries[level].b,
                    angle_start, angle_end,
                    radius, radius+1
                );
            }

            radius++;
        }
    }

    return 0;
}

nexrad_image *nexrad_radial_create_image(nexrad_radial *radial, nexrad_color_table *table) {
    nexrad_image *image;
    nexrad_color_table_entry *entries;
    size_t width, height, radius;

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

    if (radial->type == NEXRAD_RADIAL_DIGITAL) {
        if (_image_unpack_digital(image, radial, entries) < 0) goto error_image_unpack;
    } else if (radial->type == NEXRAD_RADIAL_RLE) {
        if (_image_unpack_rle(image, radial, entries)     < 0) goto error_image_unpack;
    }

    return image;

error_image_unpack:
    nexrad_image_destroy(image);

error_image_create:
error_color_table_get_entries:
    return NULL;
}
