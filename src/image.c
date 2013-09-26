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

static inline void _int_order(int *a, int *b) {
    if (*a > *b) {
        int c = *a;

        *a = *b;
        *b =  c;
    }
}

void nexrad_image_draw_arc_segment(nexrad_image *image, uint8_t level, int amin, int amax, int rmin, int rmax) {
    int x, xc, y, yc, r, re, w;
    int xmin, xmax, ymin, ymax;
    unsigned char *buf;
    double rad = M_PI / 180;

    enum {
        NONE, ESE, SSE, SSW, WSW, WNW, NNW, NNE, ENE
    } octant = NONE;

    if (image == NULL || amin > amax || rmin > rmax) {
        return;
    }

    _int_order(&amin, &amax);
    _int_order(&rmin, &rmax);

    if ((amin >=  90 && amin <= 135) || (amax >=  90 && amax < 135)) octant = ESE;
    if ((amin >= 135 && amin <= 180) || (amax >= 135 && amax < 180)) octant = SSE;
    if ((amin >= 180 && amin <= 225) || (amax >= 180 && amax < 225)) octant = SSW;
    if ((amin >= 225 && amin <= 270) || (amax >= 225 && amax < 270)) octant = WSW;
    if ((amin >= 270 && amin <= 315) || (amax >= 270 && amax < 315)) octant = WNW;
    if ((amin >= 315 && amin <= 360) || (amax >= 315 && amax < 360)) octant = NNW;
    if ((amin >=   0 && amin <=  45) || (amax >=   0 && amax <  45)) octant = NNE;
    if ((amin >=  45 && amin <=  90) || (amax >=  45 && amax <  90)) octant = ENE;

    if (!octant) {
        return;
    }

    buf = image->buf;
    w   = image->width;

    xc = image->x_center;
    yc = image->y_center;

    for (r=rmin; r<rmax; r++) {
        xmin = (int)round(r * cos(rad * amin));
        ymin = (int)round(r * sin(rad * amin));
        xmax = (int)round(r * cos(rad * amax));
        ymax = (int)round(r * sin(rad * amax));

        _int_order(&xmin, &xmax);
        _int_order(&ymin, &ymax);

        x  = r;
        y  = 0;
        re = 1 - x;

        while (x >= y) {
            if ((x >= xmin && x <= xmax) || (y >= ymin && y <= ymax)) {
                switch (octant) {
                    case ESE: _buf_write_pixel(buf, level,  x+xc,  y+yc, w); break;
                    case SSE: _buf_write_pixel(buf, level,  y+xc,  x+yc, w); break;
                    case SSW: _buf_write_pixel(buf, level, -y+xc,  x+yc, w); break;
                    case WSW: _buf_write_pixel(buf, level, -x+xc,  y+yc, w); break;
                    case WNW: _buf_write_pixel(buf, level, -x+xc, -y+yc, w); break;
                    case NNW: _buf_write_pixel(buf, level, -y+xc, -x+yc, w); break;
                    case NNE: _buf_write_pixel(buf, level,  x+xc, -y+yc, w); break;
                    case ENE: _buf_write_pixel(buf, level,  y+xc, -x+yc, w); break;

                    default: {
                        break;
                    }
                }
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
