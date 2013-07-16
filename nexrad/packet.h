#ifndef _NEXRAD_PACKET_H
#define _NEXRAD_PACKET_H

#include <stdint.h>

#define NEXRAD_PACKET_TYPE_ALPHA   8
#define NEXRAD_PACKET_TYPE_VECTOR 10
#define NEXRAD_PACKET_TYPE_HAIL   19

typedef struct _nexrad_packet_header {
    uint16_t code;
    uint16_t size;
} nexrad_packet_header;

#endif /* _NEXRAD_PACKET_H */
