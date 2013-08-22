#ifndef _NEXRAD_RADIAL_H
#define _NEXRAD_RADIAL_H

#include <stdint.h>
#include <sys/types.h>

#define NEXRAD_RADIAL_PACKET 0xaf1f

#pragma pack(1)
#pragma pack(push)

typedef struct _nexrad_radial_packet {
    uint16_t type;           /* Always 0xaf1f */
    uint16_t rangebin_first; /* Index to first range bin */
    uint16_t rangebin_count; /* Number of range bins */
     int16_t i;              /* I coordinate of center of sweep */
     int16_t j;              /* J coordinate of center of sweep */
    uint16_t scale;          /* Scale factor in units of 0.001 */
    uint16_t rays;           /* Number of rays in product */
} nexrad_radial_packet;

typedef struct _nexrad_radial_ray {
    uint16_t runs;        /* Number of RLE-encoded runs in current ray */
    uint16_t angle_start; /* Scan angle of current ray */
    uint16_t angle_delta; /* Angle delta from previous ray */
} nexrad_radial_ray;

typedef struct _nexrad_radial_run {
    unsigned int length :4; /* Length of run */
    unsigned int level  :4; /* Level (color code) of run */
} nexrad_radial_run;

#pragma pack(pop)

typedef struct _nexrad_radial {
    nexrad_radial_packet * packet;
    size_t                 rays_left;
    nexrad_radial_ray *    current;
} nexrad_radial;

nexrad_radial *     nexrad_radial_packet_open(nexrad_radial_packet *packet);
nexrad_radial_ray * nexrad_radial_read_ray(nexrad_radial *radial, nexrad_radial_run **runs);
void                nexrad_radial_close(nexrad_radial *radial);

#endif /* _NEXRAD_RADIAL_H */
