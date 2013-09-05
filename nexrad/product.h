#ifndef _NEXRAD_MESSAGE_PRODUCT_H
#define _NEXRAD_MESSAGE_PRODUCT_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/date.h>

#pragma pack(push)
#pragma pack(1)

#define NEXRAD_VERSION 1

enum nexrad_product_id {
    NEXRAD_PRODUCT_NHI = 59
};

typedef struct _nexrad_product {
    enum nexrad_product_id id;
    char                   name[8];
} nexrad_product;

enum nexrad_radar_mode_id {
    NEXRAD_RADAR_MODE_PRECIP = 2
};

typedef struct _nexrad_product_description {
     int16_t    divider;       /* Start of block */
     int32_t    site_lat;      /* Radar site latitude */
     int32_t    site_lon;      /* Radar site longitude */
    uint16_t    site_altitude; /* Radar site altitude */
    uint16_t    product;       /* NOAA product code */
    uint16_t    mode;          /* Radar operational mode */
    uint16_t    vcp;           /* Radar volume coverage pattern */
     int16_t    seq;           /* Request sequence number */
    uint16_t    scan;          /* Volume scan number */
    nexrad_date scan_date;     /* Start of current scan */
    nexrad_date gen_date;      /* Time of radar product generation */
    char        _padding[54];  /* Product-specific data */
     uint8_t    version;       /* Version */
     uint8_t    blanking;      /* Spot blanking */

    /*
     * An interesting and mildly annoying note regarding these offsets: NOAA
     * seems to think that it is a good idea to use "halfword" (16-bit) size
     * increments to describe these offsets, so to obtain the appropriate
     * offset, multiply the offset by two and add the product to the physical
     * offset of the start of the Product Message Header.
     */
    uint32_t symbology_offset; /* Offset to product symbology block */
    uint32_t graphic_offset;   /* Offset to graphic product message block */
    uint32_t tabular_offset;   /* Offset to tabular alphanumeric block */
} nexrad_product_description;

#pragma pack(pop)

#endif /* _NEXRAD_MESSAGE_PRODUCT_H */
