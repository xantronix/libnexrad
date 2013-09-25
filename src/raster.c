#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <errno.h>
#include "pnglite.h"

#include <nexrad/raster.h>

struct _nexrad_raster {
    nexrad_raster_packet * packet;
    size_t                 bytes_read;
    size_t                 lines_left;
    nexrad_raster_line *   current;
};

struct _nexrad_raster_image {
    unsigned char * buf;

    size_t size;
    size_t width;
    size_t height;

    enum nexrad_raster_image_depth depth;
    enum nexrad_raster_image_color color;
};

static int _valid_packet(nexrad_raster_packet *packet) {
    if (packet == NULL) {
        return 0;
    }

    switch (be16toh(packet->type)) {
        case 0xba0f:
        case 0xba07: {
            break;
        }

        default: {
            return 0;
        }
    }

    if (
               be16toh(packet->flags_1) != 0x8000 ||
               be16toh(packet->flags_2) != 0x00c0 ||
      (int16_t)be16toh(packet->i)        >   2047 ||
      (int16_t)be16toh(packet->i)        <  -2048 ||
      (int16_t)be16toh(packet->j)        >   2047 ||
      (int16_t)be16toh(packet->j)        <  -2048 ||
               be16toh(packet->x_scale)  <      1 ||
               be16toh(packet->x_scale)  >     67 ||
               be16toh(packet->y_scale)  <      1 ||
               be16toh(packet->y_scale)  >     67 ||
               be16toh(packet->lines)    <      1 ||
               be16toh(packet->lines)    >    464
    ) {
        return 0;
    }

    return 1;
}

nexrad_raster *nexrad_raster_packet_open(nexrad_raster_packet *packet) {
    nexrad_raster *raster;

    if (!_valid_packet(packet)) {
        return NULL;
    }

    if ((raster = malloc(sizeof(nexrad_raster))) == NULL) {
        goto error_malloc;
    }

    raster->packet     = packet;
    raster->bytes_read = sizeof(nexrad_raster_packet);
    raster->lines_left = be16toh(packet->lines);
    raster->current    = (nexrad_raster_line *)((char *)packet + sizeof(nexrad_raster_packet));

    return raster;

error_malloc:
    return NULL;
}

static size_t _raster_line_width(nexrad_raster_line *line) {
    nexrad_raster_run *runs = (nexrad_raster_run *)((char *)line + sizeof(nexrad_raster_line));

    size_t width = 0;

    int i, nruns = be16toh(line->runs);

    for (i=0; i<nruns; i++) {
        width += runs[i].length;
    }

    return width;
}

nexrad_raster_line *nexrad_raster_read_line(nexrad_raster *raster, void **data, size_t *runsp) {
    nexrad_raster_line *line;
    size_t runs;

    if (raster == NULL) {
        return NULL;
    }

    if (raster->lines_left == 0) {
        return NULL;
    }

    line = raster->current;
    runs = sizeof(nexrad_raster_line) + be16toh(line->runs);

    /*
     * Advance the current line pointer beyond the line to follow.
     */
    raster->current = (nexrad_raster_line *)((char *)line + runs);

    /*
     * Increase the number of bytes read in the current raster packet.
     */
    raster->bytes_read += runs;

    /*
     * Decrement the number of lines left in the current raster packet.
     */
    raster->lines_left--;

    /*
     * If the caller provided a pointer to an address to store the resultant
     * raster line run count, then populate that address with that value.
     */
    if (runsp)
        *runsp = runs;

    /*
     * If the caller provided a pointer to an address to store a pointer to
     * the raster line's RLE-encoded data, then provide that address.
     */
    if (data)
        *data = (char *)line + sizeof(nexrad_raster_line);

    return line;
}

size_t nexrad_raster_bytes_read(nexrad_raster *raster) {
    if (raster == NULL) {
        return 0;
    }

    return raster->bytes_read;
}

void nexrad_raster_close(nexrad_raster *raster) {
    if (raster == NULL) {
        return;
    }

    raster->packet     = NULL;
    raster->bytes_read = 0;
    raster->lines_left = 0;
    raster->current    = NULL;

    free(raster);
}

