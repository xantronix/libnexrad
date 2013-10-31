#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include "geodesic.h"

#include <nexrad/geo.h>

struct _nexrad_geo_spheroid {
    struct geod_geodesic * geod;

    double circumference;
    double flattening;
};

struct _nexrad_geo_radial_map {
    size_t size;
    size_t page_size;
    size_t mapped_size;
    int    fd;

    nexrad_geo_radial_map_header * header;
    nexrad_geo_radial_map_point  * points;
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

void nexrad_geo_radial_map_find_extents(nexrad_geo_spheroid *spheroid, nexrad_geo_cartesian *radar, uint16_t rangebins, uint16_t rangebin_meters, nexrad_geo_cartesian *extents) {
    double range = rangebins * rangebin_meters;

    uint16_t i, a;

    for (i=0, a=0; i<4; i++, a+=90) {
        nexrad_geo_polar polar = { a, range };

        nexrad_geo_find_cartesian_dest(spheroid, radar, &extents[i], &polar);
    }
}

void nexrad_geo_radial_map_get_dimensions(nexrad_geo_cartesian *extents, double scale, uint16_t *width, uint16_t *height) {
    *width  = (uint16_t)round((extents[1].lon - extents[3].lon) / scale);
    *height = (uint16_t)round((extents[0].lat - extents[2].lat) / scale);
}

static inline int _mapped_size(size_t size, size_t page_size) {
    return size + (page_size - (size % page_size));
}

nexrad_geo_radial_map *nexrad_geo_radial_map_create(const char *path, nexrad_geo_spheroid *spheroid, nexrad_geo_cartesian *radar, uint16_t rangebins, uint16_t rangebin_meters, double scale) {
    nexrad_geo_radial_map *map;
    nexrad_geo_cartesian extents[4];
    uint16_t width, height;

    if (path == NULL || spheroid == NULL || radar == NULL) {
        return NULL;
    }

    if ((map = malloc(sizeof(*map))) == NULL) {
        goto error_malloc_map;
    }

    map->size = sizeof(nexrad_geo_radial_map_header)
        + sizeof(nexrad_geo_radial_map_point) * width * height;

    map->page_size   = (size_t)sysconf(_SC_PAGESIZE);
    map->mapped_size = _mapped_size(map->size, map->page_size);

    if ((map->fd = open(path, O_CREAT | O_RDWR, 0644)) < 0) {
        goto error_open;
    }

    if ((map->header = mmap(NULL, map->mapped_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, map->fd, 0)) == NULL) {
        goto error_mmap;
    }

    map->points = (nexrad_geo_radial_map_point *)map->header +
        sizeof(nexrad_geo_radial_map_header);

    nexrad_geo_radial_map_find_extents(
        spheroid, radar, rangebins, rangebin_meters, extents
    );

    nexrad_geo_radial_map_get_dimensions(extents, scale, &width, &height);

    memcpy(map->header->magic, NEXRAD_GEO_RADIAL_MAP_MAGIC, 4);

    map->header->version         = htobe16(NEXRAD_GEO_RADIAL_MAP_VERSION);
    map->header->type            = htobe16(NEXRAD_GEO_RADIAL_MAP_EQUIRECT);
    map->header->width           = htobe16(width);
    map->header->height          = htobe16(height);
    map->header->rangebins       = htobe16(rangebins);
    map->header->rangebin_meters = htobe16(rangebin_meters);
    map->header->misc            = htobe32(scale / NEXRAD_GEO_COORD_MAGNITUDE);
    map->header->station_lat     = htobe32(radar->lat / NEXRAD_GEO_COORD_MAGNITUDE);
    map->header->station_lon     = htobe32(radar->lon / NEXRAD_GEO_COORD_MAGNITUDE);

    memset(map->points, '\0', sizeof(nexrad_geo_radial_map_point) * width * height);

    return map;

error_mmap:
    close(map->fd);

error_open:
    free(map);

error_malloc_map:
    return NULL;
}

void nexrad_geo_radial_map_close(nexrad_geo_radial_map *map) {
    if (map == NULL)
        return;

    if (map->header)
        munmap(map->header, map->mapped_size);

    if (map->fd)
        close(map->fd);

    memset(map, '\0', sizeof(*map));

    free(map);
}
