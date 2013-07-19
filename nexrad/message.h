#ifndef _NEXRAD_MESSAGE_H
#define _NEXRAD_MESSAGE_H

#include <stdint.h>

#include <nexrad/types.h>
#include <nexrad/consts.h>
#include <nexrad/modes.h>
#include <nexrad/products.h>

typedef struct _nexrad_header {
    uint16_t    type;
    nexrad_date date;
    uint32_t    size;
    uint16_t    src_id;
    uint16_t    dest_id;
    uint16_t    blocks;
} nexrad_header;

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

typedef struct _nexrad_symbology {
     int16_t divider;
    uint16_t id;
    uint32_t length;
    uint16_t layers;
     int16_t layer_divider;
} nexrad_symbology;

typedef struct _nexrad_vector {
    uint16_t code;
    uint16_t size;
    uint16_t magnitude; /* in 1/4km increments */
     int16_t i1_start;
     int16_t j1_start;
     int16_t i1_end;
     int16_t j1_end;
     int16_t i2_start;
     int16_t j2_start;
     int16_t i2_end;
     int16_t j2_end;
} nexrad_vector;

typedef struct _nexrad_hda_hail {
    uint16_t code;
    uint16_t size;
     int16_t i;
     int16_t j;
    uint16_t prob_hail;
    uint16_t prob_hail_severe;
    uint16_t max_hail_size;
} nexrad_hda_hail;

typedef struct _nexrad_alpha {
     int16_t divider;
    uint16_t id;
    uint32_t size;
    uint16_t pages;
    uint16_t page_number;
    uint16_t page_size;
} nexrad_alpha;

#define NEXRAD_PACKET_TYPE_ALPHA   8
#define NEXRAD_PACKET_TYPE_VECTOR 10
#define NEXRAD_PACKET_TYPE_HAIL   19

typedef struct _nexrad_packet_header {
    uint16_t code;
    uint16_t size;
} nexrad_packet_header;

#endif /* _NEXRAD_MESSAGE_H */
