#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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

struct _nexrad_radial_image {
    cairo_surface_t * surface;
    cairo_format_t    format;

    size_t radius;
    size_t width;
    size_t height;
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

static inline void _context_set_color(cairo_t *cr, uint8_t level) {
    double c = 255.0 / (double)level;

    cairo_set_source_rgb(cr, c, c, c);
}

static int _radial_unpack_rle(nexrad_radial *radial, nexrad_radial_image *image) {
    nexrad_radial_ray *ray;
    nexrad_radial_run *data;
    cairo_t *cr;

    size_t runs;
    size_t width  = image->width;
    size_t center = image->radius;

    if ((cr = cairo_create(image->surface)) == NULL) {
        goto error_context_create;
    }

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

    while ((ray = nexrad_radial_read_ray(radial, (void **)&data, &runs, NULL)) != NULL) {
        double angle_start = 0.1 * (double)be16toh(ray->angle_start);
        double angle_end   = 0.1 * (double)be16toh(ray->angle_delta) + angle_start;

        size_t linelen = 0;

        int r;

        for (r=0; r<runs; r++) {
            _context_set_color(cr, data[r].level);
            cairo_set_line_width(cr, (double)data[r].length);

            cairo_arc(cr, center, center, r, angle_start, angle_end);
            cairo_stroke(cr);

            linelen += data[r].length;

            if (linelen >= width) break;
        }
    }

    cairo_destroy(cr);

    return 0;

error_context_create:
    return -1;
}

static int _radial_unpack_digital(nexrad_radial *radial, nexrad_radial_image *image) {
    nexrad_radial_ray *ray;
    unsigned char *data;
    cairo_t *cr;

    size_t center = image->radius;
    size_t bins;

    if ((cr = cairo_create(image->surface)) == NULL) {
        goto error_context_create;
    }

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1.0);

    while ((ray = nexrad_radial_read_ray(radial, (void **)&data, NULL, &bins)) != NULL) {
        double angle_start = 0.1 * (double)be16toh(ray->angle_start);
        double angle_end   = 0.1 * (double)be16toh(ray->angle_delta) + angle_start;

        int r;

        for (r=0; r<bins; r++) {
            _context_set_color(cr, data[r]);

            cairo_arc(cr, center, center, r, angle_start, angle_end);
            cairo_stroke(cr);
        }
    }

    cairo_destroy(cr);

    return 0;

error_context_create:
    return -1;
}

nexrad_radial_image *nexrad_radial_create_image(nexrad_radial *radial, cairo_format_t format) {
    nexrad_radial_image *image;
    cairo_surface_t *surface;
    size_t radius, width, height;

    if (radial == NULL) {
        return NULL;
    }

    radius = be16toh(radial->packet->rangebin_count);
    width  = 2 * radius;
    height = 2 * radius;

    if ((image = malloc(sizeof(nexrad_radial_image))) == NULL) {
        goto error_malloc_image;
    }

    if ((surface = cairo_image_surface_create(format, width, height)) == NULL) {
        goto error_image_surface_create;
    }

    image->surface = surface;
    image->format  = format;
    image->width   = width;
    image->height  = height;

    if (radial->type == NEXRAD_RADIAL_DIGITAL) {
        if (_radial_unpack_digital(radial, image) < 0) goto error_radial_unpack;
    } else if (radial->type == NEXRAD_RADIAL_RLE) {
        if (_radial_unpack_rle(radial, image)     < 0) goto error_radial_unpack;
    }

    return image;

error_radial_unpack:
    cairo_surface_destroy(surface);

error_image_surface_create:
    free(image);

error_malloc_image:
    return NULL;
}

int nexrad_radial_get_image_info(nexrad_radial_image *image, size_t *width, size_t *height, cairo_format_t *format) {
    if (image == NULL) {
        return -1;
    }

    if (width)
        *width = image->width;

    if (height)
        *height = image->height;

    if (format)
        *format = image->format;

    return 0;
}

cairo_surface_t *nexrad_radial_image_get_surface(nexrad_radial_image *image) {
    if (image == NULL) {
        return NULL;
    }

    return image->surface;
}

int nexrad_radial_image_save_png(nexrad_radial_image *image, const char *path) {
    if (image == NULL || path == NULL) {
        return -1;
    }

    if (cairo_surface_write_to_png(image->surface, path) != CAIRO_STATUS_SUCCESS) {
        goto error_surface_write_to_png;
    }

    return 0;

error_surface_write_to_png:
    return -1;
}

void nexrad_radial_image_destroy(nexrad_radial_image *image) {
    if (image == NULL) {
        return;
    }

    if (image->surface)
        cairo_surface_destroy(image->surface);

    image->surface = NULL;
    image->format  = 0;
    image->radius  = 0;
    image->width   = 0;
    image->height  = 0;

    free(image);
}
