/*
 * Copyright (c) 2016 Dynamic Weather Solutions, Inc. Distributed under the
 * terms of the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nexrad/message.h>
#include <nexrad/raster.h>
#include <nexrad/radial.h>
#include <nexrad/map.h>

static void usage(int argc, char **argv) {
    fprintf(stderr, "usage: %s --reflectivity|--velocity input.l3 output.png\n", argv[0]);
    exit(1);
}

static nexrad_image *get_product_image(const char *file, nexrad_color *colors) {
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
                case NEXRAD_PACKET_RADIAL:
                case NEXRAD_PACKET_RADIAL_AF1F: {
                    nexrad_radial *radial = nexrad_radial_packet_unpack((nexrad_radial_packet *)packet,
                        NULL, nexrad_chunk_bytes_left(layer));

                    nexrad_map_point radar = {
                        .lat =  25.5458,
                        .lon = -97.2508
                    };

                    nexrad_map_point extents[4];

                    float factor = nexrad_map_range_factor(0.5,
                        NEXRAD_MAP_EARTH_REFRACTION);

                    nexrad_image *image = nexrad_map_project_radial(radial,
                        &radar, extents, colors, factor, 1000.0, 10);

                    return image;
                }

                case NEXRAD_PACKET_RASTER_BA0F:
                case NEXRAD_PACKET_RASTER_BA07: {
                    nexrad_raster *raster = nexrad_raster_packet_open((nexrad_raster_packet *)packet);

                    nexrad_image *image = nexrad_raster_create_image(raster, colors);

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

static nexrad_color *load_colors(char *name) {
    if (strcmp(name, "--reflectivity") == 0) {
        return nexrad_color_create_table(NEXRAD_COLOR_TABLE_REFLECTIVITY);
    } else if (strcmp(name, "--velocity") == 0) {
        return nexrad_color_create_table(NEXRAD_COLOR_TABLE_VELOCITY);
    } else {
        fprintf(stderr, "Invalid color table '%s'\n", name);

        exit(1);
    }
}

int main(int argc, char **argv) {
    char *infile, *outfile;
    nexrad_image *image;
    nexrad_color *colors;

    if (argc != 4) {
        usage(argc, argv);
    }

     infile = argv[2];
    outfile = argv[3];

    colors = load_colors(argv[1]);

    if ((image = get_product_image(infile, colors)) == NULL) {
        perror("get_product_image()");
        exit(1);
    }

    if (nexrad_image_save_png(image, outfile) < 0) {
        perror("nexrad_image_save_png()");
        exit(1);
    }

    return 0;
}
