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

int main(int argc, char **argv) {
    nexrad_message *message;

    if (argc != 2) {
        usage(argc, argv);
    }

    if ((message = nexrad_message_open(argv[1])) == NULL) {
        perror("nexrad_message_open()");
        exit(1);
    }

    nexrad_chunk *block;
    nexrad_chunk *layer;

    if ((block = nexrad_symbology_block_open(message)) == NULL) {
        perror("nexrad_chunk_open()");
        exit(1);
    }

    fprintf(stderr, "Opened symbology block, %lu bytes left to read\n", block->bytes_left);

    while ((layer = nexrad_symbology_block_read_layer(block)) != NULL) {
        nexrad_packet * packet;

        fprintf(stderr, "Opened symbology layer, %lu bytes left to read\n", layer->bytes_left);

        size_t size;
        void *data;

        while ((packet = nexrad_symbology_layer_read_packet(layer, NULL, &size, &data)) != NULL) {
            fprintf(stderr, "Read %lu byte packet\n", size);
            write(1, data, size);
        }

        nexrad_symbology_layer_close(layer);
    }

    nexrad_symbology_block_close(block);

    nexrad_message_close(message);

    return 0;
}
