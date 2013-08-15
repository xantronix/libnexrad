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

static void show_graphic_block(nexrad_message *message) {
    nexrad_chunk *block;
    nexrad_chunk *page;

    if ((block = nexrad_graphic_block_open(message)) == NULL) {
        perror("nexrad_graphic_block_open()");
        exit(1);
    }

    fprintf(stderr, "Opened graphic block, %lu bytes left to read\n", block->bytes_left);
    fprintf(stderr, "Pages in graphic block: %d\n", be16toh(message->graphic->pages));

    while ((page = nexrad_graphic_block_read_page(block)) != NULL) {
        nexrad_packet *packet;

        fprintf(stderr, "Opened graphic page, %lu bytes left to read\n", page->bytes_left);

        size_t size;
        while ((packet = nexrad_graphic_page_read_packet(page, &size, NULL, NULL)) != NULL) {
            if (nexrad_packet_type(packet) != 8) continue;

            void *data;

            data = (char *)packet + sizeof(nexrad_text_packet);
            size -= sizeof(nexrad_text_packet);

            write(1, data, size);
            write(1, "\n", 1);
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
    char *buf;
    int line;
    int page;

    while ((len = nexrad_tabular_block_read_line(block, &buf, &page, &line)) > 0) {
        fprintf(stderr, "Read %lu bytes from page %d, line %d\n", len, page, line);
        write(1, buf, len);
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

    show_graphic_block(message);

    nexrad_message_close(message);

    return 0;
}
