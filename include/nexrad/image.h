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

#define NEXRAD_IMAGE_PIXEL_BYTES  4
#define NEXRAD_IMAGE_COLOR_DEPTH  8
#define NEXRAD_IMAGE_COLOR_FORMAT PNG_TRUECOLOR_ALPHA

#define NEXRAD_IMAGE_PIXEL_OFFSET(x, y, w) \
  ((y * w * NEXRAD_IMAGE_PIXEL_BYTES) + x * NEXRAD_IMAGE_PIXEL_BYTES)

typedef struct _nexrad_image {
    size_t width,
           height;
} nexrad_image;

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
nexrad_image *nexrad_image_create(size_t width, size_t height);

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

#endif /* _NEXRAD_IMAGE_H */
