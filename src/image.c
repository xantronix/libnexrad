/*
 * Copyright (c) 2016 Dynamic Weather Solutions, Inc. Distributed under the
 * terms of the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

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
    uint8_t * buf;
    size_t    size;

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
    uint8_t *buf;

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

uint8_t *nexrad_image_get_buf(nexrad_image *image, size_t *size) {
    if (image == NULL) {
        return NULL;
    }

    if (size)
        *size = image->size;

    return image->buf;
}

static inline void _buf_write_pixel(uint8_t *buf, nexrad_color color, uint16_t x, uint16_t y, uint16_t w) {
    size_t offset = NEXRAD_IMAGE_PIXEL_OFFSET(x, y, w);

    memcpy(buf + offset, &color, sizeof(color));
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

void nexrad_image_draw_pixel(nexrad_image *image, nexrad_color color, uint16_t x, uint16_t y) {
    if (image == NULL) {
        return;
    }

    if (x < 0 || x > image->width || y < 0 || y > image->height) {
        return;
    }

    _buf_write_pixel(image->buf, color, x, y, image->width);
}

void nexrad_image_draw_run(nexrad_image *image, nexrad_color color, uint16_t x, uint16_t y, uint16_t length) {
    uint8_t *buf;
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
        memcpy(buf + offset, &color, sizeof(nexrad_color));

        offset += sizeof(nexrad_color);
    }
}

enum octant {
    NONE, NNE, ENE, ESE, SSE, SSW, WSW, WNW, NNW
};

static void _find_arc_octant(int *aminp, int *amaxp, enum octant *octantp) {
    int amin = *aminp, amax = *amaxp;
    enum octant octant = *octantp;

    /*
     * Ensure the angle minimum and maximums fall within a single rotation of
     * a circle.
     */
    _int_order(&amin, &amax);

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
     * Scale down the angle minimum and maximum to a 45 degree range within the
     * current octant.
     */
    amin = amin % 45;

    if (amax && (amax = amax % 45) == 0) {
        amax = 45;
    }

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

    *aminp   = amin;
    *amaxp   = amax;
    *octantp = octant;
}

static inline void _draw_arc_points(nexrad_image *image, nexrad_color color, int x, int y, enum octant octant) {
    uint8_t *buf = image->buf;

    int xc = image->x_center;
    int yc = image->y_center;
    int w  = image->width;

    switch (octant) {
        case ESE: _buf_write_pixel(buf, color,  x+xc,  y+yc, w); break;
        case SSE: _buf_write_pixel(buf, color,  y+xc,  x+yc, w); break;
        case SSW: _buf_write_pixel(buf, color, -y+xc,  x+yc, w); break;
        case WSW: _buf_write_pixel(buf, color, -x+xc,  y+yc, w); break;
        case WNW: _buf_write_pixel(buf, color, -x+xc, -y+yc, w); break;
        case NNW: _buf_write_pixel(buf, color, -y+xc, -x+yc, w); break;
        case NNE: _buf_write_pixel(buf, color,  y+xc, -x+yc, w); break;
        case ENE: _buf_write_pixel(buf, color,  x+xc, -y+yc, w); break;

        default: {
            break;
        }
    }
}

static inline void _draw_arc_complement_points(nexrad_image *image, nexrad_color color, int x, int y, enum octant octant) {
    uint8_t *buf = image->buf;

    int xc = image->x_center;
    int yc = image->y_center;
    int w  = image->width;

    switch (octant) {
        case ESE: _buf_write_pixel(buf, color,  x+xc-1,  y+yc, w); break;
        case SSE: _buf_write_pixel(buf, color,  y+xc+1,  x+yc, w); break;
        case SSW: _buf_write_pixel(buf, color, -y+xc-1,  x+yc, w); break;
        case WSW: _buf_write_pixel(buf, color, -x+xc+1,  y+yc, w); break;
        case WNW: _buf_write_pixel(buf, color, -x+xc+1, -y+yc, w); break;
        case NNW: _buf_write_pixel(buf, color, -y+xc-1, -x+yc, w); break;
        case NNE: _buf_write_pixel(buf, color,  y+xc+1, -x+yc, w); break;
        case ENE: _buf_write_pixel(buf, color,  x+xc-1, -y+yc, w); break;

        default: {
            break;
        }
    }
}

void nexrad_image_draw_arc_segment(nexrad_image *image, nexrad_color color, int amin, int amax, int rmin, int rmax) {
    int x, y, radius, re;
    int xmin, xmax, ymin, ymax;
    static const double rad = M_PI / 180;

    enum octant octant = NONE;

    if (image == NULL || amin > amax || rmin > rmax) {
        return;
    }

    if (rmin >= image->radius || rmax >= image->radius) {
        return;
    }

    _int_order(&rmin, &rmax);
    _find_arc_octant(&amin, &amax, &octant);

    /*
     * Draw nothing if the angle minimum and maximum do not span a single
     * octant.
     */
    if (octant == NONE) {
        return;
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
                _draw_arc_points(image, color, x, y, octant);

                /*
                 * If X will be decremented soon, then we will need to fill in the
                 * next pixel over to prevent a Moire pattern from presenting itself
                 * in any filled arcs.
                 */
                if (decrx) {
                    _draw_arc_complement_points(image, color, x, y, octant);
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
