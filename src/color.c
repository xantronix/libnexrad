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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <nexrad/color.h>

static nexrad_color tables[NEXRAD_COLOR_TABLE_END][16] = {
    /*
     * Reflectivity
     */
    {
        { 0x00, 0x00, 0x00, 0x00 }, { 0x00, 0xec, 0xec, 0xff }, { 0x01, 0xa0, 0xf6, 0xff }, { 0x00, 0x00, 0xf6, 0xff },
        { 0x00, 0xff, 0x00, 0xff }, { 0x00, 0xc8, 0x00, 0xff }, { 0x00, 0x90, 0x00, 0xff }, { 0xff, 0xff, 0x00, 0xff },
        { 0xe7, 0xc0, 0x00, 0xff }, { 0xff, 0x90, 0x00, 0xff }, { 0xff, 0x00, 0x00, 0xff }, { 0xd6, 0x00, 0x00, 0xff },
        { 0xc0, 0x00, 0x00, 0xff }, { 0xff, 0x00, 0xff, 0xff }, { 0x99, 0x55, 0xc9, 0xff }, { 0xff, 0xff, 0xff, 0xff }
    },

    /*
     * Velocity
     */
    {
        { 0x00, 0x00, 0x00, 0x00 }, { 0x02, 0xfc, 0x02, 0xff }, { 0x01, 0xe4, 0x01, 0xff }, { 0x01, 0xc5, 0x01, 0xff },
        { 0x07, 0xac, 0x04, 0xff }, { 0x06, 0x8f, 0x03, 0xff }, { 0x04, 0x72, 0x02, 0xff }, { 0x7c, 0x97, 0x7b, 0xff },
        { 0x98, 0x77, 0x77, 0xff }, { 0x89, 0x00, 0x00, 0xff }, { 0xa2, 0x00, 0x00, 0xff }, { 0xb9, 0x00, 0x00, 0xff },
        { 0xd8, 0x00, 0x00, 0xff }, { 0xef, 0x00, 0x00, 0xff }, { 0xfe, 0x00, 0x00, 0xff }, { 0x90, 0x00, 0xa0, 0xff }
    }
};

static void expand_table(nexrad_color *dest, nexrad_color *src) {
    size_t s, d;

    for (s=0, d=0; s<16; s++) {
        size_t i;

        for (i=0; i<16; i++, d++) {
            dest[d] = src[s];
        }
    }
}

nexrad_color *nexrad_color_create_table(enum nexrad_color_table table) {
    nexrad_color *entries;

    if ((entries = malloc(NEXRAD_COLOR_TABLE_ENTRIES * sizeof(nexrad_color))) == NULL) {
        goto error_malloc_entries;
    }

    expand_table(entries, tables[table]);

    return entries;

error_malloc_entries:
    return NULL;
}
