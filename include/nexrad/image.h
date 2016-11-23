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

#ifndef _NEXRAD_IMAGE_H
#define _NEXRAD_IMAGE_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/color.h>

/*!
 * \file nexrad/image.h
 * \brief Image buffer management functionality
 *
 * The code responsible for creating, handling and saving image buffers.
 */

typedef struct _nexrad_image nexrad_image;

/*!
 * \defgroup image Image buffer manipulation routines
 */

/*!
 * \ingroup image
 * \brief Create a new image buffer
 * \param width Width, in pixels
 * \param height Height, in pixels
 * \return A newly-allocated image buffer
 *
 * Create a new image buffer in RGBA 8-bit channel format, initialized to fully
 * transparent black values.
 */
nexrad_image *nexrad_image_create(
    uint16_t width,
    uint16_t height
);

/*!
 * \ingroup image
 * \brief Determine dimensions of an image buffer
 * \param image An image buffer object
 * \param width Pointer to a uint16_t to write image width to
 * \param height Pointer to a uint16_t to write image height to
 * \return 0 on success, -1 on failure
 *
 * Determine the dimensions of an existing image buffer.
 */
int nexrad_image_get_info(nexrad_image *image,
    uint16_t *width,
    uint16_t *height
);

/*!
 * \ingroup image
 * \brief Obtain pointer to raw image data inside buffer object
 * \param image An image buffer object
 * \param sizep Pointer to a size_t to write size of image to, in bytes
 * \return Pointer to raw image data, or NULL on failure
 *
 * Obtain a pointer to raw image data inside the specified buffer object, while
 * also obtaining the size, in bytes, of the image buffer in the `size` output
 * parameter.
 */
uint8_t *nexrad_image_get_buf(nexrad_image *image, size_t *sizep);

/*!
 * \ingroup image
 * \brief Save image buffer to disk file as PNG
 * \param image An image buffer object
 * \param path Output file path
 * \return 0 on success, -1 on failure
 *
 * Save the image buffer to a PNG file on disk.
 */
int nexrad_image_save_png(nexrad_image *image, const char *path);

/*!
 * \ingroup image
 * \brief Destroy and deallocate image buffer object
 * \param image An image buffer object
 *
 * Destroy any state associated with the specified image buffer object, and
 * free() the raw data buffer therein, as well as the object data itself.
 */
void nexrad_image_destroy(nexrad_image *image);

/*!
 * \defgroup drawing Image buffer drawing routines
 */

/*!
 * \ingroup drawing
 * \brief Draw a single pixel in buffer
 * \param image An image buffer object
 * \param color A color table entry
 * \param x X coordinate to draw pixel to
 * \param y Y coordinate to draw pixel to
 *
 * Draw a single pixel of the specified color to the specified Cartesian
 * location in the image.
 */
void nexrad_image_draw_pixel(nexrad_image *image,
    nexrad_color color,
    uint16_t x, uint16_t y
);

/*!
 * \ingroup drawing
 * \brief Draw a line (run) of pixels of a specified color
 * \param image An image buffer object
 * \param color A color table entry
 * \param x X coordinate of start of run
 * \param y Y coordinate of start of run
 * \param length Length of run, in pixels
 *
 * Draw a line (run) of pixels of a specified color.  Most useful when rendering
 * rasterized radar data provided in Run-Length Encoded data packets.
 */
void nexrad_image_draw_run(nexrad_image *image,
    nexrad_color color,
    uint16_t x, uint16_t y,
    uint16_t length
);

/*!
 * \ingroup drawing
 * \brief Draw an arc segment relative to center of image
 * \param image An image buffer object
 * \param color A color table entry
 * \param amin Minimum azimuth to draw arc segment for
 * \param amax Maximum azimuth to draw arc segment for
 * \param rmin Minimum radius (range) to draw arc segment for
 * \param rmax Maximum radius (range) to draw arc segment for
 *
 * Draw an arc segment relative to the center of the image.
 */
void nexrad_image_draw_arc_segment(nexrad_image *image,
    nexrad_color color,
    int amin, int amax,
    int rmin, int rmax
);

#endif /* _NEXRAD_IMAGE_H */
