#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <math.h>
#include <errno.h>

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

int nexrad_radial_get_info(nexrad_radial *radial, uint16_t *binsp, uint16_t *raysp) {
    if (radial == NULL) {
        return -1;
    }

    if (binsp)
        *binsp = be16toh(radial->packet->rangebin_count);

    if (raysp)
        *raysp = be16toh(radial->packet->rays);

    return 0;
}

static int _image_unpack_rle(nexrad_image *image, nexrad_radial *radial, nexrad_color_table_entry *entries) {
    nexrad_radial_ray *ray;
    nexrad_radial_run *data;

    uint16_t width, runs;

    if (nexrad_image_get_info(image, &width, NULL) < 0) {
        goto error_image_get_info;
    }

    while ((ray = nexrad_radial_read_ray(radial, (void **)&data, &runs, NULL)) != NULL) {
        int r;

        int angle_start_i = (int16_t)be16toh(ray->angle_start);
        int angle_delta_i = (int16_t)be16toh(ray->angle_delta);

        int angle_start = round(NEXRAD_RADIAL_ANGLE_FACTOR * angle_start_i);
        int angle_end   = round(NEXRAD_RADIAL_ANGLE_FACTOR * (angle_start_i + angle_delta_i));

        int radius = 0;

        for (r=0; r<runs; r++) {
            uint8_t level = data[r].level * NEXRAD_RADIAL_RLE_FACTOR;

            nexrad_image_draw_arc_segment(image,
                entries[level].r, entries[level].g, entries[level].b,
                angle_start, angle_end,
                radius, radius + data[r].length
            );

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

        int angle_start = round(NEXRAD_RADIAL_ANGLE_FACTOR * angle_start_i);
        int angle_end   = round(NEXRAD_RADIAL_ANGLE_FACTOR * (angle_start_i + angle_delta_i));

        int b;

        for (b=0; b<bins; b++) {
            uint8_t level = ((uint8_t *)data)[b];

            nexrad_image_draw_arc_segment(image,
                entries[level].r, entries[level].g, entries[level].b,
                angle_start, angle_end,
                b, b+1
            );
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

    radius = be16toh(radial->packet->rangebin_count);
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
