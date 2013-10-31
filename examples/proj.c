#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nexrad/geo.h>

int main(int argc, char **argv) {
    nexrad_geo_spheroid *spheroid;
    nexrad_geo_radial_map *map;

    nexrad_geo_cartesian radar = {
        64.1333, -21.9333
    };

    spheroid = nexrad_geo_spheroid_create();

    if ((map = nexrad_geo_radial_map_create("foo.proj", spheroid, &radar, 346, 1000, 0.00815)) == NULL) {
        perror("nexrad_geo_radial_map_create()");
        exit(1);
    }

    if (nexrad_geo_radial_map_save(map) < 0) {
        perror("nexrad_geo_radial_map_save()");
        exit(1);
    }

    nexrad_geo_radial_map_close(map);

    return 0;
}
