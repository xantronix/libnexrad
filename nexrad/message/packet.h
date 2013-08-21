#ifndef _NEXRAD_MESSAGE_PACKET_H
#define _NEXRAD_MESSAGE_PACKET_H

#include <stdint.h>

#pragma pack(push)
#pragma pack(1)

enum nexrad_packet_type_id {
    NEXRAD_PACKET_TYPE_UNKNOWN =  0,
    NEXRAD_PACKET_TYPE_TEXT    =  8,
    NEXRAD_PACKET_TYPE_VECTOR  = 10,
    NEXRAD_PACKET_TYPE_CELL    = 15,
    NEXRAD_PACKET_TYPE_HAIL    = 19
};

typedef struct _nexrad_packet_header {
    uint16_t type;
    uint16_t size;
} nexrad_packet_header;

typedef nexrad_packet_header nexrad_packet;

typedef struct _nexrad_text_packet {
    nexrad_packet_header header;

    uint16_t color; /* 4-bit color value (0-15) */
     int16_t i;     /* Cartesian offset from radar in 1/4km increments */
     int16_t j;     /* Cartesian offset from radar in 1/4km increments */
} nexrad_text_packet;

typedef struct _nexrad_hail_packet {
    nexrad_packet_header header;

     int16_t i;                  /* Cartesian offset from radar */
     int16_t j;                  /* Cartesian offset from radar */
    uint16_t probability;        /* Probability of hail */
    uint16_t probability_severe; /* Probability of severe hail */
    uint16_t max_size;           /* Maximum size of hail */
} nexrad_hail_packet;

typedef struct _nexrad_vector_packet {
    nexrad_packet_header header;

    uint16_t magnitude; /* Vector magnitude in 1/4km increments */
     int16_t i1_start;  /* Cartesian origin vector */
     int16_t j1_start;  /* Cartesian origin vector */
     int16_t i1_end;    /* Cartesian origin vector */
     int16_t j1_end;    /* Cartesian origin vector */
     int16_t i2_start;  /* Cartesian destination vector */
     int16_t j2_start;  /* Cartesian destination vector */
     int16_t i2_end;    /* Cartesian destination vector */
     int16_t j2_end;    /* Cartesian destination vector */
} nexrad_vector_packet;

typedef struct _nexrad_cell_packet {
    nexrad_packet_header header;

     int16_t i;
     int16_t j;
    char     id[2];
} nexrad_cell_packet;

#pragma pack(pop)

enum nexrad_packet_type_id nexrad_packet_type(nexrad_packet *packet);

#endif /* _NEXRAD_MESSAGE_PACKET_H */
