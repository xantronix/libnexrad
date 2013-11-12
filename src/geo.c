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
#include "util.h"

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
    nexrad_geo_projection_point  * points;
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

void nexrad_geo_projection_find_extents(nexrad_geo_spheroid *spheroid, nexrad_geo_cartesian *radar, uint16_t rangebins, uint16_t rangebin_meters, nexrad_geo_cartesian *extents) {
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

static nexrad_geo_projection *_projection_open(const char *path, size_t size, int new) {
    nexrad_geo_projection *proj;

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

    if ((proj = malloc(sizeof(*proj))) == NULL) {
        goto error_malloc;
    }

    proj->size        = size;
    proj->page_size   = (size_t)sysconf(_SC_PAGESIZE);
    proj->mapped_size = _mapped_size(size, proj->page_size);

    if ((proj->fd = open(path, open_flags, 0644)) < 0) {
        goto error_open;
    }

    if ((proj->header = mmap(NULL, proj->mapped_size, mmap_prot, mmap_flags, proj->fd, 0)) == NULL) {
        goto error_mmap;
    }

    proj->points = (nexrad_geo_projection_point *)((char *)proj->header +
        sizeof(nexrad_geo_projection_header));

    return proj;

error_mmap:
    close(proj->fd);

error_open:
    free(proj);

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

nexrad_geo_projection *nexrad_geo_projection_create_equirect(const char *path, nexrad_geo_spheroid *spheroid, nexrad_geo_cartesian *radar, uint16_t rangebins, uint16_t rangebin_meters, double scale) {
    nexrad_geo_projection *proj;

    nexrad_geo_cartesian extents[4];

    uint16_t world_width, world_height,
        world_offset_x,
        world_offset_y;

    uint16_t width, height, x, y;
    size_t size;

    if (path == NULL || spheroid == NULL || radar == NULL) {
        return NULL;
    }

    world_height = (uint16_t)round(180.0 / scale);
    world_width  = world_height * 2;

    nexrad_geo_projection_find_extents(
        spheroid, radar, rangebins, rangebin_meters, extents
    );

    world_offset_x = (uint16_t)_equirect_find_x(extents[3].lon, world_width);
    world_offset_y = (uint16_t)_equirect_find_y(extents[0].lat, world_height);

    width  = (uint16_t)_equirect_find_x(extents[1].lon, world_width)  - world_offset_x;
    height = (uint16_t)_equirect_find_y(extents[2].lat, world_height) - world_offset_y;

    size = sizeof(nexrad_geo_projection_header)
        + sizeof(nexrad_geo_projection_point) * width * height;

    if ((proj = _projection_open(path, size, 1)) == NULL) {
        goto error_projection_open;
    }

    if (lseek(proj->fd, size - 1, SEEK_SET) < 0) {
        goto error_lseek;
    }

    if (write(proj->fd, "\0", 1) < 0) {
        goto error_write;
    }

    memcpy(proj->header->magic, NEXRAD_GEO_PROJECTION_MAGIC, 4);

    proj->header->version         = htobe16(NEXRAD_GEO_PROJECTION_VERSION);
    proj->header->type            = htobe16(NEXRAD_GEO_PROJECTION_EQUIRECT);
    proj->header->width           = htobe16(width);
    proj->header->height          = htobe16(height);
    proj->header->rangebins       = htobe16(rangebins);
    proj->header->rangebin_meters = htobe16(rangebin_meters);
    proj->header->station_lat     = (int32_t)htobe32((int32_t)round(radar->lat / NEXRAD_GEO_COORD_MAGNITUDE));
    proj->header->station_lon     = (int32_t)htobe32((int32_t)round(radar->lon / NEXRAD_GEO_COORD_MAGNITUDE));
    proj->header->angle           = 0;

    for (x=0; x<4; x++) {
        proj->header->extents[x].lat = (int32_t)htobe32((int32_t)round(extents[x].lat / NEXRAD_GEO_COORD_MAGNITUDE));
        proj->header->extents[x].lon = (int32_t)htobe32((int32_t)round(extents[x].lon / NEXRAD_GEO_COORD_MAGNITUDE));
    }

    memset(&proj->header->opts, '\0', sizeof(proj->header->opts));
    proj->header->opts.equirect.scale = htobe32((int32_t)round(scale / NEXRAD_GEO_COORD_MAGNITUDE));

    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
            nexrad_geo_polar polar;
            nexrad_geo_projection_point *output = &proj->points[y*width+x];

            int azimuth, range;

            nexrad_geo_cartesian point = {
                .lat = _equirect_find_lat(y + world_offset_y, world_height),
                .lon = _equirect_find_lon(x + world_offset_x, world_width)
            };

            nexrad_geo_find_polar_dest(spheroid, radar, &point, &polar);

            azimuth = (int)round(polar.azimuth);
            range   = (int)round(polar.range / rangebin_meters);

            while (azimuth >= 360) azimuth -= 360;
            while (azimuth <    0) azimuth += 360;

            output->azimuth = htobe16((uint16_t)azimuth);
            output->range   = htobe16((uint16_t)range);
        }
    }

    return proj;

error_lseek:
error_write:
    nexrad_geo_projection_close(proj);

error_projection_open:
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

nexrad_geo_projection *nexrad_geo_projection_create_mercator(const char *path, nexrad_geo_spheroid *spheroid, nexrad_geo_cartesian *radar, uint16_t rangebins, uint16_t rangebin_meters, int zoom) {
    nexrad_geo_projection *proj;

    nexrad_geo_cartesian extents[4];

    size_t world_size,
        world_offset_x,
        world_offset_y;

    uint16_t width, height, x, y;
    size_t size;

    if (path == NULL || spheroid == NULL || radar == NULL) {
        return NULL;
    }

    if (zoom < NEXRAD_GEO_MERCATOR_MIN_ZOOM || zoom > NEXRAD_GEO_MERCATOR_MAX_ZOOM) {
        return NULL;
    }

    nexrad_geo_projection_find_extents(
        spheroid, radar, rangebins, rangebin_meters, extents
    );

    world_size = NEXRAD_GEO_MERCATOR_TILE_SIZE * pow(2, zoom);

    world_offset_x = _mercator_find_x(extents[3].lon, world_size);
    world_offset_y = _mercator_find_y(extents[0].lat, world_size);

    width  = _mercator_find_x(extents[1].lon, world_size) - world_offset_x;
    height = _mercator_find_y(extents[2].lat, world_size) - world_offset_y;

    size = sizeof(nexrad_geo_projection_header)
        + sizeof(nexrad_geo_projection_point) * width * height;

    if ((proj = _projection_open(path, size, 1)) == NULL) {
        goto error_projection_open;
    }

    if (lseek(proj->fd, size - 1, SEEK_SET) < 0) {
        goto error_lseek;
    }

    if (write(proj->fd, "\0", 1) < 0) {
        goto error_write;
    }

    memcpy(proj->header->magic, NEXRAD_GEO_PROJECTION_MAGIC, 4);

    proj->header->version         = htobe16(NEXRAD_GEO_PROJECTION_VERSION);
    proj->header->type            = htobe16(NEXRAD_GEO_PROJECTION_MERCATOR);
    proj->header->width           = htobe16(width);
    proj->header->height          = htobe16(height);
    proj->header->rangebins       = htobe16(rangebins);
    proj->header->rangebin_meters = htobe16(rangebin_meters);
    proj->header->station_lat     = htobe32((int32_t)round(radar->lat / NEXRAD_GEO_COORD_MAGNITUDE));
    proj->header->station_lon     = htobe32((int32_t)round(radar->lon / NEXRAD_GEO_COORD_MAGNITUDE));
    proj->header->angle           = 0;

    for (x=0; x<4; x++) {
        proj->header->extents[x].lat = htobe32((int32_t)round(extents[x].lat / NEXRAD_GEO_COORD_MAGNITUDE));
        proj->header->extents[x].lon = htobe32((int32_t)round(extents[x].lon / NEXRAD_GEO_COORD_MAGNITUDE));
    }

    memset(&proj->header->opts, '\0', sizeof(proj->header->opts));
    proj->header->opts.mercator.zoom  = htobe16(zoom);
    proj->header->opts.mercator.scale = htobe32((uint32_t)round(360.0 / (double)world_size / NEXRAD_GEO_COORD_MAGNITUDE));

    for (y=0; y<height; y++) {
        nexrad_geo_cartesian point = {
            .lat = _mercator_find_lat(y + world_offset_y, world_size),
            .lon = 0.0
        };

        for (x=0; x<width; x++) {
            nexrad_geo_polar polar;
            nexrad_geo_projection_point *output = &proj->points[y*width+x];

            int azimuth, range;

            point.lon = _mercator_find_lon(x + world_offset_x, world_size);

            nexrad_geo_find_polar_dest(spheroid, radar, &point, &polar);

            azimuth = (int)round(polar.azimuth);
            range   = (int)round(polar.range / rangebin_meters);

            while (azimuth >= 360) azimuth -= 360;
            while (azimuth <    0) azimuth += 360;

            output->azimuth = htobe16((uint16_t)azimuth);
            output->range   = htobe16((uint16_t)range);
        }
    }

    return proj;

error_lseek:
error_write:
    nexrad_geo_projection_close(proj);

error_projection_open:
    return NULL;
}

static int _is_valid_projection_header(nexrad_geo_projection_header *header) {
    if (strncmp(header->magic, NEXRAD_GEO_PROJECTION_MAGIC, 4) != 0)
        return 0;

    if (be16toh(header->version) != NEXRAD_GEO_PROJECTION_VERSION)
        return 0;

    switch (be16toh(header->type)) {
        case NEXRAD_GEO_PROJECTION_EQUIRECT:
        case NEXRAD_GEO_PROJECTION_MERCATOR: {
            break;
        }

        default: {
            return 0;
        }
    }

    return 1;
}

nexrad_geo_projection *nexrad_geo_projection_open(const char *path) {
    nexrad_geo_projection *proj;
    struct stat st;

    if (path == NULL) {
        return NULL;
    }

    if (stat(path, &st) < 0) {
        goto error_stat;
    }

    if ((proj = _projection_open(path, st.st_size, 0)) == NULL) {
        goto error_projection_open;
    }

    if (!_is_valid_projection_header(proj->header)) {
        goto error_invalid_projection_header;
    }

    return proj;

error_invalid_projection_header:
    nexrad_geo_projection_close(proj);

error_projection_open:
error_stat:
    return NULL;
}

enum nexrad_geo_projection_type nexrad_geo_projection_get_type(nexrad_geo_projection *proj) {
    if (proj == NULL) {
        return -1;
    }

    return be16toh(proj->header->type);
}

int nexrad_geo_projection_read_dimensions(nexrad_geo_projection *proj, uint16_t *width, uint16_t *height) {
    if (proj == NULL) {
        return -1;
    }

    if (width)
        *width = be16toh(proj->header->width);

    if (height)
        *height = be16toh(proj->header->height);

    return 0;
}

int nexrad_geo_projection_read_range(nexrad_geo_projection *proj, uint16_t *rangebins, uint16_t *rangebin_meters) {
    if (proj == NULL) {
        return -1;
    }

    if (rangebins)
        *rangebins = be16toh(proj->header->rangebins);

    if (rangebin_meters)
        *rangebin_meters = be16toh(proj->header->rangebin_meters);

    return 0;
}

int nexrad_geo_projection_read_station_location(nexrad_geo_projection *proj, nexrad_geo_cartesian *radar) {
    if (proj == NULL) {
        return -1;
    }

    if (radar) {
        radar->lat = NEXRAD_GEO_COORD_MAGNITUDE * (int32_t)be32toh(proj->header->station_lat);
        radar->lon = NEXRAD_GEO_COORD_MAGNITUDE * (int32_t)be32toh(proj->header->station_lon);
    }

    return 0;
}

int nexrad_geo_projection_read_extents(nexrad_geo_projection *proj, nexrad_geo_cartesian *extents) {
    int i;

    if (proj == NULL || extents == NULL) {
        return -1;
    }

    for (i=0; i<4; i++) {
        extents[i].lat = NEXRAD_GEO_COORD_MAGNITUDE * (int32_t)be32toh(proj->header->extents[i].lat);
        extents[i].lon = NEXRAD_GEO_COORD_MAGNITUDE * (int32_t)be32toh(proj->header->extents[i].lon);
    }

    return 0;
}

int nexrad_geo_projection_find_polar_point(nexrad_geo_projection *proj, uint16_t x, uint16_t y, nexrad_geo_polar *polar) {
    uint16_t width;

    if (proj == NULL || x > be16toh(proj->header->width) || y > be16toh(proj->header->height)) {
        return -1;
    }

    width = be16toh(proj->header->width);

    if (polar) {
        nexrad_geo_projection_point *point = &proj->points[y*width+x];

        polar->azimuth = be16toh(point->azimuth);
        polar->range   = be16toh(proj->header->rangebin_meters) * be16toh(point->range);
    }

    return 0;
}

nexrad_geo_projection_point *nexrad_geo_projection_get_points(nexrad_geo_projection *proj) {
    if (proj == NULL) {
        return NULL;
    }

    return proj->points;
}

int nexrad_geo_projection_save(nexrad_geo_projection *proj) {
    if (proj == NULL) {
        return -1;
    }

    return msync(proj->header, proj->size, MS_SYNC);
}

void nexrad_geo_projection_close(nexrad_geo_projection *proj) {
    if (proj == NULL)
        return;

    if (proj->header)
        munmap(proj->header, proj->mapped_size);

    if (proj->fd)
        close(proj->fd);

    memset(proj, '\0', sizeof(*proj));

    free(proj);
}
