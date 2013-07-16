#ifndef _NEXRAD_HEADER_H
#define _NEXRAD_HEADER_H

#include <stdint.h>

#include <nexrad/types.h>

typedef struct _nexrad_header {
    uint16_t    product;
    nexrad_date date;
    uint32_t    size;
    uint16_t    source_id;
    uint16_t    dest_id;
    uint16_t    blocks;
} nexrad_header;

#endif /* _NEXRAD_HEADER_H */
