#include <stdlib.h>
#include <math.h>
#include "pnglite.h"

#include <nexrad/image.h>

#define NEXRAD_IMAGE_PIXEL_BYTES  4
#define NEXRAD_IMAGE_COLOR_DEPTH  8
#define NEXRAD_IMAGE_COLOR_FORMAT PNG_TRUECOLOR_ALPHA

#define NEXRAD_IMAGE_PIXEL_OFFSET(x, y, w) \
  ((y * w * NEXRAD_IMAGE_PIXEL_BYTES) + x * NEXRAD_IMAGE_PIXEL_BYTES)

struct _nexrad_image {
    unsigned char * buf;
    size_t          size;

    uint16_t width;
    uint16_t height;
    uint16_t radius;
    uint16_t x_center;
    uint16_t y_center;
};

static size_t _image_size(uint16_t width, uint16_t height) {
    return NEXRAD_IMAGE_PIXEL_BYTES * width * height;
}

nexrad_image *nexrad_image_create(uint16_t width, uint16_t height) {
    nexrad_image *image;
    size_t size;
    unsigned char *buf;

    if (width == 0 || height == 0) {
        return NULL;
    }

    if ((image = malloc(sizeof(nexrad_image))) == NULL) {
        goto error_malloc_image;
    }

    size = _image_size(width, height);

    if ((buf = malloc(size)) == NULL) {
        goto error_malloc_buf;
    }

    memset(buf, '\0', size);

    image->buf      = buf;
    image->size     = size;
    image->width    = width;
    image->height   = height;
    image->radius   = width > height? height: width;
    image->x_center = width  >> 1;
    image->y_center = height >> 1;

    return image;

error_malloc_buf:
    free(image);

error_malloc_image:
    return NULL;
}

int nexrad_image_get_info(nexrad_image *image, uint16_t *width, uint16_t *height) {
    if (image == NULL) {
        return -1;
    }

    if (width)
        *width = image->width;

    if (height)
        *height = image->height;

    return 0;
}

unsigned char *nexrad_image_get_buf(nexrad_image *image, size_t *size) {
    if (image == NULL) {
        return NULL;
    }

    if (size)
        *size = image->size;

    return image->buf;
}

static inline void _buf_write_pixel(unsigned char *buf, uint8_t r, uint8_t g, uint8_t b, uint16_t x, uint16_t y, uint16_t w) {
    size_t offset = NEXRAD_IMAGE_PIXEL_OFFSET(x, y, w);

    buf[offset]   = r;
    buf[offset+1] = g;
    buf[offset+2] = b;
    buf[offset+3] = 0xff;
}

static inline void _int_swap(int *a, int *b) {
    int c = *a;

    *a = *b;
    *b =  c;
}

static inline void _int_order(int *a, int *b) {
    if (*a > *b) {
        _int_swap(a, b);
    }
}

void nexrad_image_draw_pixel(nexrad_image *image, uint8_t r, uint8_t g, uint8_t b, uint16_t x, uint16_t y) {
    if (image == NULL) {
        return;
    }

    if (x < 0 || x > image->width || y < 0 || y > image->height) {
        return;
    }

    _buf_write_pixel(image->buf, r, g, b, x, y, image->width);
}

void nexrad_image_draw_run(nexrad_image *image, uint8_t r, uint8_t g, uint8_t b, uint16_t x, uint16_t y, uint16_t length) {
    unsigned char *buf;
    size_t offset;
    uint16_t i;

    if (image == NULL) {
        return;
    }

    if (x >= image->width || y >= image->height || x + length > image->width) {
        return;
    }

    buf    = image->buf;
    offset = NEXRAD_IMAGE_PIXEL_OFFSET(x, y, image->width);

    for (i=0; i<length; i++) {
        buf[offset++] = r;
        buf[offset++] = g;
        buf[offset++] = b;
        buf[offset++] = 0xff;
    }
}

