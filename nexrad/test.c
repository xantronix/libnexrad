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

        size_t total_size, data_size;
        void *data;

        while ((packet = nexrad_symbology_layer_read_packet(layer, &total_size, &data_size, &data)) != NULL) {
            enum nexrad_packet_type_id type = nexrad_packet_type(packet);

            fprintf(stderr, "Read packet type %d, total size %u, payload %u\n",
                type, total_size, data_size
            );

            if (type != NEXRAD_PACKET_TYPE_HAIL) {
                continue;
            }

            nexrad_hail_packet *hail = packet;

            fprintf(stderr, "Hail %6d,%6d offset from radar, %u/%u probability/severe, %u max hail size\n",
                hail->i, hail->j, hail->probability, hail->probability_severe, hail->max_size
            );
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

        size_t total_size, data_size;
        void *data;

        while ((packet = nexrad_graphic_page_read_packet(page, &total_size, &data_size, &data)) != NULL) {
            enum nexrad_packet_type_id type = nexrad_packet_type(packet);

            fprintf(stderr, "Read packet type %d, total size %u, payload %u\n",
                type, total_size, data_size
            );
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

    fprintf(stderr, "Pages in tabular block: %d\n", be16toh(message->tabular->pages));

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

    show_symbology_block(message);
    show_graphic_block(message);
    show_tabular_block(message);

    nexrad_message_close(message);

    return 0;
}
