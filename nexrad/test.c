#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

    nexrad_chunk *           symbology_reader;
    nexrad_symbology_layer * layer;

    if ((symbology_reader = nexrad_chunk_open(message->symbology, NEXRAD_CHUNK_SYMBOLOGY_BLOCK)) == NULL) {
        perror("nexrad_chunk_open()");
        exit(1);
    }

    while ((layer = nexrad_chunk_read(symbology_reader, NULL, NULL)) != NULL) {
        nexrad_chunk *  layer_reader;
        nexrad_packet * packet;

        if ((layer_reader = nexrad_chunk_open(layer, NEXRAD_CHUNK_SYMBOLOGY_LAYER)) == NULL) {
            perror("nexrad_chunk_open()");
            exit(1);
        }

        while ((packet = nexrad_chunk_read(layer_reader, NULL, NULL)) != NULL) {
            printf("Wee, read a packet!\n");
        }

        nexrad_chunk_close(layer_reader);
    }

    nexrad_chunk_close(symbology_reader);

    nexrad_message_close(message);

    return 0;
}
