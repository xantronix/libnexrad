#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nexrad/message.h>
#include <nexrad/raster.h>
#include <nexrad/radial.h>

static void usage(int argc, char **argv) {
    fprintf(stderr, "usage: %s colors.clut input.l3 output.png\n", argv[0]);
    exit(1);
}

static nexrad_image *get_product_image(const char *file, nexrad_color_table *table) {
    nexrad_message *message;
    nexrad_symbology_block *symbology;
    nexrad_chunk *block, *layer;

    nexrad_geo_spheroid *spheroid;
    nexrad_geo_cartesian radar;

    if ((message = nexrad_message_open(file)) == NULL) {
        goto error_message_open;
    }

    if ((spheroid = nexrad_geo_spheroid_create()) == NULL) {
        goto error_geo_spheroid_create;
    }

    if (nexrad_message_read_station_location(message, &radar.lat, &radar.lon, NULL) < 0) {
        goto error_message_read_station_location;
    }

    if ((symbology = nexrad_message_get_symbology_block(message)) == NULL) {
        goto error_message_get_symbology_block;
    }

    if ((block = nexrad_symbology_block_open(symbology)) == NULL) {
        goto error_symbology_block_open;
    }

    while ((layer = nexrad_symbology_block_read_layer(block)) != NULL) {
        nexrad_packet *packet;
        size_t size;

        while ((packet = nexrad_symbology_layer_peek_packet(layer, &size)) != NULL) {
            enum nexrad_packet_type type = nexrad_packet_get_type(packet);

            switch (type) {
                case NEXRAD_PACKET_RADIAL: {
                    nexrad_radial *radial = nexrad_radial_packet_open((nexrad_radial_packet *)packet);

                    nexrad_image *image = nexrad_radial_create_unprojected_image(radial, table, &radar, spheroid, 0.00815);

                    return image;
                }

                case NEXRAD_PACKET_RADIAL_AF1F: {
                    nexrad_radial *radial = nexrad_radial_packet_open((nexrad_radial_packet *)packet);

                    nexrad_image *image = nexrad_radial_create_unprojected_image(radial, table, &radar, spheroid, 0.00815);

                    return image;
                }

                case NEXRAD_PACKET_RASTER_BA0F:
                case NEXRAD_PACKET_RASTER_BA07: {
                    nexrad_raster *raster = nexrad_raster_packet_open((nexrad_raster_packet *)packet);

                    nexrad_image *image = nexrad_raster_create_image(raster, table);

                    return image;
                }

                default: {
                    fprintf(stderr, "Got packet type %d\n", type);

                    break;
                }
            }

            nexrad_symbology_layer_next_packet(layer, size);
        }
    }

    return NULL;

error_symbology_block_open:
error_message_get_symbology_block:
error_message_read_station_location:
error_geo_spheroid_create:
    nexrad_message_close(message);

error_message_open:
    return NULL;
}

int main(int argc, char **argv) {
    char *clut, *infile, *outfile;
    nexrad_image *image;
    nexrad_color_table *table;

    if (argc != 4) {
        usage(argc, argv);
    }

       clut = argv[1];
     infile = argv[2];
    outfile = argv[3];

    if ((table = nexrad_color_table_load(clut)) == NULL) {
        perror("nexrad_color_table_load()");
        exit(1);
    }

    if ((image = get_product_image(infile, table)) == NULL) {
        perror("get_produt_image()");
        exit(1);
    }

    if (nexrad_image_save_png(image, outfile) < 0) {
        perror("nexrad_image_save_png()");
        exit(1);
    }

    return 0;
}
