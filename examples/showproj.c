#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nexrad/geo.h>

int main(int argc, char **argv) {
    nexrad_geo_projection *proj;
    nexrad_geo_cartesian radar;
    nexrad_geo_polar polar;

    if ((proj = nexrad_geo_projection_open("foo.proj")) == NULL) {
        perror("nexrad_geo_projection_open()");
    }

    if (nexrad_geo_projection_read_station_location(proj, &radar) < 0) {
        perror("nexrad_geo_projection_read_station_location()");
    }

    if (nexrad_geo_projection_find_polar_point(proj, 255, 255, &polar) < 0) {
        perror("nexrad_geo_projection_read_polar_point()");
    }

    fprintf(stderr, "Station: %.3f, %.3f\n", radar.lat, radar.lon);

    fprintf(stderr, "Azimuth, range at 255, 255: %.3f, %.3f\n",
        polar.azimuth, polar.range
    );

    nexrad_geo_projection_close(proj);

    return 0;
}
