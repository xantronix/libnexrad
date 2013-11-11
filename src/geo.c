#include <stdio.h>
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

static inline int _mapped_size(size_t size, size_t page_size) {
    return size + (page_size - (size % page_size));
}

static nexrad_geo_radial_map *_radial_map_open(const char *path, size_t size, int new) {
    nexrad_geo_radial_map *map;

    int open_flags = 0;
    int mmap_prot  = 0;
    int mmap_flags = 0;

    if (new) {
        open_flags |= O_CREAT | O_TRUNC | O_RDWR;
        mmap_prot  |= PROT_READ | PROT_WRITE;
        mmap_flags |= MAP_SHARED;
    } else {
        open_flags |= O_RDONLY;
        mmap_prot  |= PROT_READ;
        mmap_flags |= MAP_PRIVATE;
    }

    if ((map = malloc(sizeof(*map))) == NULL) {
        goto error_malloc;
    }

    map->size        = size;
    map->page_size   = (size_t)sysconf(_SC_PAGESIZE);
    map->mapped_size = _mapped_size(size, map->page_size);

    if ((map->fd = open(path, open_flags, 0644)) < 0) {
        goto error_open;
    }

    if ((map->header = mmap(NULL, map->mapped_size, mmap_prot, mmap_flags, map->fd, 0)) == NULL) {
        goto error_mmap;
    }

    map->points = (nexrad_geo_radial_map_point *)((char *)map->header +
        sizeof(nexrad_geo_radial_map_header));

    return map;

error_mmap:
    close(map->fd);

error_open:
    free(map);

error_malloc:
    return NULL;
}

static double _equirect_find_lon(int x, int width) {
    return (360.0 * ((double)x / (double)width)) - 180.0;
}

static int _equirect_find_x(double lon, int width) {
    return (int)round((double)width * ((lon + 180.0) / 360.0));
}

static double _equirect_find_lat(int y, int height) {
    return (180.0 - (180.0 * ((double)y / (double)height))) - 90.0;
}

static int _equirect_find_y(double lat, int height) {
    return (int)round((double)height - ((double)height * ((lat + 90.0) / 180.0)));
}

nexrad_geo_radial_map *nexrad_geo_radial_map_create_equirect(const char *path, nexrad_geo_spheroid *spheroid, nexrad_geo_cartesian *radar, uint16_t rangebins, uint16_t rangebin_meters, double scale) {
    nexrad_geo_radial_map *map;

    nexrad_geo_cartesian extents[4];

    uint16_t earth_width, earth_height,
        earth_offset_x,
        earth_offset_y;

    uint16_t width, height, x, y;
    size_t size;

    if (path == NULL || spheroid == NULL || radar == NULL) {
        return NULL;
    }

    earth_height = (uint16_t)round(180.0 / scale);
    earth_width  = earth_height * 2;

    nexrad_geo_radial_map_find_extents(
        spheroid, radar, rangebins, rangebin_meters, extents
    );

    earth_offset_x = (uint16_t)_equirect_find_x(extents[3].lon, earth_width);
    earth_offset_y = (uint16_t)_equirect_find_y(extents[0].lat, earth_height);

    fprintf(stderr, "Thing 1: %d\n", _equirect_find_x(extents[3].lon, earth_width));
    fprintf(stderr, "Thing 2: %d\n", _equirect_find_x(extents[1].lon, earth_width));

    width  = (uint16_t)_equirect_find_x(extents[1].lon, earth_width)  - earth_offset_x;
    height = (uint16_t)_equirect_find_y(extents[2].lat, earth_height) - earth_offset_y;

    fprintf(stderr, "Scale: %f degrees lon/x\n", scale);
    fprintf(stderr, "Earth dimensions: %dx%d\n", earth_width, earth_height);
    fprintf(stderr, "Earth offset: %d, %d\n", earth_offset_x, earth_offset_y);
    fprintf(stderr, "Dimensions: %dx%d\n", width, height);

    size = sizeof(nexrad_geo_radial_map_header)
        + sizeof(nexrad_geo_radial_map_point) * width * height;

    if ((map = _radial_map_open(path, size, 1)) == NULL) {
        goto error_radial_map_open;
    }

    if (lseek(map->fd, size - 1, SEEK_SET) < 0) {
        goto error_lseek;
    }

    if (write(map->fd, "\0", 1) < 0) {
        goto error_write;
    }

    memcpy(map->header->magic, NEXRAD_GEO_RADIAL_MAP_MAGIC, 4);

    map->header->version         = htobe16(NEXRAD_GEO_RADIAL_MAP_VERSION);
    map->header->type            = htobe16(NEXRAD_GEO_RADIAL_MAP_EQUIRECT);
    map->header->width           = htobe16(width);
    map->header->height          = htobe16(height);
    map->header->rangebins       = htobe16(rangebins);
    map->header->rangebin_meters = htobe16(rangebin_meters);
    map->header->station_lat     = (int32_t)htobe32(radar->lat / NEXRAD_GEO_COORD_MAGNITUDE);
    map->header->station_lon     = (int32_t)htobe32(radar->lon / NEXRAD_GEO_COORD_MAGNITUDE);
    map->header->angle           = 0;

    for (x=0; x<4; x++) {
        map->header->extents[x].lat = (int32_t)htobe32(extents[x].lat / NEXRAD_GEO_COORD_MAGNITUDE);
        map->header->extents[x].lon = (int32_t)htobe32(extents[x].lon / NEXRAD_GEO_COORD_MAGNITUDE);
    }

    memset(&map->header->opts, '\0', sizeof(map->header->opts));
    map->header->opts.equirect.scale = htobe32(scale / NEXRAD_GEO_COORD_MAGNITUDE);

    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
            nexrad_geo_polar polar;
            nexrad_geo_radial_map_point *output = &map->points[y*width+x];

            int azimuth, range;

            nexrad_geo_cartesian point = {
                .lat = _equirect_find_lat(y + earth_offset_y, earth_height),
                .lon = _equirect_find_lon(x + earth_offset_x, earth_width)
            };

            if (x == 255 && y == 255) {
                fprintf(stderr, "Found point %.3f, %.3f at point %d, %d\n",
                    point.lat, point.lon,
                    x + earth_offset_x,
                    y + earth_offset_y
                );
            }

            nexrad_geo_find_polar_dest(spheroid, radar, &point, &polar);

            azimuth = (int)round(polar.azimuth);
            range   = (int)round(polar.range / rangebin_meters);

            while (azimuth >= 360) azimuth -= 360;
            while (azimuth <    0) azimuth += 360;

            output->azimuth = htobe16((uint16_t)azimuth);
            output->range   = htobe16((uint16_t)range);
        }
    }

    return map;

