#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nexrad/geo.h>

int main(int argc, char **argv) {
    nexrad_geo_radial_map *map;
    nexrad_geo_cartesian radar;

    if ((map = nexrad_geo_radial_map_open("foo.proj")) == NULL) {
        perror("nexrad_geo_radial_map_open()");
    }

    if (nexrad_geo_radial_map_read_station_location(map, &radar) < 0) {
        perror("nexrad_geo_radial_map_read_station_location()");
    }

    fprintf(stderr, "Station: %.3f, %.3f\n", radar.lat, radar.lon);

    nexrad_geo_radial_map_close(map);

    return 0;
}