void nexrad_image_draw_arc_segment(nexrad_image *image, uint8_t r, uint8_t g, uint8_t b, int amin, int amax, int rmin, int rmax) {
    int x, xc, y, yc, radius, re, w;
    int xmin, xmax, ymin, ymax;
    unsigned char *buf;
    static double rad = M_PI / 180;

    enum {
        NONE, ESE, SSE, SSW, WSW, WNW, NNW, NNE, ENE
    } octant = NONE;

    if (image == NULL || amin > amax || rmin > rmax) {
        return;
    }

    if (rmin >= image->radius || rmax >= image->radius) {
        return;
    }

    /*
     * Ensure the angle and radius minimum and maximum arguments are well
     * ordered and fall within a single rotation of a circle.
     */
    _int_order(&amin, &amax);
    _int_order(&rmin, &rmax);

    amin = amin % 360;

    if (amax && (amax = amax % 360) == 0) {
        amax = 360;
    }

    /*
     * Then, determine which octant the arc range pertains to.
     */
    if (amin >=  90 && amin <= 135 && amax >=  90 && amax <= 135) octant = ESE;
    if (amin >= 135 && amin <= 180 && amax >= 135 && amax <= 180) octant = SSE;
    if (amin >= 180 && amin <= 225 && amax >= 180 && amax <= 225) octant = SSW;
    if (amin >= 225 && amin <= 270 && amax >= 225 && amax <= 270) octant = WSW;
    if (amin >= 270 && amin <= 315 && amax >= 270 && amax <= 315) octant = WNW;
    if (amin >= 315 && amin <= 360 && amax >= 315 && amax <= 360) octant = NNW;
    if (amin >=   0 && amin <=  45 && amax >=   0 && amax <=  45) octant = NNE;
    if (amin >=  45 && amin <=  90 && amax >=  45 && amax <=  90) octant = ENE;

    /*
     * Draw nothing if the angle minimum and maximum do not span a single
     * octant.
     */
    if (!octant) {
        return;
    }

    /*
     * Scale down the angle minimum and maximum to a 45 degree range within the
     * current octant.
     */
    amin = amin % 45;

    if (amax && (amax = amax % 45) == 0) {
        amax = 45;
    }

    /*
     * Set up a bit more state for the eventual pixel writing operations.
     */
    buf = image->buf;
    w   = image->width;

    xc = image->x_center;
    yc = image->y_center;

    /*
     * Using a modified Bresenham's midpoint circle algorithm, every second
     * octant (starting from NNE, clockwise) is drawn backwards.  Therefore,
     * in order to ensure angle minimum and maximum are visualized properly, we
     * need to swap the order of the padding of the angle boundaries within the
     * 45 degree octant.
     */
    switch (octant) {
        case ENE:
        case SSE:
        case WSW:
        case NNW: {
            int adelta = 45 - amax;

            amax = 45 - amin;
            amin = adelta;
        }

        default: break;
    }

    /*
     * For each radius in pixels from center point...
     */
    for (radius=rmin; radius<rmax; radius++) {
        /*
         * Determine the boundings of the current octant's arc segment.
         */
        xmin = (int)round(radius * cos(rad * amin));
        ymin = (int)round(radius * sin(rad * amin));
        xmax = (int)round(radius * cos(rad * amax));
        ymax = (int)round(radius * sin(rad * amax));

        _int_order(&xmin, &xmax);
        _int_order(&ymin, &ymax);

        x  = radius;
        y  = 0;
        re = 1 - x;

        /*
         * Loop only while X is above or equal to the minimum, and Y is below or
         * equal to the maximum.
         */
        while (x >= xmin && y <= ymax) {
            int decrx = (re >= 0)?                1: 0;
            int draw  = (x <= xmax && y >= ymin)? 1: 0;

            /*
             * However, when determining whether to draw the current point, as
             * X's minimum and Y's maximum may be reached prior to this condition,
             * make a separate check for the sake of expediency to determine if
             * the current points fall within X's maximum and Y's minimum, and
             * plot a point under the correct octant as needed.
             */
            if (draw) {
                switch (octant) {
                    case ESE: _buf_write_pixel(buf, r, g, b,  x+xc,  y+yc, w); break;
                    case SSE: _buf_write_pixel(buf, r, g, b,  y+xc,  x+yc, w); break;
                    case SSW: _buf_write_pixel(buf, r, g, b, -y+xc,  x+yc, w); break;
                    case WSW: _buf_write_pixel(buf, r, g, b, -x+xc,  y+yc, w); break;
                    case WNW: _buf_write_pixel(buf, r, g, b, -x+xc, -y+yc, w); break;
                    case NNW: _buf_write_pixel(buf, r, g, b, -y+xc, -x+yc, w); break;
                    case NNE: _buf_write_pixel(buf, r, g, b,  y+xc, -x+yc, w); break;
                    case ENE: _buf_write_pixel(buf, r, g, b,  x+xc, -y+yc, w); break;

                    default: {
                        break;
                    }
                }

                /*
                 * If X will be decremented soon, then we will need to fill in the
                 * next pixel over to prevent a Moire pattern from presenting itself
                 * in any filled arcs.
                 */
                if (decrx) {
                    switch (octant) {
                        case ESE: _buf_write_pixel(buf, r, g, b,  x+xc-1,  y+yc, w); break;
                        case SSE: _buf_write_pixel(buf, r, g, b,  y+xc+1,  x+yc, w); break;
                        case SSW: _buf_write_pixel(buf, r, g, b, -y+xc-1,  x+yc, w); break;
                        case WSW: _buf_write_pixel(buf, r, g, b, -x+xc+1,  y+yc, w); break;
                        case WNW: _buf_write_pixel(buf, r, g, b, -x+xc+1, -y+yc, w); break;
                        case NNW: _buf_write_pixel(buf, r, g, b, -y+xc-1, -x+yc, w); break;
                        case NNE: _buf_write_pixel(buf, r, g, b,  y+xc+1, -x+yc, w); break;
                        case ENE: _buf_write_pixel(buf, r, g, b,  x+xc-1, -y+yc, w); break;

                        default: {
                            break;
                        }
                    }
                }
            }

            y++;

            if (decrx) {
                x--;
                re += 2 * (y - x + 1);
            } else {
                re += 2 * y + 1;
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
        image->width, image->height, NEXRAD_IMAGE_COLOR_DEPTH, NEXRAD_IMAGE_COLOR_FORMAT, image->buf
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
    image->radius = 0;

    free(image);
}
