#ifndef _NEXRAD_MAP_H
#define _NEXRAD_MAP_H

#include <nexrad/image.h>
#include <nexrad/radial.h>

typedef struct _nexrad_map_point {
    union {
        float lat;
        float azimuth;
    };

    union {
        float lon;
        float range;
    };
} nexrad_map_point;

typedef struct _nexrad_map_radar {
    float lat, lon, altitude;
    char name[4];
} nexrad_map_radar;

nexrad_image *nexrad_map_project_radial(nexrad_radial *radial,
    nexrad_map_radar *radar,
    nexrad_color_table *clut,
    int zoom);

#endif /* _NEXRAD_MAP_H */
