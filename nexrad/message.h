#ifndef _NEXRAD_MESSAGE_H
#define _NEXRAD_MESSAGE_H

#include <stdint.h>

#include <nexrad/types.h>
#include <nexrad/products.h>
#include <nexrad/radar.h>

#pragma pack(push)
#pragma pack(1)

#define NEXRAD_BLOCK_DIVIDER -1
#define NEXRAD_VERSION        1

#define NEXRAD_PRODUCT_NHI 59 /* NOAA Hail Index */

typedef struct _nexrad_message_header {
     int16_t    type;    /* Product type */
    nexrad_date date;    /* Date of message transmission */
    uint32_t    size;    /* Size of message, including header */
    uint16_t    src_id;  /* Message source */
    uint16_t    dest_id; /* Message destination */
    uint16_t    blocks;  /* Number of blocks in message (5 or less) */
} nexrad_message_header;

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
    uint32_t symbology_offset; /* Offset to product symbology block */
    uint32_t graphic_offset;   /* Offset to graphic product message block */
    uint32_t tabular_offset;   /* Offset to tabular alphanumeric block */
} nexrad_product_description;

typedef struct _nexrad_block_header {
     int16_t divider; /* Divider indicating start of block */
    uint16_t id;      /* Block ID */
    uint32_t size;    /* Size of block in entirety, including full header */
} nexrad_block_header;

#define NEXRAD_SYMBOLOGY_BLOCK_ID 1

typedef struct _nexrad_symbology_block {
    nexrad_block_header header;

    uint16_t layers;  /* Number of layers following */
} nexrad_symbology_block;

#define NEXRAD_PACKET_TYPE_TEXT    8
#define NEXRAD_PACKET_TYPE_VECTOR 10
#define NEXRAD_PACKET_TYPE_HAIL   19

typedef struct _nexrad_packet_header {
    uint16_t code;
    uint16_t size;
} nexrad_packet_header;

typedef struct _nexrad_text_packet 
    nexrad_packet_header header;

    uint16_t color; /* 4-bit color value (0-15) */
     int16_t i;     /* Cartesian offset from radar in 1/4km increments */
     int16_t j;     /* Cartesian offset from radar in 1/4km increments */
} nexrad_text_packet;

typedef struct _nexrad_hail_packet {
    nexrad_packet_header header;

     int16_t i;                /* Cartesian offset from radar */
     int16_t j;                /* Cartesian offset from radar */
    uint16_t prob_hail;        /* Probability of hail */
    uint16_t prob_hail_severe; /* Probability of severe hail */
    uint16_t max_hail_size;    /* Maximum size of hail */
} nexrad_hail_packet;

typedef struct _nexrad_vector_packet {
    nexrad_packet_header header;

    uint16_t magnitude; /* Vector magnitude in 1/4km increments */
     int16_t i1_start;  /* Cartesian origin vector */
     int16_t j1_start;  /* Cartesian origin vector */
     int16_t i1_end;    /* Cartesian origin vector */
     int16_t j1_end;    /* Cartesian origin vector */
     int16_t i2_start;  /* Cartesian destination vector */
     int16_t j2_start;  /* Cartesian destination vector */
     int16_t i2_end;    /* Cartesian destination vector */
     int16_t j2_end;    /* Cartesian destination vector */
} nexrad_vector_packet;

#define NEXRAD_GRAPHIC_BLOCK 2

typedef struct _nexrad_graphic_block {
    nexrad_block_header header;

     int16_t divider; /* Block divider (-1) */
    uint16_t id;      /* Block ID (2) */
    uint32_t size;    /* Size of block (inclusive) */
    uint16_t pages;   /* Number of alphanumeric pages */
} nexrad_graphic_block;

typedef struct _nexrad_graphic_page {
    uint16_t page;
    uint16_t size;
} nexrad_graphic_page;

#define NEXRAD_TABULAR_BLOCK_ID 3

typedef struct _nexrad_tabular_block {
    nexrad_block_header header;

    /*
     * For some inexplicable reason, the tabular alphanumeric block duplicates
     * the NEXRAD product message header and product description blocks.  Of
     * particular forensic interest is the fact that this second product
     * description block refers only to an offset to the previous product
     * symbology block, but provides null values for the offsets to the graphic
     * product message block and tabular alphanumeric block.  It has not yet
     * been determined what the significance of this construct is.
     */
    nexrad_product_header      product_header;
    nexrad_product_description product_description;

     int16_t divider;   /* Standard block divider */
    uint16_t pages;     /* Number of pages to follow */
    uint16_t line_size; /* Number of characters per line */
} nexrad_tabular_block;

#pragma pack(pop)

#endif /* _NEXRAD_MESSAGE_H */
