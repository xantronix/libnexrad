#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include "geodesic.h"
#include <errno.h>

#include <nexrad/geo.h>

struct _nexrad_geo_spheroid {
    struct geod_geodesic * geod;

    double circumference;
    double flattening;
};

struct _nexrad_geo_projection {
    size_t size;
    size_t page_size;
    size_t mapped_size;
    int    fd;

    nexrad_geo_projection_header * header;

    struct {
        uint16_t azimuth;
        uint16_t range;
    } * mappings;
};

nexrad_geo_spheroid *nexrad_geo_spheroid_create() {
    nexrad_geo_spheroid *spheroid;

    if ((spheroid = malloc(sizeof(nexrad_geo_spheroid))) == NULL) {
        goto error_malloc_spheroid;
    }

    if ((spheroid->geod = malloc(sizeof(struct geod_geodesic))) == NULL) {
        goto error_malloc_geod;
    }

    geod_init(spheroid->geod,
        NEXRAD_GEO_SPHEROID_CIRCUMFERENCE,
        NEXRAD_GEO_SPHEROID_FLATTENING
    );

    errno = 0;

    spheroid->circumference = NEXRAD_GEO_SPHEROID_CIRCUMFERENCE;
    spheroid->flattening    = NEXRAD_GEO_SPHEROID_FLATTENING;

    return spheroid;

error_malloc_geod:
    free(spheroid);

error_malloc_spheroid:
    return NULL;
}

double nexrad_geo_spheroid_get_circumference(nexrad_geo_spheroid *spheroid) {
    if (spheroid == NULL) {
        return -1;
    }

    return spheroid->circumference;
}

double nexrad_geo_spheroid_get_flattening(nexrad_geo_spheroid *spheroid) {
    if (spheroid == NULL) {
        return -1;
    }

    return spheroid->flattening;
}

void nexrad_geo_find_polar_dest(nexrad_geo_spheroid *spheroid, nexrad_geo_cartesian *origin, nexrad_geo_cartesian *dest, nexrad_geo_polar *polar) {
    if (spheroid == NULL || origin == NULL || dest == NULL || polar == NULL) {
        return;
    }

    geod_inverse(spheroid->geod,
        origin->lat, origin->lon, dest->lat, dest->lon, &polar->range, NULL, &polar->azimuth
    );
}

void nexrad_geo_find_cartesian_dest(nexrad_geo_spheroid *spheroid, nexrad_geo_cartesian *origin, nexrad_geo_cartesian *dest, nexrad_geo_polar *polar) {
    if (spheroid == NULL || origin == NULL || dest == NULL || polar == NULL) {
        return;
    }

    geod_direct(spheroid->geod,
        origin->lat, origin->lon, polar->azimuth, polar->range, &dest->lat, &dest->lon, NULL
    );
}

void nexrad_geo_spheroid_destroy(nexrad_geo_spheroid *spheroid) {
    if (spheroid == NULL) {
        return;
    }

    if (spheroid->geod != NULL) {
        free(spheroid->geod);
        spheroid->geod = NULL;
    }

    free(spheroid);
}
