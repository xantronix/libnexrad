#ifndef _NEXRAD_POLY_H
#define _NEXRAD_POLY_H

#include <nexrad/radial.h>

#define NEXRAD_POLY_BYTE_ORDER_LE 1
#define NEXRAD_POLY_TYPE          3
#define NEXRAD_POLY_MULTI_TYPE    6
#define NEXRAD_POLY_RINGS         1
#define NEXRAD_POLY_RING_POINTS   4

#pragma pack(1)
#pragma pack(push)

typedef struct _nexrad_poly_point {
    double lat;
    double lon;
} nexrad_poly_point;

typedef struct _nexrad_poly_ring {
    uint32_t points;
} nexrad_poly_ring;

typedef struct _nexrad_poly {
    uint8_t  byte_order;
    uint32_t type;
    uint32_t rings;
} nexrad_poly;

typedef struct _nexrad_poly_multi {
    uint8_t  byte_order;
    uint32_t type;
    uint32_t polys;
} nexrad_poly_multi;

#pragma pack(pop)

int nexrad_poly_multi_size_for_radial(nexrad_radial *radial,
    size_t * sizep,
    int *    rangebinsp
);

int nexrad_poly_multi_write_from_radial(
    nexrad_radial *        radial,
    int                    rangebins,
    nexrad_poly_multi *    multi,
    size_t                 size,
    nexrad_geo_cartesian * radar,
    nexrad_geo_spheroid *  spheroid
);

nexrad_poly_multi *nexrad_poly_multi_create_from_radial(
    nexrad_radial *        radial,
    size_t *               sizep,
    nexrad_geo_cartesian * radar,
    nexrad_geo_spheroid *  spheroid
);

void nexrad_poly_multi_destroy(nexrad_poly_multi *multi);

#endif /* _NEXRAD_POLY_H */