error_lseek:
error_write:
    nexrad_geo_radial_map_close(map);

error_radial_map_open:
    return NULL;
}

static double _mercator_find_lon(int x, int width) {
    return _equirect_find_lon(x, width);
}

static int _mercator_find_x(double lon, int width) {
    return (int)round((double)width * ((lon + 180.0) / 360.0));
}

static double _mercator_find_lat(int y, int height) {
    static const double deg = 180 / M_PI;

    int cy = height / 2;
    double r = M_PI * (((double)cy - (double)y) / (double)cy);

    return deg * atan(sinh(r));
}

static int _mercator_find_y(double lat, int height) {
    static const double rad = M_PI / 180;

    int cy   = height / 2;
    int sign = (lat >= 0)? 1: -1;

    double sinl = sin(sign * rad * lat);
    double yrad = sign * log((1.0 + sinl) / (1.0 - sinl)) / 2.0;

    return cy - (int)round(height * (yrad / (2 * M_PI)));
}

nexrad_geo_radial_map *nexrad_geo_radial_map_create_mercator(const char *path, nexrad_geo_spheroid *spheroid, nexrad_geo_cartesian *radar, uint16_t rangebins, uint16_t rangebin_meters, double scale) {
    nexrad_geo_radial_map *map;

    nexrad_geo_cartesian extents[4];

    uint16_t earth_size,
        earth_offset_x,
        earth_offset_y;

    uint16_t width, height, x, y;
    size_t size;

    if (path == NULL || spheroid == NULL || radar == NULL) {
        return NULL;
    }

    nexrad_geo_radial_map_find_extents(
        spheroid, radar, rangebins, rangebin_meters, extents
    );

    earth_size = (uint16_t)round(360 / scale);

    earth_offset_x = _mercator_find_x(extents[3].lon, earth_size);
    earth_offset_y = _mercator_find_y(extents[0].lat, earth_size);

    width  = _mercator_find_x(extents[1].lon, earth_size) - earth_offset_x;
    height = _mercator_find_y(extents[2].lat, earth_size) - earth_offset_y;

    size = sizeof(nexrad_geo_radial_map_header)
        + sizeof(nexrad_geo_radial_map_point) * width * height;

    if ((map = _radial_map_open(path, size, 1)) == NULL) {
        goto error_radial_map_open;
    }

    if (lseek(map->fd, size - 1, SEEK_SET) < 0) {
        goto error_lseek;
    }

    if (write(map->fd, "\0", 1) < 0) {
        goto error_write;
    }

    memcpy(map->header->magic, NEXRAD_GEO_RADIAL_MAP_MAGIC, 4);

    map->header->version         = htobe16(NEXRAD_GEO_RADIAL_MAP_VERSION);
    map->header->type            = htobe16(NEXRAD_GEO_RADIAL_MAP_MERCATOR);
    map->header->width           = htobe16(width);
    map->header->height          = htobe16(height);
    map->header->rangebins       = htobe16(rangebins);
    map->header->rangebin_meters = htobe16(rangebin_meters);
    map->header->station_lat     = htobe32(radar->lat / NEXRAD_GEO_COORD_MAGNITUDE);
    map->header->station_lon     = htobe32(radar->lon / NEXRAD_GEO_COORD_MAGNITUDE);
    map->header->angle           = 0;

    for (x=0; x<4; x++) {
        map->header->extents[x].lat = htobe32(extents[x].lat / NEXRAD_GEO_COORD_MAGNITUDE);
        map->header->extents[x].lon = htobe32(extents[x].lon / NEXRAD_GEO_COORD_MAGNITUDE);
    }

    memset(&map->header->opts, '\0', sizeof(map->header->opts));
    map->header->opts.mercator.scale = htobe32(scale / NEXRAD_GEO_COORD_MAGNITUDE);

    for (y=0; y<height; y++) {
        nexrad_geo_cartesian point = {
            .lat = _mercator_find_lat(y + earth_offset_y, earth_size),
            .lon = 0.0
        };

        for (x=0; x<width; x++) {
            nexrad_geo_polar polar;
            nexrad_geo_radial_map_point *output = &map->points[y*width+x];

            int azimuth, range;

            point.lon = _mercator_find_lon(x + earth_offset_x, earth_size);

            nexrad_geo_find_polar_dest(spheroid, radar, &point, &polar);

            azimuth = (int)round(polar.azimuth);
            range   = (int)round(polar.range / rangebin_meters);

            while (azimuth >= 360) azimuth -= 360;
            while (azimuth <    0) azimuth += 360;

            output->azimuth = htobe16((uint16_t)azimuth);
            output->range   = htobe16((uint16_t)range);
        }
    }

    return map;

error_lseek:
error_write:
    nexrad_geo_radial_map_close(map);

error_radial_map_open:
    return NULL;
}

