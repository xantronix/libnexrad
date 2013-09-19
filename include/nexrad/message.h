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

typedef struct _nexrad_message nexrad_message;

/*
 * Methods for facilitating file I/O
 */
nexrad_message * nexrad_message_open(const char *path);
void             nexrad_message_close(nexrad_message *message);

time_t nexrad_message_get_scan_timestamp(nexrad_message *message);
time_t nexrad_message_get_gen_timestamp(nexrad_message *message);
int    nexrad_message_get_region(nexrad_message *message, char *dest, size_t destlen);
int    nexrad_message_get_office(nexrad_message *message, char *dest, size_t destlen);
int    nexrad_message_get_station(nexrad_message *message, char *dest, size_t destlen);
int    nexrad_message_get_product_code(nexrad_message *message, char *dest, size_t destlen);
int    nexrad_message_get_product_id(nexrad_message *message);
int    nexrad_message_get_site_coords(nexrad_message *message, double *lat, double *lon);

#endif /* _NEXRAD_MESSAGE_H */
