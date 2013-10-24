#ifndef _NEXRAD_GEO_H
#define _NEXRAD_GEO_H

#define NEXRAD_GEO_SPHEROID_CIRCUMFERENCE 6378137.0
#define NEXRAD_GEO_SPHEROID_FLATTENING    (1/298.257223563)

#define NEXRAD_GEO_NM_METERS 1852

typedef struct _nexrad_geo_cartesian {
    double lat;
    double lon;
} nexrad_geo_cartesian;

typedef struct _nexrad_geo_polar {
    double azimuth;
    double range;
} nexrad_geo_polar;

typedef struct _nexrad_geo_spheroid nexrad_geo_spheroid;

nexrad_geo_spheroid *nexrad_geo_spheroid_create();

double nexrad_geo_spheroid_get_circumference(nexrad_geo_spheroid *spheroid);

double nexrad_geo_spheroid_get_flattening(nexrad_geo_spheroid *spheroid);

void nexrad_geo_spheroid_find_cartesian_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * dest,
    nexrad_geo_polar *     polar
);

void nexrad_geo_spheroid_find_polar_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * dest,
    nexrad_geo_polar *     polar
);

void nexrad_geo_spheroid_destroy(nexrad_geo_spheroid *spheroid);

#endif /* _NEXRAD_GEO_POINT_H */