static int _is_valid_radial_map_header(nexrad_geo_radial_map_header *header) {
    if (strncmp(header->magic, NEXRAD_GEO_RADIAL_MAP_MAGIC, 4) != 0)
        return 0;

    if (be16toh(header->version) != NEXRAD_GEO_RADIAL_MAP_VERSION)
        return 0;

    switch (be16toh(header->type)) {
        case NEXRAD_GEO_RADIAL_MAP_EQUIRECT:
        case NEXRAD_GEO_RADIAL_MAP_MERCATOR: {
            break;
        }

        default: {
            return 0;
        }
    }

    return 1;
}

nexrad_geo_radial_map *nexrad_geo_radial_map_open(const char *path) {
    nexrad_geo_radial_map *map;
    struct stat st;

    if (path == NULL) {
        return NULL;
    }

    if (stat(path, &st) < 0) {
        goto error_stat;
    }

    if ((map = _radial_map_open(path, st.st_size, 0)) == NULL) {
        goto error_radial_map_open;
    }

    if (!_is_valid_radial_map_header(map->header)) {
        goto error_invalid_radial_map_header;
    }

    return map;

error_invalid_radial_map_header:
    nexrad_geo_radial_map_close(map);

error_radial_map_open:
error_stat:
    return NULL;
}

enum nexrad_geo_radial_map_type nexrad_geo_radial_map_get_type(nexrad_geo_radial_map *map) {
    if (map == NULL) {
        return -1;
    }

    return be16toh(map->header->type);
}

int nexrad_geo_radial_map_read_dimensions(nexrad_geo_radial_map *map, uint16_t *width, uint16_t *height) {
    if (map == NULL) {
        return -1;
    }

    if (width)
        *width = be16toh(map->header->width);

    if (height)
        *height = be16toh(map->header->height);

    return 0;
}

int nexrad_geo_radial_map_read_range(nexrad_geo_radial_map *map, uint16_t *rangebins, uint16_t *rangebin_meters) {
    if (map == NULL) {
        return -1;
    }

    if (rangebins)
        *rangebins = be16toh(map->header->rangebins);

    if (rangebin_meters)
        *rangebin_meters = be16toh(map->header->rangebin_meters);

    return 0;
}

int nexrad_geo_radial_map_read_station_location(nexrad_geo_radial_map *map, nexrad_geo_cartesian *radar) {
    if (map == NULL) {
        return -1;
    }

    if (radar) {
        radar->lat = NEXRAD_GEO_COORD_MAGNITUDE * (int32_t)be32toh(map->header->station_lat);
        radar->lon = NEXRAD_GEO_COORD_MAGNITUDE * (int32_t)be32toh(map->header->station_lon);
    }

    return 0;
}

int nexrad_geo_radial_map_find_polar_point(nexrad_geo_radial_map *map, uint16_t x, uint16_t y, nexrad_geo_polar *polar) {
    uint16_t width;

    if (map == NULL || x > be16toh(map->header->width) || y > be16toh(map->header->height)) {
        return -1;
    }

    width = be16toh(map->header->width);

    if (polar) {
        nexrad_geo_radial_map_point *point = &map->points[y*width+x];

        polar->azimuth = be16toh(point->azimuth);
        polar->range   = be16toh(map->header->rangebin_meters) * be16toh(point->range);
    }

    return 0;
}

nexrad_geo_radial_map_point *nexrad_geo_radial_map_get_points(nexrad_geo_radial_map *map) {
    if (map == NULL) {
        return NULL;
    }

    return map->points;
}

int nexrad_geo_radial_map_save(nexrad_geo_radial_map *map) {
    if (map == NULL) {
        return -1;
    }

    return msync(map->header, map->size, MS_SYNC);
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
