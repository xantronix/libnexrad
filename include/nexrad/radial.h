#ifndef _NEXRAD_RADIAL_H
#define _NEXRAD_RADIAL_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/color.h>
#include <nexrad/image.h>

#define NEXRAD_RADIAL_RLE_FACTOR 16

enum nexrad_radial_type {
    NEXRAD_RADIAL_RLE     = 0xaf1f,
    NEXRAD_RADIAL_DIGITAL = 16
};

#pragma pack(1)
#pragma pack(push)

typedef struct _nexrad_radial_packet {
    uint16_t type;           /* 16 or 0xaf1f */
    uint16_t rangebin_first; /* Index to first range bin (whatever the hell that means) */
    uint16_t rangebin_count; /* Number of range bins per radial (resolution) */
     int16_t i;              /* I coordinate of center of sweep */
     int16_t j;              /* J coordinate of center of sweep */
    uint16_t scale;          /* Scale factor in units of 0.001 */
    uint16_t rays;           /* Number of rays in product */
} nexrad_radial_packet;

typedef struct _nexrad_radial_ray {
    uint16_t size;        /* Number of halfwords (0xaf1f) or bytes (16) in current ray */
    uint16_t angle_start; /* Scan angle of current ray */
    uint16_t angle_delta; /* Angle delta from previous ray */
} nexrad_radial_ray;

typedef struct _nexrad_radial_run { /* For 0xaf1f */
    unsigned int level  :4; /* Level (color code) of run */
    unsigned int length :4; /* Length of run */
} nexrad_radial_run;

#pragma pack(pop)

typedef struct _nexrad_radial nexrad_radial;

nexrad_radial *nexrad_radial_packet_open(nexrad_radial_packet *packet);

size_t nexrad_radial_bytes_read(nexrad_radial *radial);

void nexrad_radial_reset(nexrad_radial *radial);

void nexrad_radial_close(nexrad_radial *radial);

nexrad_radial_ray *nexrad_radial_read_ray(nexrad_radial *radial,
    void **data,
    size_t *runsp,
    size_t *binsp
);

int nexrad_radial_get_info(nexrad_radial *radial,
    size_t *binsp,
    size_t *raysp
);

nexrad_image *nexrad_radial_create_image(nexrad_radial *radial,
    nexrad_color_table *table
);

#endif /* _NEXRAD_RADIAL_H */
