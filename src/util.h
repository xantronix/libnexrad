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

#ifndef _UTIL_H
#define _UTIL_H

#include <sys/types.h>
#include "config.h"

#define bswap16(v) (((v & 0xff00) >> 8) | (v & 0x00ff) << 8)

#define bswap32(v) \
    (((v & 0xff000000) >> 24) | \
     ((v & 0x00ff0000) >>  8) | \
     ((v & 0x0000ff00) <<  8) | \
     ((v & 0x000000ff) << 24))

#define bswap64(v) \
    (((v & 0xff00000000000000) >> 56) | \
     ((v & 0x00ff000000000000) >> 40) | \
     ((v & 0x0000ff0000000000) >> 16) | \
     ((v & 0x000000ff00000000) >>  8) | \
     ((v & 0x00000000ff000000) <<  8) | \
     ((v & 0x0000000000ff0000) << 16) | \
     ((v & 0x000000000000ff00) << 40) | \
     ((v & 0x00000000000000ff) << 56))

#ifndef _ENDIAN_H
#ifdef __DO_SWAP_BYTES
#define be16toh(v) ((uint16_t)bswap16((uint16_t)v))
#define be32toh(v) ((uint32_t)bswap32((uint32_t)v))
#define htobe16(v) ((uint16_t)bswap16((uint16_t)v))
#define htobe32(v) ((uint32_t)bswap32((uint32_t)v))
#define htole32(v) ((uint32_t)v)
#define htole64(v) ((uint64_t)v)
#else
#define be16toh(v) ((uint16_t)v)
#define be32toh(v) ((uint32_t)v)
#define htobe16(v) ((uint16_t)v)
#define htobe32(v) ((uint32_t)v)
#define htole32(v) ((uint32_t)bswap32((uint32_t)v))
#define htole64(v) ((uint64_t)bswap64((uint64_t)v))
#endif /* __DO_SWAP_BYTES */
#endif /* !_ENDIAN_H */

int safecpy(char *dest, char *src, size_t destlen, size_t srclen);

#endif /* _UTIL_H */
