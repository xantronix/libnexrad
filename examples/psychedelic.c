#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nexrad/message.h>
#include <nexrad/raster.h>
#include <nexrad/radial.h>

#include "../src/util.h"

static void usage(int argc, char **argv) {
    fprintf(stderr, "usage: %s colors.clut file.proj output.png\n", argv[0]);
    exit(1);
}

static nexrad_radial *create_radial() {
    int rangebins = 346;
    int rays      = 360;
    int i;

    size_t size = sizeof(nexrad_radial_packet)
        + rays * (sizeof(nexrad_radial_ray) + rangebins);

    nexrad_radial_packet *packet;

    if ((packet = malloc(size)) == NULL) {
        return NULL;
    }

    packet->type           = htobe16(16);
    packet->rangebin_first = htobe16(0);
    packet->rangebin_count = htobe16(rangebins);
    packet->i              = htobe16(0);
    packet->j              = htobe16(0);
    packet->scale          = htobe16(999);
    packet->rays           = htobe16(rays);

    for (i=0; i<rays; i++) {
        nexrad_radial_ray *ray = (nexrad_radial_ray *)((char *)packet
            + sizeof(nexrad_radial_packet)
            + i * (sizeof(nexrad_radial_ray) + rangebins));

        uint8_t *bins = (uint8_t *)ray + sizeof(nexrad_radial_ray);

        ray->size        = htobe16(rangebins);
        ray->angle_start = htobe16(10 * i);
        ray->angle_delta = htobe16(10);

        memset(bins, '\0', rangebins);

        if (i % 5 == 0) {
            int r;

            for (r=0; r<rangebins; r++) {
                bins[r] = (i + r) % 255;
            }
        }

        if (i % 45 == 0) {
            memset(bins, 0xff, rangebins);
        }
    }

    return nexrad_radial_packet_open(packet);
}

int main(int argc, char **argv) {
    char *clut, *projfile, *outfile;
    nexrad_image *image;
    nexrad_color_table *table;
    nexrad_geo_projection *proj;

    if (argc != 4) {
        usage(argc, argv);
    }

       clut  = argv[1];
    projfile = argv[2];
    outfile  = argv[3];

    if ((table = nexrad_color_table_load(clut)) == NULL) {
        perror("nexrad_color_table_load()");
        exit(1);
    }

    if ((proj = nexrad_geo_projection_open(projfile)) == NULL) {
        perror("nexrad_geo_projection_open()");
        exit(1);
    }

    if ((image = nexrad_radial_create_projected_image(create_radial(), table, proj)) == NULL) {
        perror("nexrad_radial_create_projected_image()");
        exit(1);
    }

    if (nexrad_image_save_png(image, outfile) < 0) {
        perror("nexrad_image_save_png()");
        exit(1);
    }

    return 0;
}
