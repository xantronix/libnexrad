#ifndef _NEXRAD_ALPHA_H
#define _NEXRAD_ALPHA_H

#include <stdint.h>

#include <nexrad/consts.h>

typedef struct _nexrad_alpha {
     int16_t divider;
    uint16_t id;
    uint32_t size;
    uint16_t pages;
    uint16_t page_number;
    uint16_t page_size;
} nexrad_alpha;

#endif /* _NEXRAD_ALPHA_H */
