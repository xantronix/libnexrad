#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

static inline void _buf_write_pixel(unsigned char *buf, uint8_t c, int x, int y, int w) {
    buf[(y*w) + x] = c;
}

static void _swap_double(double *a, double *b) {
    double c = *a;

    *a = *b;
    *b =  c;
}

static void _swap_int(int *a, int *b) {
    int c = *a;

    *a = *b;
    *b =  c;
}

void nexrad_image_draw_line(nexrad_image *image, uint8_t level, int x0, int y0, int x1, int y1) {
    int x, y, w;
    double x_delta = x1 - x0;
    double y_delta = y1 - y0;
    double error, error_delta;
    unsigned char *buf;

    if (image == NULL) {
        return;
    }

    buf = image->buf;
    w   = image->width;

    x_delta = x1 - x0;
    y_delta = y1 - y0;

    error = 0.0;

    if (x_delta == 0) {
        error_delta = 0;
    } else {
        error_delta = y_delta / x_delta;
    }

    if (error_delta < 0) error_delta = 0 - error_delta;

    fprintf(stderr, "x_delta: %f, y_delta: %f\n", x_delta, y_delta);
    fprintf(stderr, "error_delta is %f\n", error_delta);

    y = y0;

    for (x=x0; x<x1; x++) {
        fprintf(stderr, "x is %d\n", x);

        _buf_write_pixel(buf, level, x, y, w);

        error += error_delta;

        if (error >= 0.5) {
            y++;
            fprintf(stderr, "Incrementing y to %d\n", y);
            error -= 1.0;
        }
    }
}

void nexrad_image_draw_arc_segment(nexrad_image *image, uint8_t level, int amin, int amax, int rmin, int rmax) {
    int x, xc, y, yc, w, r, re;
    unsigned char *buf;

    double rad = (M_PI / 180);

    if (
        image == NULL ||
        amin   > amax ||
        rmin   > rmax
    ) {
        return;
    }

    xc = image->x_center;
    yc = image->y_center;
    w  = image->width;

    buf = image->buf;

    for (r=rmin; r<rmax; r++) {
        int xa = (int)round(r * cos(rad * amin));
        int xb = (int)round(r * cos(rad * amax));
        int ya = (int)round(r * sin(rad * amin));
        int yb = (int)round(r * sin(rad * amax));

        int xmin, xmax, ymin, ymax;

        if (xa < xb) {
            xmin = xa;
            xmax = xb;
        } else {
            xmin = xb;
            xmax = xa;
        }

        if (ya < yb) {
            ymin = ya;
            ymax = yb;
        } else {
            ymin = yb;
            ymax = ya;
        }

        x  = r;
        y  = 0;
        re = 1 - x;

        while (x >= y) {
            if (x >= xmin && x <= xmax && y >= ymin && y <= ymax) {
                _buf_write_pixel(buf, level,  x+xc,  y+yc, w);
                _buf_write_pixel(buf, level,  y+xc,  x+yc, w);
                _buf_write_pixel(buf, level, -x+xc,  y+yc, w);
                _buf_write_pixel(buf, level, -y+xc,  x+yc, w);
                _buf_write_pixel(buf, level, -x+xc, -y+yc, w);
                _buf_write_pixel(buf, level, -y+xc, -x+yc, w);
                _buf_write_pixel(buf, level,  x+xc, -y+yc, w);
                _buf_write_pixel(buf, level,  y+xc, -x+yc, w);
            }

            y++;

            if (re < 0) {
                re += 2 * y + 1;
            } else {
                x--;
                re += 2 * (y - x + 1);
            }
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
