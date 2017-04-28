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

#ifndef _NEXRAD_COLOR_H
#define _NEXRAD_COLOR_H

#include <stdint.h>

#define NEXRAD_COLOR_TABLE_ENTRIES 256

enum nexrad_color_table {
    NEXRAD_COLOR_TABLE_REFLECTIVITY = 0,
    NEXRAD_COLOR_TABLE_VELOCITY     = 1,
    NEXRAD_COLOR_TABLE_END          = 2
};

typedef struct _nexrad_color {
    uint8_t r, g, b, a;
} nexrad_color;

nexrad_color *nexrad_color_table_load(enum nexrad_color_table table);

#endif /* _NEXRAD_COLOR_H */
