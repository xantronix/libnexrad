#include <stdlib.h>
#include <string.h>
#include <endian.h>

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

nexrad_radial_ray *nexrad_radial_read_ray(nexrad_radial *radial, void **data, size_t *runsp, size_t *binsp, size_t *sizep) {
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
     * If the caller provided a pointer to an address in which to store the size
     * of the current ray (including header), then populate that value.
     */
    if (sizep)
        *sizep = size;

    /*
     * If the caller provided a pointer to an address to populate with a pointer
     * to the bins within the current ray, then provide that.
     */
    if (data)
        *data = (nexrad_radial_run *)((char *)ray + sizeof(nexrad_radial_ray));

    return ray;
}

int nexrad_radial_get_info(nexrad_radial *radial, size_t *raysp, size_t *binsp) {
    if (radial == NULL) {
        return -1;
    }

    if (raysp)
        *raysp = be16toh(radial->packet->rays);

    if (binsp)
        *binsp = be16toh(radial->packet->rangebin_count);

    return 0;
}

static void _copy_rle_data(unsigned char *buf, nexrad_radial *radial) {
    nexrad_radial_ray *ray;
    nexrad_radial_run *data;

    size_t offset = 0, runs, size;

    while ((ray = nexrad_radial_read_ray(radial, (void **)&data, &runs, NULL, &size)) != NULL) {
        int r;

        for (r=0; r<runs; r++) {
            memset(buf + offset, data[r].level * NEXRAD_RADIAL_RLE_FACTOR, data[r].length);

            offset += data[r].length;
        }
    }
}

static void _copy_digital_data(unsigned char *buf, nexrad_radial *radial) {
    nexrad_radial_ray *ray;
    unsigned char *data;

    size_t offset = 0, bins, size;

    while ((ray = nexrad_radial_read_ray(radial, (void **)&data, NULL, &bins, &size)) != NULL) {
        memcpy(buf + offset, data, bins);

        offset += bins;
    }
}

nexrad_image *nexrad_radial_create_image(nexrad_radial *radial) {
    nexrad_image *image;
    size_t width, height, size;
    enum nexrad_image_depth depth;
    enum nexrad_image_color color;
    unsigned char *buf;

    if (radial == NULL) {
        return NULL;
    }

    width  = be16toh(radial->packet->rangebin_count);
    height = be16toh(radial->packet->rays);
    depth  = NEXRAD_IMAGE_8BPP;
    color  = NEXRAD_IMAGE_GRAYSCALE;

    if ((image = nexrad_image_create(width, height, depth, color)) == NULL) {
        goto error_image_create;
    }

    if ((buf = nexrad_image_get_buf(image)) == NULL) {
        goto error_image_get_buf;
    }

    if ((size = nexrad_image_get_size(image)) < 0) {
        goto error_image_get_size;
    }

    if (radial->type == NEXRAD_RADIAL_DIGITAL) {
        _copy_digital_data(buf, radial);
    } else if (radial->type == NEXRAD_RADIAL_RLE) {
        _copy_rle_data(buf, radial);
    }

    return image;

error_image_get_size:
error_image_get_buf:
    nexrad_image_destroy(image);

error_image_create:
    return NULL;
}
