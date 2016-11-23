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

#ifndef _NEXRAD_TABULAR_H
#define _NEXRAD_TABULAR_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/header.h>
#include <nexrad/block.h>
#include <nexrad/product.h>

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_tabular_block {
    nexrad_block_header header;

    /*
     * For some inexplicable reason, the tabular alphanumeric block duplicates
     * the NEXRAD product message header and product description blocks.  Of
     * particular forensic interest is the fact that this second product
     * description block refers only to an offset to the previous product
     * symbology block, but provides null values for the offsets to the graphic
     * product message block and tabular alphanumeric block.  It has not yet
     * been determined what the significance of this construct is.
     */
    nexrad_message_header      message_header;
    nexrad_product_description product_description;

     int16_t divider; /* Standard block divider */
    uint16_t pages;   /* Number of pages to follow */
} nexrad_tabular_block;

#pragma pack(pop)

#define NEXRAD_TABULAR_BLOCK_MAX_LINE_SIZE 80

typedef struct _nexrad_tabular_text {
    char * current;    /* Current pointer */
    int    page;       /* Current page number */
    int    line;       /* Line number in current page */
    int    pages_left; /* Number of pages left in text */
    size_t bytes_left; /* Number of bytes left in text */
} nexrad_tabular_text;

nexrad_tabular_text *nexrad_tabular_block_open(nexrad_tabular_block *block);

ssize_t nexrad_tabular_block_read_line(nexrad_tabular_text *text,
    char **data,
    int *page,
    int *line
);

void nexrad_tabular_block_close(nexrad_tabular_text *block);

#endif /* _NEXRAD_TABULAR_H */
