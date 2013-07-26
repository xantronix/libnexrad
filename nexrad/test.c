#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

    printf("sizeof(nexrad_file_header):         %lu\n", sizeof(nexrad_file_header));
    printf("sizeof(nexrad_message_header):      %lu\n", sizeof(nexrad_message_header));
    printf("sizeof(nexrad_product_description): %lu\n", sizeof(nexrad_product_description));
    printf("sizeof(nexrad_symbology_block):     %lu\n", sizeof(nexrad_symbology_block));
    printf("sizeof(nexrad_graphic_block):       %lu\n", sizeof(nexrad_graphic_block));
    printf("sizeof(nexrad_tabular_block):       %lu\n", sizeof(nexrad_tabular_block));

    if ((message = nexrad_message_open(argv[1])) == NULL) {
        perror("nexrad_message_open()");
        exit(1);
    }

    printf("Offset to message header:      0x%lx\n", (void *)(message->message_header) - (void *)(message->data));
    printf("Offset to product description: 0x%lx\n", (void *)(message->description)    - (void *)(message->data));
    printf("Offset to symbology block:     0x%lx\n", (void *)(message->symbology)      - (void *)(message->data));
    printf("Offset to graphic block:       0x%lx\n", (void *)(message->graphic)        - (void *)(message->data));
    printf("Offset to tabular block:       0x%lx\n", (void *)(message->tabular)        - (void *)(message->data));

    printf("Crap: %d\n", be16toh(message->tabular->message_header.type));

    nexrad_message_close(message);

    return 0;
}
