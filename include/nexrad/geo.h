#ifndef _NEXRAD_GEO_H
#define _NEXRAD_GEO_H

#define NEXRAD_GEO_SPHEROID_CIRCUMFERENCE 6378137.0
#define NEXRAD_GEO_SPHEROID_FLATTENING    (1/298.257223563)

#define NEXRAD_GEO_NM_METERS       1852
#define NEXRAD_GEO_COORD_MAGNITUDE    0.001
#define NEXRAD_GEO_AZIMUTH_FACTOR     0.1

#define NEXRAD_GEO_PROJECTION_MAGIC   "PROJ"
#define NEXRAD_GEO_PROJECTION_VERSION 0x01

#define NEXRAD_GEO_MERCATOR_MAX_LAT    85.05112878
#define NEXRAD_GEO_MERCATOR_TILE_SIZE 256
#define NEXRAD_GEO_MERCATOR_MIN_ZOOM    4
#define NEXRAD_GEO_MERCATOR_MAX_ZOOM   10

#include <stdint.h>

enum nexrad_geo_projection_type {
    NEXRAD_GEO_PROJECTION_NONE,
    NEXRAD_GEO_PROJECTION_SPHEROID,
    NEXRAD_GEO_PROJECTION_EQUIRECT,
    NEXRAD_GEO_PROJECTION_MERCATOR
};

typedef struct _nexrad_geo_polar {
    double azimuth;
    double range;
} nexrad_geo_polar;

typedef struct _nexrad_geo_cartesian {
    double lat;
    double lon;
} nexrad_geo_cartesian;

#pragma pack(1)
#pragma pack(push)

typedef struct _nexrad_geo_projection_header {
    char     magic[4];
    uint16_t version;
    uint16_t type;
    uint16_t width;
    uint16_t height;
    uint32_t world_width;    /* Width of world in projected pixels */
    uint32_t world_height;   /* Height of world in projected pixels */
    uint32_t world_offset_x; /* Offset of projection in world */
    uint32_t world_offset_y; /* Offset of projection in world */
    uint16_t rangebins;
    uint16_t rangebin_meters;
    uint16_t angle; /* Scan elevation angle */

    struct {
        int32_t lat;
        int32_t lon;
    } extents[4];

    union {
        char unknown[32];

        struct {
            uint32_t scale;
            char unused[28];
        } equirect;

        struct {
            uint32_t scale;
            uint16_t zoom;
            char unused[26];
        } mercator;
    } opts;

    /*
     * Information pertaining to radar at center of projection
     */
     int32_t station_lat;
     int32_t station_lon;
} nexrad_geo_projection_header;

typedef struct _nexrad_geo_projection_point {
    uint16_t azimuth;
    uint16_t range;
} nexrad_geo_projection_point;

#pragma pack(pop)

typedef struct _nexrad_geo_spheroid   nexrad_geo_spheroid;
typedef struct _nexrad_geo_projection nexrad_geo_projection;

nexrad_geo_spheroid *nexrad_geo_spheroid_create();

double nexrad_geo_spheroid_get_circumference(nexrad_geo_spheroid *spheroid);

double nexrad_geo_spheroid_get_flattening(nexrad_geo_spheroid *spheroid);

void nexrad_geo_find_polar_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * dest,
    nexrad_geo_polar *     output
);

void nexrad_geo_find_cartesian_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * output,
    nexrad_geo_polar *     dest
);

void nexrad_geo_spheroid_destroy(nexrad_geo_spheroid *spheroid);

void nexrad_geo_projection_find_extents(
    nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian *radar,
    uint16_t rangebins,
    uint16_t rangebin_meters,
    nexrad_geo_cartesian *extents
);

nexrad_geo_projection *nexrad_geo_projection_create_equirect(
    const char *path,
    nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian *radar,
    uint16_t rangebins,
    uint16_t rangebin_meters,
    double scale
);

nexrad_geo_projection *nexrad_geo_projection_create_mercator(
    const char *path,
    nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian *radar,
    uint16_t rangebins,
    uint16_t rangebin_meters,
    int zoom
);

nexrad_geo_projection *nexrad_geo_projection_open(const char *path);

enum nexrad_geo_projection_type nexrad_geo_projection_get_type(
    nexrad_geo_projection *proj
);

int nexrad_geo_projection_read_dimensions(
    nexrad_geo_projection *proj,
    uint16_t *width,
    uint16_t *height
);

int nexrad_geo_projection_read_range(
    nexrad_geo_projection *proj,
    uint16_t *rangebins,
    uint16_t *rangebin_meters
);

int nexrad_geo_projection_read_station_location(
    nexrad_geo_projection *proj,
    nexrad_geo_cartesian *radar
);

int nexrad_geo_projection_read_extents(
    nexrad_geo_projection *proj,
    nexrad_geo_cartesian *extents
);

int nexrad_geo_projection_find_cartesian_point(nexrad_geo_projection *proj,
    uint16_t x,
    uint16_t y,
    nexrad_geo_cartesian *cartesian
);

int nexrad_geo_projection_find_polar_point(nexrad_geo_projection *proj,
    uint16_t x,
    uint16_t y,
    nexrad_geo_polar *polar
);

nexrad_geo_projection_point *nexrad_geo_projection_get_points(nexrad_geo_projection *proj);

int nexrad_geo_projection_save(nexrad_geo_projection *proj);

void nexrad_geo_projection_close(nexrad_geo_projection *proj);

#endif /* _NEXRAD_GEO_POINT_H */
