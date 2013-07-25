#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <nexrad/message.h>

#define DEBUG(s) fprintf(stderr, "[DEBUG] %s\n", s)

static inline int _mapped_size(size_t size, size_t page_size) {
    return size + (page_size - (size % page_size));
}

static int _index_message(nexrad_message *message) {
    nexrad_file_header *file_header = message->data;

    nexrad_message_header *      message_header;
    nexrad_product_description * description;
    nexrad_symbology_block *     symbology;
    nexrad_graphic_block *       graphic;
    nexrad_tabular_block *       tabular;

    if (file_header->_whitespace1 != ' ') {
        DEBUG("Here!");
        goto error_invalid_file_header;
    }

    message_header = (nexrad_message_header *)(file_header + sizeof(nexrad_file_header));

    if (message_header->blocks > 5) {
        fprintf(stderr, "MAGIKARP: %lu\n", message_header->blocks);
        goto error_invalid_message_header;
    }

    description = (nexrad_product_description *)(message_header + sizeof(nexrad_message_header));
    symbology   = (nexrad_symbology_block *)    (description    + sizeof(nexrad_product_description));
    graphic     = (nexrad_graphic_block *)      (symbology      + sizeof(nexrad_symbology_block));
    tabular     = (nexrad_tabular_block *)      (graphic        + sizeof(nexrad_graphic_block));

    message->file_header    = file_header;
    message->message_header = message_header;
    message->description    = description;
    message->symbology      = symbology;
    message->graphic        = graphic;
    message->tabular        = tabular;

    return 0;

error_invalid_message_header:
error_invalid_file_header:
    return -1;
}

nexrad_message *nexrad_message_open(const char *path) {
    nexrad_message *message;
    struct stat st;

    if ((message = malloc(sizeof(nexrad_message))) == NULL) {
        DEBUG("No, really, THIS time, I mean it...HERE!");
        goto error_malloc;
    }

    if (stat(path, &st) < 0) {
        DEBUG("Poop.  Where?  Here?");
        goto error_stat;
    }

    message->size        = st.st_size;
    message->page_size   = (size_t)sysconf(_SC_PAGESIZE);
    message->mapped_size = _mapped_size(st.st_size, message->page_size);

    if ((message->fd = open(path, O_RDONLY)) < 0) {
        DEBUG("Poo and goo");
        goto error_open;
    }

    if ((message->data = mmap(NULL, message->mapped_size, PROT_READ, MAP_PRIVATE, message->fd, 0)) == NULL) {
        DEBUG("Unsure @___@");
        goto error_mmap;
    }

    if (_index_message(message) < 0) {
        DEBUG("MAGIKARP @___@");
        goto error_index_message;
    }

    return message;

error_index_message:
    munmap(message->data, message->mapped_size);

error_mmap:
    close(message->fd);

error_open:
    free(message);

error_stat:
error_malloc:
    return NULL;
}

void nexrad_message_close(nexrad_message *message) {
    if (message == NULL) {
        return;
    }

    if (message->data && message->mapped_size > 0) {
        munmap(message->data, message->mapped_size);

        message->data        = 0;
        message->mapped_size = 0;
    }

    if (message->fd) {
        close(message->fd);

        message->fd = 0;
    }

    message->size           = 0;
    message->page_size      = 0;
    message->file_header    = NULL;
    message->message_header = NULL;
    message->description    = NULL;
    message->symbology      = NULL;
    message->graphic        = NULL;
    message->tabular        = NULL;

    free(message);

    return;
}
