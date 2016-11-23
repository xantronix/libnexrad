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

#ifndef _NEXRAD_BLOCK_H
#define _NEXRAD_BLOCK_H

#include <stdint.h>

#pragma pack(push)
#pragma pack(1)

#define NEXRAD_BLOCK_DIVIDER -1

#define nexrad_block_after(data, prev) (void *)((char *)data + sizeof(prev))

enum nexrad_block_id {
    NEXRAD_BLOCK_SYMBOLOGY           = 1,
    NEXRAD_BLOCK_GRAPHIC             = 2,
    NEXRAD_BLOCK_TABULAR             = 3,
    NEXRAD_BLOCK_MESSAGE_HEADER      = 4,
    NEXRAD_BLOCK_PRODUCT_DESCRIPTION = 5
};

typedef struct _nexrad_block_header {
     int16_t divider; /* Divider indicating start of block */
    uint16_t id;      /* Block ID */
    uint32_t size;    /* Size of block in entirety, including full header */
} nexrad_block_header;

#pragma pack(pop)

#endif /* _NEXRAD_BLOCK_H */
