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

#ifndef _NEXRAD_HEADER_H
#define _NEXRAD_HEADER_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/date.h>

#pragma pack(push)
#pragma pack(1)

#define NEXRAD_HEADER_UNKNOWN_SIGNATURE "\x01\x0d\x0d\x0a"
#define NEXRAD_HEADER_WMO_SIGNATURE     "SDUS"

typedef struct _nexrad_unknown_header {
    char reserved1[4];
    char reserved2[4];
    char reserved3[3];
} nexrad_unknown_header;

typedef struct _nexrad_wmo_header {
    char region[6];
    char _whitespace1;
    char office[4];
    char _whitespace2;
    char timestamp[6];
    char _whitespace3[3];
    char product_code[3];
    char station[3];
    char _whitespace4[3];
} nexrad_wmo_header;

typedef struct _nexrad_message_header {
     int16_t    product_type; /* Product type ID */
    nexrad_date date;         /* Date of message transmission */
    uint32_t    size;         /* Size of message, including header */
    uint16_t    src_id;       /* Message source */
    uint16_t    dest_id;      /* Message destination */
    uint16_t    blocks;       /* Number of blocks in message (5 or less) */
} nexrad_message_header;

#pragma pack(pop)

#endif /* _NEXRAD_HEADER_H */
