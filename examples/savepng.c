#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nexrad/message.h>
#include <nexrad/raster.h>
#include <nexrad/radial.h>

static void usage(int argc, char **argv) {
    fprintf(stderr, "usage: %s colors.clut file.proj input.l3 output.png\n", argv[0]);
    exit(1);
}

static nexrad_image *get_product_image(const char *file, nexrad_color_table *table, nexrad_geo_projection *proj) {
    nexrad_message *message;
    nexrad_symbology_block *symbology;
    nexrad_chunk *block, *layer;

    if ((message = nexrad_message_open(file)) == NULL) {
        goto error_message_open;
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

                    nexrad_image *image = nexrad_radial_create_projected_image(radial, table, proj);

                    return image;
                }

                case NEXRAD_PACKET_RADIAL_AF1F: {
                    nexrad_radial *radial = nexrad_radial_packet_open((nexrad_radial_packet *)packet);

                    nexrad_image *image = nexrad_radial_create_projected_image(radial, table, proj);

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
    nexrad_message_close(message);

error_message_open:
    return NULL;
}

int main(int argc, char **argv) {
    char *clut, *infile, *projfile, *outfile;
    nexrad_image *image;
    nexrad_color_table *table;
    nexrad_geo_projection *proj;

    if (argc != 5) {
        usage(argc, argv);
    }

       clut  = argv[1];
    projfile = argv[2];
    infile   = argv[3];
    outfile  = argv[4];

    if ((table = nexrad_color_table_load(clut)) == NULL) {
        perror("nexrad_color_table_load()");
        exit(1);
    }

    if ((proj = nexrad_geo_projection_open(projfile)) == NULL) {
        perror("nexrad_geo_projection_open()");
        exit(1);
    }

    if ((image = get_product_image(infile, table, proj)) == NULL) {
        perror("get_produt_image()");
        exit(1);
    }

    if (nexrad_image_save_png(image, outfile) < 0) {
        perror("nexrad_image_save_png()");
        exit(1);
    }

    return 0;
}
