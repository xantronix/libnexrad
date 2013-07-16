#ifndef _NEXRAD_DESCRIPTION_H
#define _NEXRAD_DESCRIPTION_H

#include <stdint.h>

#include <nexrad/types.h>
#include <nexrad/consts.h>
#include <nexrad/modes.h>
#include <nexrad/products.h>

typedef struct _nexrad_description {
     int16_t    divider;
     int32_t    site_lat;
     int32_t    site_lon;
    uint16_t    site_altitude;
    uint16_t    product;
    uint16_t    mode;
    uint16_t    vcp;
     int16_t    seq;
    uint16_t    scan;
    nexrad_date scan_date;
    nexrad_date product_date;
    char        _padding[62];
     uint8_t    version;
     uint8_t    blanking;
    uint32_t    symbology_offset;
    uint32_t    graphic_offset;
    uint32_t    tabular_offset;
} nexrad_description;

#endif /* _NEXRAD_DESCRIPTION_H */
