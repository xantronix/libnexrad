#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nexrad/geo.h>

int main(int argc, char **argv) {
    nexrad_geo_spheroid *spheroid;
    nexrad_geo_projection *proj;

    nexrad_geo_cartesian radar = {
        35.333, -97.278
    };

    spheroid = nexrad_geo_spheroid_create();

    if ((proj = nexrad_geo_projection_create_equirect("equirect.proj", spheroid, &radar, 346, 1000, 0.00815)) == NULL) {
        perror("nexrad_geo_projection_create_equirect()");
        exit(1);
    }

    if (nexrad_geo_projection_save(proj) < 0) {
        perror("nexrad_geo_projection_save()");
        exit(1);
    }

    nexrad_geo_projection_close(proj);

    if ((proj = nexrad_geo_projection_create_mercator("mercator.proj", spheroid, &radar, 346, 1000, 8)) == NULL) {
        perror("nexrad_geo_projection_create_mercator()");
        exit(1);
    }

    if (nexrad_geo_projection_save(proj) < 0) {
        perror("nexrad_geo_projection_save()");
        exit(1);
    }

    nexrad_geo_projection_close(proj);

    return 0;
}
