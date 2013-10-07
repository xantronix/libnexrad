#ifndef _UTIL_H
#define _UTIL_H

#include <sys/types.h>
#include "config.h"

#define bswap16(v) (((v & 0xff00) >> 16) | (v & 0x00ff) << 16)

#define bswap32(v) \
    (((v & 0xff000000) >> 24) | \
     ((v & 0x00ff0000) >>  8) | \
     ((v & 0x0000ff00) <<  8) | \
     ((v & 0x000000ff) << 24))

#ifndef _ENDIAN_H
#ifdef __DO_SWAP_BYTES
#define be16toh(v) bswap16(v)
#define be32toh(v) bswap32(v)
#define htobe16(v) bswap16(v)
#define htobe32(v) bswap32(v)
#endif /* __DO_SWAP_BYTES */
#endif /* !_ENDIAN_H */

int safecpy(char *dest, char *src, size_t destlen, size_t srclen);

#endif /* _UTIL_H */
