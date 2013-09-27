#ifndef _NEXRAD_RASTER_H
#define _NEXRAD_RASTER_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/image.h>

#define NEXRAD_RASTER_RLE_FACTOR 16

#pragma pack(1)
#pragma pack(push)

typedef struct _nexrad_raster_packet {
    uint16_t type;         /* 0xba0f or 0xba07 */
    uint16_t flags_1;      /* 0x8000, so I'm told */
    uint16_t flags_2;      /* 0x00c0, so they say */
     int16_t i;            /* Start of raster data (upper left corner) */
     int16_t j;            /* Start of raster data (upper left corner) */
    uint16_t x_scale;      /* X axis scaling factor */
    uint16_t x_fractional; /* Reserved */
    uint16_t y_scale;      /* Y axis scaling factor */
    uint16_t y_fractional; /* Reserved */
    uint16_t lines;        /* Number of raster lines */
    uint16_t packing;      /* Packing format (seemingly always 2) */
} nexrad_raster_packet;

typedef struct _nexrad_raster_line {
    uint16_t runs; /* Number of RLE-encoded runs in raster line */
} nexrad_raster_line;

typedef struct _nexrad_raster_run {
    unsigned int level  :4; /* Level (color code) of run */
    unsigned int length :4; /* Length of run */
} nexrad_raster_run;

#pragma pack(pop)

typedef struct _nexrad_raster nexrad_raster;

nexrad_raster *nexrad_raster_packet_open(nexrad_raster_packet *packet);

size_t nexrad_raster_bytes_read(nexrad_raster *raster);

void nexrad_raster_close(nexrad_raster *raster);

nexrad_raster_line *nexrad_raster_read_line(nexrad_raster *raster,
    void **data,
    size_t *runsp
);

int nexrad_raster_get_info(nexrad_raster *raster,
    size_t *widthp,
    size_t *heightp
);

nexrad_image *nexrad_raster_create_image(nexrad_raster *raster);

#endif /* _NEXRAD_RASTER_H */
