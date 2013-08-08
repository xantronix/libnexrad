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
    nexrad_chunk *chunk;
    size_t size;

    if (argc != 2) {
        usage(argc, argv);
    }

    if ((message = nexrad_message_open(argv[1])) == NULL) {
        perror("nexrad_message_open()");
        exit(1);
    }

    if ((chunk = nexrad_chunk_open(message->symbology, NEXRAD_CHUNK_SYMBOLOGY_BLOCK)) == NULL) {
        perror("nexrad_chunk_open()");
        exit(1);
    }

    while (nexrad_chunk_read(chunk, &size, NULL)) {
        fprintf(stderr, "Got a chunk!\n");
    }

    nexrad_chunk_close(chunk);
    nexrad_message_close(message);

    return 0;
}
