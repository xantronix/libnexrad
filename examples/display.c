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
#include <errno.h>
#include <unistd.h>
#include "../src/util.h"

#include <nexrad/message.h>
#include <nexrad/radial.h>
#include <nexrad/raster.h>

static void usage(int argc, char **argv) {
    fprintf(stderr, "usage: %s sn.nnnn.ssss\n", argv[0]);

    exit(1);
}

static void show_radial_packet(nexrad_radial_packet *packet, size_t *size) {
    nexrad_radial *radial;
    nexrad_radial_ray *ray;

    if ((radial = nexrad_radial_packet_open(packet)) == NULL) {
        perror("nexrad_radial_packet_open()");
        exit(1);
    }

    printf("Huzzah, got a radial type %d packet with %d rangebin offset, %d rangebins/ray, %d rays\n",
        (int)be16toh(packet->type),
        (int)be16toh(packet->rangebin_first),
        (int)be16toh(packet->rangebin_count),
        (int)be16toh(packet->rays)
    );

    printf("Radial has scale %d with %d, %d offset\n",
        (int)be16toh(packet->scale),
        (int)be16toh(packet->i),
        (int)be16toh(packet->j)
    );

    while ((ray = nexrad_radial_read_ray(radial, NULL)) != NULL) {
        printf("Wee, got a ray with size %hu start %hu delta %hu\n",
            be16toh(ray->size), be16toh(ray->angle_start), be16toh(ray->angle_delta)
        );
    }

    *size = nexrad_radial_bytes_read(radial);

    printf("Done reading radial of %lu bytes\n", *size);

    nexrad_radial_close(radial);
}

static void show_raster_packet(nexrad_raster_packet *packet, size_t *size) {
    nexrad_raster *raster;
    nexrad_raster_line *line;
    uint16_t line_size;
    uint16_t width, height;

    if ((raster = nexrad_raster_packet_open((nexrad_raster_packet *)packet)) == NULL) {
        perror("nexrad_raster_packet_open()");
        exit(1);
    }

    if (nexrad_raster_get_info(raster, &width, &height) < 0) {
        perror("nexrad_raster_get_info()");
        exit(1);
    }

    printf("Huzzah, got a raster, %hu%hu\n", width, height);

    while ((line = nexrad_raster_read_line(raster, NULL, &line_size)) != NULL) {
        printf("Wee, got a line sized %hu bytes!\n", line_size);
    }

    *size = nexrad_raster_bytes_read(raster);

    printf("Done reading raster of %lu bytes\n", *size);

    nexrad_raster_close(raster);
}

static void show_packet(nexrad_packet *packet, size_t *size) {
    enum nexrad_packet_type type = nexrad_packet_get_type(packet);

    switch (type) {
        case NEXRAD_PACKET_RADIAL:
        case NEXRAD_PACKET_RADIAL_AF1F: {
            show_radial_packet((nexrad_radial_packet *)packet, size);

            break;
        }

        case NEXRAD_PACKET_RASTER_BA0F:
        case NEXRAD_PACKET_RASTER_BA07: {
            show_raster_packet((nexrad_raster_packet *)packet, size);

            break;
        }

        case NEXRAD_PACKET_HAIL: {
            int i, j, probability, probability_severe, max_size;

            nexrad_packet_read_hail_data(packet,
                &i, &j, &probability, &probability_severe, &max_size
            );

            printf("Hail %4d,%4d offset from radar, %d/%d probability/severe, %d max hail size\n",
                i, j, probability, probability_severe, max_size
            );

            break;
        }

        case NEXRAD_PACKET_CELL: {
            int i, j;
            char id[3];

            nexrad_packet_read_cell_data(packet, &i, &j, id, 3);

            printf("Storm cell %2s %4d,%4d offset from radar\n",
                id, i, j
            );

            break;
        }

        default: {
            printf("Read symbology packet type %d\n", type);

            break;
        }
    }
}

