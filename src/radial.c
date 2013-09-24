#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <errno.h>

#include <nexrad/radial.h>

struct _nexrad_radial {
    nexrad_radial_packet *  packet;
    enum nexrad_radial_type type;
    size_t                  bytes_read;
    size_t                  rays_left;
    nexrad_radial_ray *     current;
};

static int _valid_packet(nexrad_radial_packet *packet, enum nexrad_radial_type type) {
    switch (type) {
        case NEXRAD_RADIAL_RLE:
        case NEXRAD_RADIAL_DIGITAL: {
            break;
        }

        default: {
            return 0;
        }
    }

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

nexrad_radial_ray *nexrad_radial_read_ray(nexrad_radial *radial, void **data, size_t *runsp, size_t *binsp) {
    nexrad_radial_ray *ray;
    size_t runs, bins, size;

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

int nexrad_radial_get_info(nexrad_radial *radial, size_t *binsp, size_t *raysp) {
    if (radial == NULL) {
        return -1;
    }

    if (binsp)
        *binsp = be16toh(radial->packet->rangebin_count);

    if (raysp)
        *raysp = be16toh(radial->packet->rays);

    return 0;
}

static int _image_unpack_rle(nexrad_image *image, nexrad_radial *radial, size_t width) {
    nexrad_radial_ray *ray;
    nexrad_radial_run *data;
    unsigned char *buf;

    size_t offset = 0, runs;

    if ((buf = nexrad_image_get_buf(image, NULL)) == NULL) {
        goto error_image_get_buf;
    }

    while ((ray = nexrad_radial_read_ray(radial, (void **)&data, &runs, NULL)) != NULL) {
        int r;
        size_t linelen = 0;

        for (r=0; r<runs; r++) {
            memset(buf + offset, data[r].level * NEXRAD_RADIAL_RLE_FACTOR, data[r].length);

            offset  += data[r].length;
            linelen += data[r].length;

            if (linelen >= width) break;
        }

        /*
         * If the current run failed to extend to the line width, then pad the
         * rest of the line with black pixels.
         */

        if (linelen < width) {
            size_t padding = width - linelen;

            memset(buf + offset, '\0', padding);

            offset += padding;
        }
    }

    return 0;

error_image_get_buf:
    return -1;
}

static int _image_unpack_digital(nexrad_image *image, nexrad_radial *radial) {
    nexrad_radial_ray *ray;
    unsigned char *buf, *data;

    size_t offset = 0, bins;

    if ((buf = nexrad_image_get_buf(image, NULL)) == NULL) {
        goto error_image_get_buf;
    }

    while ((ray = nexrad_radial_read_ray(radial, (void **)&data, NULL, &bins)) != NULL) {
        memcpy(buf + offset, data, bins);

        offset += bins;
    }

    return 0;

error_image_get_buf:
    return -1;
}

nexrad_image *nexrad_radial_create_image(nexrad_radial *radial, enum nexrad_image_depth depth, enum nexrad_image_color color) {
    nexrad_image *image;
    size_t width, height;

    if (radial == NULL) {
        return NULL;
    }

    if (depth != NEXRAD_IMAGE_8BPP || color != NEXRAD_IMAGE_GRAYSCALE) {
        errno = EINVAL;
        return NULL;
    }

    width  = be16toh(radial->packet->rangebin_count);
    height = be16toh(radial->packet->rays);

    if ((image = nexrad_image_create(width, height, depth, color)) == NULL) {
        goto error_image_create;
    }

    if (radial->type == NEXRAD_RADIAL_DIGITAL) {
        if (_image_unpack_digital(image, radial)    < 0) goto error_image_unpack;
    } else if (radial->type == NEXRAD_RADIAL_RLE) {
        if (_image_unpack_rle(image, radial, width) < 0) goto error_image_unpack;
    }

    return image;

error_image_unpack:
    nexrad_image_destroy(image);

error_image_create:
    return NULL;
}
