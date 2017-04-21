#ifndef _NEXRAD_MAP_H
#define _NEXRAD_MAP_H

#include <nexrad/image.h>
#include <nexrad/radial.h>

#define NEXRAD_MAP_EARTH_RADIUS      6378137.0
#define NEXRAD_MAP_REFRACTION_FACTOR (4.0f/3.0f)

#define NEXRAD_MAP_MAX_LAT    85.05112878
#define NEXRAD_MAP_TILE_SIZE 256
#define NEXRAD_MAP_MIN_ZOOM    4
#define NEXRAD_MAP_MAX_ZOOM   10

typedef struct _nexrad_map_point {
    float lat, lon;
} nexrad_map_point;

typedef struct _nexrad_map_heading {
    float azimuth, range;
} nexrad_map_heading;

typedef struct _nexrad_map_radar {
    float lat, lon, altitude;
    char name[4];
} nexrad_map_radar;

void nexrad_map_find_point(nexrad_map_point start,
                           nexrad_map_heading heading,
                           nexrad_map_point *end);

void nexrad_map_find_heading(nexrad_map_point start,
                             nexrad_map_point end,
                             nexrad_map_heading *heading);

nexrad_image *nexrad_map_project_radial(nexrad_radial *radial,
    nexrad_map_radar *radar,
    nexrad_color_table *clut,
    float tilt,
    float resolution,
    int zoom);

#endif /* _NEXRAD_MAP_H */
