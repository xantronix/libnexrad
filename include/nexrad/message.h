#ifndef _NEXRAD_MESSAGE_H
#define _NEXRAD_MESSAGE_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/header.h>
#include <nexrad/chunk.h>
#include <nexrad/product.h>
#include <nexrad/symbology.h>
#include <nexrad/graphic.h>
#include <nexrad/tabular.h>
#include <nexrad/packet.h>

#define NEXRAD_MESSAGE_MAX_BODY_SIZE  8388608
#define NEXRAD_MESSAGE_MAX_SIZE     104857600

typedef struct _nexrad_message nexrad_message;

nexrad_message *nexrad_message_open_buf(void *buf, size_t len);

nexrad_message *nexrad_message_open(const char *path);

void nexrad_message_destroy(nexrad_message *message);

void nexrad_message_close(nexrad_message *message);

nexrad_message_header *nexrad_message_get_header(nexrad_message *message);

nexrad_product_description *nexrad_message_get_product_description(nexrad_message *message);

nexrad_symbology_block *nexrad_message_get_symbology_block(nexrad_message *message);

nexrad_graphic_block *nexrad_message_get_graphic_block(nexrad_message *message);

nexrad_tabular_block *nexrad_message_get_tabular_block(nexrad_message *message);

time_t nexrad_message_get_scan_timestamp(nexrad_message *message);

time_t nexrad_message_get_gen_timestamp(nexrad_message *message);

int nexrad_message_get_product_type(nexrad_message *message);

int nexrad_message_find_product_code(nexrad_message *message,
    char **code,
    size_t *len
);

int nexrad_message_find_region(
    nexrad_message *message,
    char **region,
    size_t *len
);

int nexrad_message_find_office(nexrad_message *message,
    char **office, size_t *len
);

char nexrad_message_get_station_prefix(nexrad_message *message);

int nexrad_message_find_station_suffix(nexrad_message *message,
    char **suffix, size_t *len
);

int nexrad_message_read_station(nexrad_message *message,
    char *station,
    size_t destlen
);

int nexrad_message_read_station_location(nexrad_message *message,
    double *lat,
    double *lon,
    double *alt
);

nexrad_packet *nexrad_message_find_symbology_packet_by_type(nexrad_message *message, enum nexrad_packet_type);

#endif /* _NEXRAD_MESSAGE_H */
