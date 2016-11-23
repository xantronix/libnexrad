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

#ifndef _NEXRAD_CHUNK_H
#define _NEXRAD_CHUNK_H

#include <stdint.h>
#include <sys/types.h>

enum nexrad_chunk_type {
    NEXRAD_CHUNK_UNKNOWN,
    NEXRAD_CHUNK_SYMBOLOGY_BLOCK,
    NEXRAD_CHUNK_GRAPHIC_BLOCK,
    NEXRAD_CHUNK_TABULAR_BLOCK,
    NEXRAD_CHUNK_SYMBOLOGY_LAYER,
    NEXRAD_CHUNK_SYMBOLOGY_PACKET,
    NEXRAD_CHUNK_GRAPHIC_PAGE,
    NEXRAD_CHUNK_GRAPHIC_PACKET
};

typedef struct _nexrad_chunk nexrad_chunk;

/*
 * Generic interface for reading radar product data chunks
 */
nexrad_chunk *nexrad_chunk_open(void *chunk, enum nexrad_chunk_type type);

void *nexrad_chunk_peek(nexrad_chunk *iterator,
    size_t *size,
    size_t *payload,
    void **data
);

void nexrad_chunk_next(nexrad_chunk *iterator, size_t size);

void *nexrad_chunk_read(nexrad_chunk *iterator,
    size_t *size,
    size_t *payload,
    void **data
);

nexrad_chunk *nexrad_chunk_read_block_layer(nexrad_chunk *block,
    enum nexrad_chunk_type type
);

void nexrad_chunk_close(nexrad_chunk *iterator);

#endif /* _NEXRAD_CHUNK_H */