int nexrad_raster_get_info(nexrad_raster *raster, size_t *widthp, size_t *heightp) {
    if (raster == NULL) {
        return -1;
    }

    /*
     * If we cannot identify that there is at least one raster line available
     * in the current raster packet, then do not attempt to populate data.
     */
    if (be16toh(raster->packet->lines) < 1) {
        return -1;
    }

    if (widthp) {
        nexrad_raster_line *line = (nexrad_raster_line *)((char *)raster->packet + sizeof(nexrad_raster_packet));

        *widthp = _raster_line_width(line);
    }

    if (heightp)
        *heightp = be16toh(raster->packet->lines);

    return 0;
}

static int _raster_unpack_rle(nexrad_raster *raster, unsigned char *buf, size_t width) {
    nexrad_raster_line *line;
    nexrad_raster_run  *data;

    size_t offset = 0, runs;

    while ((line = nexrad_raster_read_line(raster, (void **)&data, &runs)) != NULL) {
        int r;
        size_t linelen = 0;

        for (r=0; r<runs; r++) {
            memset(buf + offset, data[r].level * NEXRAD_RASTER_RLE_FACTOR, data[r].length);

            linelen += data[r].length;
            offset  += data[r].length;

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
}

nexrad_raster_image *nexrad_raster_create_image(nexrad_raster *raster, enum nexrad_raster_image_depth depth, enum nexrad_raster_image_color color) {
    nexrad_raster_image *image;
    size_t width, height, size;
    unsigned char *buf;

    if (depth != NEXRAD_RASTER_IMAGE_8BPP || color != NEXRAD_RASTER_IMAGE_GRAYSCALE) {
        errno = EINVAL;
        return NULL;
    }

    if (nexrad_raster_get_info(raster, &width, &height) < 0) {
        goto error_raster_get_info;
    }

    if ((image = malloc(sizeof(nexrad_raster_image))) == NULL) {
        goto error_malloc_raster_image;
    }

    size = width * height * depth;

    if ((buf = malloc(size)) == NULL) {
        goto error_malloc_buf;
    }

    if (_raster_unpack_rle(raster, buf, width) < 0) {
        goto error_raster_unpack_rle;
    }

    image->buf    = buf;
    image->size   = size;
    image->width  = width;
    image->height = height;
    image->depth  = depth;
    image->color  = color;

    return image;

error_raster_unpack_rle:
    free(buf);

error_malloc_buf:
    free(image);

error_malloc_raster_image:
error_raster_get_info:
    return NULL;
}

int nexrad_raster_image_get_info(nexrad_raster_image *image, size_t *width, size_t *height, enum nexrad_raster_image_depth *depth, enum nexrad_raster_image_color *color) {
    if (image == NULL) {
        return -1;
    }

    if (width)
        *width = image->width;

    if (height)
        *height = image->height;

    if (depth)
        *depth = image->depth;

    if (color)
        *color = image->color;

    return 0;
}

unsigned char *nexrad_raster_image_get_buf(nexrad_raster_image *image, size_t *size) {
    if (image == NULL) {
        return NULL;
    }

    if (size)
        *size = image->size;

    return image->buf;
}

int nexrad_raster_image_save_png(nexrad_raster_image *image, const char *path) {
    png_t png;

    if (image == NULL || path == NULL) {
        return -1;
    }

    memset(&png, '\0', sizeof(png));

    png_init(NULL, NULL);

    if (png_open_file_write(&png, path) < 0) {
        goto error_open_file_write;
    }

    if (png_set_data(&png,
        image->width, image->height, image->depth * 8, image->color, image->buf
    ) < 0) {
        goto error_set_data;
    }

    if (png_close_file(&png) < 0) {
        goto error_close_file;
    }

    return 0;

error_close_file:
error_set_data:
    png_close_file(&png);

error_open_file_write:
    return -1;
}

void nexrad_raster_image_destroy(nexrad_raster_image *image) {
    if (image == NULL) {
        return;
    }

    if (image->buf) {
        free(image->buf);
    }

    image->buf    = NULL;
    image->size   = 0;
    image->width  = 0;
    image->height = 0;
    image->depth  = 0;
    image->color  = 0;

    free(image);
}
