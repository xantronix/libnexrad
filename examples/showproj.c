#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nexrad/geo.h>

int main(int argc, char **argv) {
    nexrad_geo_radial_map *map;
    nexrad_geo_cartesian radar;
    nexrad_geo_polar polar;

    if ((map = nexrad_geo_radial_map_open("foo.proj")) == NULL) {
        perror("nexrad_geo_radial_map_open()");
    }

    if (nexrad_geo_radial_map_read_station_location(map, &radar) < 0) {
        perror("nexrad_geo_radial_map_read_station_location()");
    }

    if (nexrad_geo_radial_map_find_polar_point(map, 255, 255, &polar) < 0) {
        perror("nexrad_geo_radial_map_read_polar_point()");
    }

    fprintf(stderr, "Station: %.3f, %.3f\n", radar.lat, radar.lon);

    fprintf(stderr, "Azimuth, range at 255, 255: %.3f, %.3f\n",
        polar.azimuth, polar.range
    );

    nexrad_geo_radial_map_close(map);

    return 0;
}