static void show_symbology_block(nexrad_message *message) {
    nexrad_symbology_block *symbology;
    nexrad_chunk *block;
    nexrad_chunk *layer;

    if ((symbology = nexrad_message_get_symbology_block(message)) == NULL) {
        return;
    }

    if ((block = nexrad_symbology_block_open(symbology)) == NULL) {
        perror("nexrad_symbology_block_open()");
        exit(1);
    }

    while ((layer = nexrad_symbology_block_read_layer(block)) != NULL) {
        nexrad_packet *packet;
        size_t size;

        while ((packet = nexrad_symbology_layer_peek_packet(layer, &size)) != NULL) {
            show_packet(packet, &size);

            nexrad_symbology_layer_next_packet(layer, size);
        }

        nexrad_symbology_layer_close(layer);
    }

    nexrad_symbology_block_close(block);
}

static void show_graphic_block(nexrad_message *message) {
    nexrad_graphic_block *graphic;
    nexrad_chunk *block;
    nexrad_chunk *page;

    if ((graphic = nexrad_message_get_graphic_block(message)) == NULL) {
        return;
    }

    if ((block = nexrad_graphic_block_open(graphic)) == NULL) {
        perror("nexrad_graphic_block_open()");
        exit(1);
    }

    while ((page = nexrad_graphic_block_read_page(block)) != NULL) {
        nexrad_packet *packet;
        size_t size;

        while ((packet = nexrad_graphic_page_read_packet(page, &size)) != NULL) {
            enum nexrad_packet_type type = nexrad_packet_get_type(packet);

            switch (type) {
                case NEXRAD_PACKET_TEXT: {
                    int i, j, color;
                    char text[81];
                    size_t len;

                    nexrad_packet_read_text_data(packet,
                        &i, &j, &color, text, &len, 81
                    );

                    printf("Read text packet with color %02x, position %d,%d\n",
                        color, i, j
                    );

                    fwrite(text, len, 1, stdout);
                    printf("\n");

                    break;
                }

                case NEXRAD_PACKET_VECTOR: {
                    nexrad_vector vector;
                    int magnitude;

                    nexrad_packet_read_vector_data(packet, &magnitude, &vector);

                    printf("Read vector packet with magnitude %u, starting %d,%d, ending %d,%d -> starting %d,%d ending %d,%d\n",
                        magnitude,
                        vector.i1_start, vector.j1_start,
                        vector.i1_end,   vector.i1_end,
                        vector.i2_start, vector.j2_start,
                        vector.i2_end,   vector.j2_end
                    );

                    break;
                } 
                default: {
                    printf("Read graphic packet type %d\n", type);

                    break;
                }
            }
        }

        nexrad_graphic_page_close(page);
    }

    nexrad_graphic_block_close(block);
}

static void show_tabular_block(nexrad_message *message) {
    nexrad_tabular_block *tabular;
    nexrad_tabular_text *block;

    if ((tabular = nexrad_message_get_tabular_block(message)) == NULL) {
        return;
    }

    if ((block = nexrad_tabular_block_open(tabular)) == NULL) {
        perror("nexrad_tabular_block_open()");
        exit(1);
    }

    size_t len;
    char *tmp;
    int line;
    int page;

    while ((len = nexrad_tabular_block_read_line(block, &tmp, &page, &line)) > 0) {
        char buf[8];

        snprintf(buf, 8, "%2d/%2d: ", page, line);
        printf("%s", buf);

        fwrite(tmp, len, 1, stdout);
        printf("\n");
    }

    nexrad_tabular_block_close(block);
}

int main(int argc, char **argv) {
    int ret = 0;
    nexrad_message *message;
    double lat, lon;

    if (argc != 2) {
        usage(argc, argv);
    }

    if ((message = nexrad_message_open(argv[1])) == NULL) {
        perror("nexrad_message_open()");
        exit(1);
    }

    nexrad_message_read_station_location(message, &lat, &lon, NULL);

    printf("Radar lat/lon: %f, %f\n", lat, lon);

    show_symbology_block(message);
    show_graphic_block(message);
    show_tabular_block(message);

    nexrad_message_close(message);

    return ret;
}
