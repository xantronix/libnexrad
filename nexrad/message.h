#ifndef _NEXRAD_MESSAGE_H
#define _NEXRAD_MESSAGE_H

#include <stdint.h>

#include <nexrad/types.h>
#include <nexrad/consts.h>
#include <nexrad/modes.h>
#include <nexrad/products.h>

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_header {
    uint16_t    type;    /* Product type */
    nexrad_date date;    /* Date of message transmission */
    uint32_t    size;    /* Size of message, including header */
    uint16_t    src_id;  /* Message source */
    uint16_t    dest_id; /* Message destination */
    uint16_t    blocks;  /* Number of blocks in message (5 or less) */
} nexrad_header;

typedef struct _nexrad_description {
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
    nexrad_date product_date;  /* Time of radar product generation */
    char        _padding[62];  /* Product-specific data */
     uint8_t    version;       /* Version */
     uint8_t    blanking;      /* Spot blanking */

    /*
     * An interesting and mildly annoying note regarding these offsets: NOAA
     * seems to think that it is a good idea to use "halfword" (16-bit) size
     * increments to describe these offsets, so to obtain the appropriate
     * offset, multiply the offset by two and add the product to the physical
     * offset of the start of the Product Message Header.
     */
    uint32_t    symbology_offset; /* Offset to product symbology block */
    uint32_t    graphic_offset;   /* Offset to graphic product message block */
    uint32_t    tabular_offset;   /* Offset to alphanumeric table data block */
} nexrad_description;

#define NEXRAD_SYMBOLOGY_LAYER_DIVIDER -1

typedef struct _nexrad_symbology {
     int16_t divider; /* Divider indicating start of symbology block */
    uint16_t id;      /* Block ID (always 1) */
    uint32_t length;  /* Length of block in entirety (inclusive) */
    uint16_t layers;  /* Number of layers following */
} nexrad_symbology;

#define NEXRAD_PACKET_TYPE_ALPHA   8
#define NEXRAD_PACKET_TYPE_VECTOR 10
#define NEXRAD_PACKET_TYPE_HAIL   19

typedef struct _nexrad_packet_header {
    uint16_t code;
    uint16_t size;
} nexrad_packet_header;

typedef struct _nexrad_packet_hail {
     int16_t i;                /* Cartesian offset from radar */
     int16_t j;                /* Cartesian offset from radar */
    uint16_t prob_hail;        /* Probability of hail */
    uint16_t prob_hail_severe; /* Probability of severe hail */
    uint16_t max_hail_size;    /* Maximum size of hail */
} nexrad_packet_hail;

typedef struct _nexrad_vector {
    uint16_t code;      /* Packet code */
    uint16_t size;
    uint16_t magnitude; /* Vector magnitude in 1/4km increments */
     int16_t i1_start;  /* Cartesian origin vector */
     int16_t j1_start;  /* Cartesian origin vector */
     int16_t i1_end;    /* Cartesian origin vector */
     int16_t j1_end;    /* Cartesian origin vector */
     int16_t i2_start;  /* Cartesian destination vector */
     int16_t j2_start;  /* Cartesian destination vector */
     int16_t i2_end;    /* Cartesian destination vector */
     int16_t j2_end;    /* Cartesian destination vector */
} nexrad_vector;

typedef struct _nexrad_tabular {
     int16_t divider;   /* Start of tabular alphanumeric block */
    uint16_t id;        /* Block ID value 3 */
    uint32_t size;      /* Size of block, inclusive */
    uint16_t pages;     /* Number of pages to follow */
    uint16_t line_size; /* Number of characters per line */
    uint16_t page_size;
} nexrad_tabular;

#pragma pack(pop)

#endif /* _NEXRAD_MESSAGE_H */
