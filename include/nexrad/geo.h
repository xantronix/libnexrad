#ifndef _NEXRAD_GEO_H
#define _NEXRAD_GEO_H

#define NEXRAD_GEO_SPHEROID_CIRCUMFERENCE 6378137.0
#define NEXRAD_GEO_SPHEROID_FLATTENING    (1/298.257223563)

#define NEXRAD_GEO_NM_METERS       1852
#define NEXRAD_GEO_COORD_MAGNITUDE    0.001
#define NEXRAD_GEO_AZIMUTH_FACTOR     0.1

#define NEXRAD_GEO_RADIAL_MAP_MAGIC   "PROJ"
#define NEXRAD_GEO_RADIAL_MAP_VERSION 0x01

#include <stdint.h>

enum nexrad_geo_radial_map_type {
    NEXRAD_GEO_RADIAL_MAP_NONE,
    NEXRAD_GEO_RADIAL_MAP_SPHEROID,
    NEXRAD_GEO_RADIAL_MAP_EQUIRECT,
    NEXRAD_GEO_RADIAL_MAP_MERCATOR
};

typedef struct _nexrad_geo_polar {
    double azimuth;
    double range;
} nexrad_geo_polar;

typedef struct _nexrad_geo_cartesian {
    double lat;
    double lon;
} nexrad_geo_cartesian;

typedef struct _nexrad_geo_radial_map_header {
    char     magic[4];
    uint16_t version;
    uint16_t type;
    uint16_t width;
    uint16_t height;
    uint16_t rangebins;
    uint16_t rangebin_meters;

    /*
     * Type-dependent values:
     *
     * NEXRAD_GEO_PROJECTION_EQUIRECT: Scale of pixels in units of 0.001 degrees
     */
    uint32_t misc; /* Type-dependent value */

    /*
     * Information pertaining to radar at center of map
     */
     int32_t station_lat;
     int32_t station_lon;
} nexrad_geo_radial_map_header;

typedef struct _nexrad_geo_radial_map_point {
    uint16_t azimuth;
    uint16_t range;
} nexrad_geo_radial_map_point;

typedef struct _nexrad_geo_spheroid   nexrad_geo_spheroid;
typedef struct _nexrad_geo_radial_map nexrad_geo_radial_map;

nexrad_geo_spheroid *nexrad_geo_spheroid_create();

double nexrad_geo_spheroid_get_circumference(nexrad_geo_spheroid *spheroid);

double nexrad_geo_spheroid_get_flattening(nexrad_geo_spheroid *spheroid);

void nexrad_geo_find_polar_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * cart,
    nexrad_geo_polar *     dest
);

void nexrad_geo_find_cartesian_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * dest,
    nexrad_geo_polar *     polar
);

void nexrad_geo_spheroid_destroy(nexrad_geo_spheroid *spheroid);

void nexrad_geo_radial_map_find_extents(
    nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian *radar,
    uint16_t rangebins,
    uint16_t rangebin_meters,
    nexrad_geo_cartesian *extents
);

void nexrad_geo_radial_map_get_dimensions(
    nexrad_geo_cartesian *extents,
    double scale,
    uint16_t *width,
    uint16_t *height
);

nexrad_geo_radial_map *nexrad_geo_radial_map_create(
    const char *path,
    nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian *radar,
    uint16_t rangebins,
    uint16_t rangebin_meters,
    double scale
);

nexrad_geo_radial_map *nexrad_geo_radial_map_open(const char *path);

enum nexrad_geo_radial_map_type nexrad_geo_radial_map_get_type(
    nexrad_geo_radial_map *map
);

int nexrad_geo_radial_map_read_dimensions(
    nexrad_geo_radial_map *map,
    uint16_t *width,
    uint16_t *height
);

int nexrad_geo_radial_map_read_range(
    nexrad_geo_radial_map *map,
    uint16_t *rangebins,
    uint16_t *rangebin_meters
);

int nexrad_geo_radial_map_read_station_location(
    nexrad_geo_radial_map *map,
    nexrad_geo_cartesian *radar
);

int nexrad_geo_radial_map_find_polar_point(nexrad_geo_radial_map *map,
    uint16_t x,
    uint16_t y,
    nexrad_geo_polar *polar
);

int nexrad_geo_radial_map_save(nexrad_geo_radial_map *map);

void nexrad_geo_radial_map_close(nexrad_geo_radial_map *map);

#endif /* _NEXRAD_GEO_POINT_H */
