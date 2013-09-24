#include <stdlib.h>
#include <sys/types.h>
#include "pnglite.h"

#include <nexrad/image.h>

struct _nexrad_image {
    unsigned char * buf;
    size_t          size;

    size_t width;
    size_t height;
    size_t x_center;
    size_t y_center;

    enum nexrad_image_depth depth;
    enum nexrad_image_color color;
    enum nexrad_image_type  type;
};

static size_t _image_size(size_t width, size_t height, enum nexrad_image_depth depth) {
    return width * height * depth;
}

static int _valid_args(size_t width, size_t height, enum nexrad_image_depth depth, enum nexrad_image_color color) {
    if (width == 0 || height == 0) {
        return 0;
    }

    switch (depth) {
        case NEXRAD_IMAGE_32BPP:
        case NEXRAD_IMAGE_24BPP:
        case NEXRAD_IMAGE_16BPP:
        case NEXRAD_IMAGE_8BPP: {
            break;
        }

        default: {
            return 0;
        }
    }

    switch (color) {
        case NEXRAD_IMAGE_GRAYSCALE:
        case NEXRAD_IMAGE_TRUECOLOR:
        case NEXRAD_IMAGE_INDEXED:
        case NEXRAD_IMAGE_GRAYSCALE_ALPHA:
        case NEXRAD_IMAGE_TRUECOLOR_ALPHA: {
            break;
        }

        default: {
            return 0;
        }
    }

    return 1;
}

nexrad_image *nexrad_image_create(size_t width, size_t height, enum nexrad_image_depth depth, enum nexrad_image_color color) {
    nexrad_image *image;
    size_t size;
    unsigned char *buf;

    if (!_valid_args(width, height, depth, color)) {
        return NULL;
    }

    if ((image = malloc(sizeof(nexrad_image))) == NULL) {
        goto error_malloc_image;
    }

    size = _image_size(width, height, depth);

    if ((buf = malloc(size)) == NULL) {
        goto error_malloc_buf;
    }

    memset(buf, '\0', size);

    image->buf      = buf;
    image->size     = size;
    image->width    = width;
    image->height   = height;
    image->x_center = width  / 2;
    image->y_center = height / 2;
    image->depth    = depth;
    image->color    = color;

    return image;

error_malloc_buf:
    free(image);

error_malloc_image:
    return NULL;
}

int nexrad_image_get_info(nexrad_image *image, size_t *width, size_t *height, enum nexrad_image_depth *depth, enum nexrad_image_color *color) {
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

unsigned char * nexrad_image_get_buf(nexrad_image *image, size_t *size) {
    if (image == NULL) {
        return NULL;
    }

    if (size)
        *size = image->size;

    return image->buf;
}

static inline void _buf_draw_pixel(unsigned char *buf, int c, int x, int y, int w) {
    buf[(y*w) + x] = c;
}

void nexrad_image_draw_arc_section(nexrad_image *image, int level, float angle_min, float angle_max, int radius_min, int radius_max) {
    int w      = image->width;
    int xc     = image->x_center;
    int yc     = image->y_center;
    int x      = image->x_center;
    int y      = 0;
    int rerror = 1-x;

    unsigned char *buf = image->buf;

    if (
        image      == NULL ||
        angle_min  <    0  ||
        angle_min  >= 360  ||
        angle_max  >= 360  ||
        radius_min <    y  ||
        radius_max >    x
    ) {
        return;
    }

    while (x >= y) {
        _buf_draw_pixel(buf, level,  x+xc,  y+yc, w);
        _buf_draw_pixel(buf, level,  y+xc,  x+yc, w);
        _buf_draw_pixel(buf, level, -x+xc,  y+yc, w);
        _buf_draw_pixel(buf, level, -y+xc,  x+yc, w);
        _buf_draw_pixel(buf, level, -x+xc, -y+yc, w);
        _buf_draw_pixel(buf, level, -y+xc, -x+yc, w);
        _buf_draw_pixel(buf, level,  x+xc, -y+yc, w);
        _buf_draw_pixel(buf, level,  y+xc, -x+yc, w);

        y++;

        if (rerror < 0) {
            rerror += 2 * y + 1;
        } else {
            x--;
            rerror += 2 * (y - x + 1);
        }
    }
}

int nexrad_image_save_png(nexrad_image *image, const char *path) {
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
    return -1;

error_set_data:
    png_close_file(&png);

error_open_file_write:
    return -1;
}

void nexrad_image_destroy(nexrad_image *image) {
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
