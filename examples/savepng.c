#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nexrad/message.h>
#include <nexrad/radial.h>
#include <nexrad/raster.h>

static void usage(int argc, char **argv) {
    fprintf(stderr, "usage: %s input.l3 output.png\n", argv[0]);
    exit(1);
}

static int save_product_image(const char *infile, const char *outfile) {
    nexrad_message *message;
    nexrad_symbology_block *symbology;
    nexrad_chunk *block, *layer;

    if ((message = nexrad_message_open(infile)) == NULL) {
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
                case NEXRAD_PACKET_RADIAL:
                case NEXRAD_PACKET_RADIAL_AF1F: {
                    nexrad_radial *radial = nexrad_radial_packet_open((nexrad_radial_packet *)packet);

                    nexrad_radial_image *image = nexrad_radial_create_image(radial,
                        CAIRO_FORMAT_RGB24
                    );

                    fprintf(stderr, "Found radial packet, creating image %s\n", outfile);

                    if (nexrad_radial_image_save_png(image, outfile) < 0) {
                        return -1;
                    }

                    nexrad_radial_image_destroy(image);
                    nexrad_radial_close(radial);

                    return 0;
                }

                case NEXRAD_PACKET_RASTER_BA0F:
                case NEXRAD_PACKET_RASTER_BA07: {
                    nexrad_raster *raster = nexrad_raster_packet_open((nexrad_raster_packet *)packet);

                    nexrad_raster_image *image = nexrad_raster_create_image(raster,
                        NEXRAD_RASTER_IMAGE_8BPP, NEXRAD_RASTER_IMAGE_GRAYSCALE
                    );

                    fprintf(stderr, "Found raster packet, creating image %s\n", outfile);

                    if (nexrad_raster_image_save_png(image, outfile) < 0) {
                        return -1;
                    }

                    nexrad_raster_image_destroy(image);
                    nexrad_raster_close(raster);

                    return 0;
                }

                default: {
                    fprintf(stderr, "Got packet type %d\n", type);

                    break;
                }
            }

            nexrad_symbology_layer_next_packet(layer, size);
        }
    }

    return -1;

error_symbology_block_open:
error_message_get_symbology_block:
    nexrad_message_close(message);

error_message_open:
    return -1;
}

int main(int argc, char **argv) {
    char *infile, *outfile;

    if (argc != 3) {
        usage(argc, argv);
    }

     infile = argv[1];
    outfile = argv[2];

    if (save_product_image(infile, outfile) < 0) {
        perror("save_product_image()");
        exit(1);
    }

    return 0;
}
