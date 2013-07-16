#ifndef _NEXRAD_PACKET_HEADER_H
#define _NEXRAD_PACKET_HEADER_H

#include <stdint.h>

typedef struct _nexrad_packet_header {
    uint16_t code;
    uint16_t size;
} nexrad_packet_header;

#endif /* _NEXRAD_PACKET_HEADER_H */
