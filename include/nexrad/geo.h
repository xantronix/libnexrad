#ifndef _NEXRAD_GEO_H
#define _NEXRAD_GEO_H

#define NEXRAD_GEO_SPHEROID_CIRCUMFERENCE 6378137.0
#define NEXRAD_GEO_SPHEROID_FLATTENING    (1/298.257223563)

#define NEXRAD_GEO_NM_METERS 1852

#define NEXRAD_GEO_PROJECTION_MAGIC   "PROJ"
#define NEXRAD_GEO_PROJECTION_VERSION 0x01

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

typedef struct _nexrad_geo_projection_header {
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
    char     station_name[4];
     int32_t station_lat;
     int32_t station_lon;
} nexrad_geo_projection_header;

typedef struct _nexrad_geo_spheroid   nexrad_geo_spheroid;
typedef struct _nexrad_geo_projection nexrad_geo_projection;

nexrad_geo_spheroid *nexrad_geo_spheroid_create();

double nexrad_geo_spheroid_get_circumference(nexrad_geo_spheroid *spheroid);

double nexrad_geo_spheroid_get_flattening(nexrad_geo_spheroid *spheroid);

void nexrad_geo_find_polar_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * dest,
    nexrad_geo_polar *     polar
);

void nexrad_geo_find_cartesian_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * dest,
    nexrad_geo_polar *     polar
);

void nexrad_geo_spheroid_destroy(nexrad_geo_spheroid *spheroid);

#endif /* _NEXRAD_GEO_POINT_H */
