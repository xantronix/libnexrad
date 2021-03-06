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

#include <nexrad/graphic.h>

nexrad_chunk *nexrad_graphic_block_open(nexrad_graphic_block *block) {
    return nexrad_chunk_open(block, NEXRAD_CHUNK_GRAPHIC_BLOCK);
}

nexrad_chunk *nexrad_graphic_block_read_page(nexrad_chunk *block) {
    return nexrad_chunk_read_block_layer(block, NEXRAD_CHUNK_GRAPHIC_PAGE);
}

void nexrad_graphic_page_next_packet(nexrad_chunk *page, size_t size) {
    nexrad_chunk_next(page, size);
}

nexrad_packet *nexrad_graphic_page_peek_packet(nexrad_chunk *page, size_t *size) {
    return nexrad_chunk_peek(page, size, NULL, NULL);
}

nexrad_packet *nexrad_graphic_page_read_packet(nexrad_chunk *page, size_t *size) {
    return nexrad_chunk_read(page, size, NULL, NULL);
}

void nexrad_graphic_page_close(nexrad_chunk *page) {
    nexrad_chunk_close(page);
}

void nexrad_graphic_block_close(nexrad_chunk *block) {
    nexrad_chunk_close(block);
}

