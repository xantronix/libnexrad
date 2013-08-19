#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <endian.h>

#include <nexrad/message.h>

static void usage(int argc, char **argv) {
    fprintf(stderr, "usage: %s sn.nnnn.ssss\n", argv[0]);

    exit(1);
}

static void show_symbology_block(nexrad_message *message) {
    nexrad_chunk *block;
    nexrad_chunk *layer;

    if ((block = nexrad_symbology_block_open(message)) == NULL) {
        perror("nexrad_symbology_block_open()");
        exit(1);
    }

    while ((layer = nexrad_symbology_block_read_layer(block)) != NULL) {
        nexrad_packet *packet;
        size_t size;

        while ((packet = nexrad_symbology_layer_read_packet(layer, &size)) != NULL) {
            enum nexrad_packet_type_id type = nexrad_packet_type(packet);

            switch (type) {
                case NEXRAD_PACKET_TYPE_HAIL: {
                    nexrad_hail_packet *hail = (nexrad_hail_packet *)packet;

                    fprintf(stderr, "Hail %4d,%4d offset from radar, %d/%d probability/severe, %d max hail size\n",
                        (int16_t)be16toh(hail->i),                  (int16_t)be16toh(hail->j),       (int16_t)be16toh(hail->probability),
                        (int16_t)be16toh(hail->probability_severe), (int16_t)be16toh(hail->max_size)
                    );

                    break;
                }

                case NEXRAD_PACKET_TYPE_CELL: {
                    nexrad_cell_packet *cell = (nexrad_cell_packet *)packet;

                    fprintf(stderr, "Storm cell %2s %4d,%4d offset from radar\n",
                        cell->id, (int16_t)be16toh(cell->i), (int16_t)be16toh(cell->j)
                    );

                    break;
                }

                default: {
                    fprintf(stderr, "Read symbology packet type %d\n", type);

                    break;
                }
            }
        }

        nexrad_symbology_layer_close(layer);
    }

    nexrad_symbology_block_close(block);
}

static void show_graphic_block(nexrad_message *message) {
    nexrad_chunk *block;
    nexrad_chunk *page;

    if ((block = nexrad_graphic_block_open(message)) == NULL) {
        perror("nexrad_graphic_block_open()");
        exit(1);
    }

    while ((page = nexrad_graphic_block_read_page(block)) != NULL) {
        nexrad_packet *packet;
        size_t size;

        while ((packet = nexrad_graphic_page_read_packet(page, &size)) != NULL) {
            enum nexrad_packet_type_id type = nexrad_packet_type(packet);

            switch (type) {
                case NEXRAD_PACKET_TYPE_TEXT: {
                    nexrad_text_packet *text = (nexrad_text_packet *)packet;

                    size_t len = size - sizeof(nexrad_text_packet);

                    fprintf(stderr, "Read text packet with color %02x, position %d,%d\n",
                        be16toh(text->color), (int16_t)be16toh(text->i), (int16_t)be16toh(text->j)
                    );

                    write(1, (char *)packet + sizeof(nexrad_text_packet), len);
                    write(1, "\n", 1);

                    break;
                }

                case NEXRAD_PACKET_TYPE_VECTOR: {
                    nexrad_vector_packet *vector = (nexrad_vector_packet *)packet;

                    fprintf(stderr, "Read vector packet with magnitude %u, starting %d,%d, ending %d,%d -> starting %d,%d ending %d,%d\n",
                        be16toh(vector->magnitude),
                        (int16_t)be16toh(vector->i1_start), (int16_t)be16toh(vector->j1_start),
                        (int16_t)be16toh(vector->i1_end),   (int16_t)be16toh(vector->i1_end),
                        (int16_t)be16toh(vector->i2_start), (int16_t)be16toh(vector->j2_start),
                        (int16_t)be16toh(vector->i2_end),   (int16_t)be16toh(vector->j2_end)
                    );

                    break;
                } 
                default: {
                    fprintf(stderr, "Read graphic packet type %d\n", type);

                    break;
                }
            }
        }

        nexrad_graphic_page_close(page);
    }

    nexrad_graphic_block_close(block);
}

static void show_tabular_block(nexrad_message *message) {
    nexrad_text *block;

    if ((block = nexrad_tabular_block_open(message)) == NULL) {
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
        write(1, buf, strlen(buf));
        write(1, tmp, len);
        write(1, "\n", 1);
    }

    nexrad_tabular_block_close(block);
}

int main(int argc, char **argv) {
    nexrad_message *message;

    if (argc != 2) {
        usage(argc, argv);
    }

    if ((message = nexrad_message_open(argv[1])) == NULL) {
        perror("nexrad_message_open()");
        exit(1);
    }

    fprintf(stderr, "VCP %d, scan %d\n",
        be16toh(message->description->vcp), (int16_t)be16toh(message->description->scan)
    );

    show_symbology_block(message);
    show_graphic_block(message);
    show_tabular_block(message);

    nexrad_message_close(message);

    return 0;
}
