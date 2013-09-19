#include <stdlib.h>
#include <endian.h>
#include "util.h"

#include <nexrad/packet.h>

#pragma pack(push)
#pragma pack(1)

struct _nexrad_text_packet {
    nexrad_packet_header header;

    uint16_t color; /* 4-bit color value (0-15) */
     int16_t i;     /* Cartesian offset from radar in 1/4km increments */
     int16_t j;     /* Cartesian offset from radar in 1/4km increments */
};

struct _nexrad_cell_packet {
    nexrad_packet_header header;

     int16_t i;
     int16_t j;
    char     id[NEXRAD_PACKET_CELL_ID_LEN];
};

struct _nexrad_hail_packet {
    nexrad_packet_header header;

     int16_t i;                  /* Cartesian offset from radar */
     int16_t j;                  /* Cartesian offset from radar */
     int16_t probability;        /* Probability of any hail */
     int16_t probability_severe; /* Probability of severe hail */
    uint16_t max_size;           /* Maximum size of hail */
};

struct _nexrad_vector_packet {
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
};

#pragma pack(pop)

enum nexrad_packet_type_id nexrad_packet_type(nexrad_packet *packet) {
    if (packet == NULL) return 0;

    return be16toh(packet->type);
}

int nexrad_packet_read_text_data(nexrad_packet *packet, int *i, int *j, int *color, char *data, size_t *textlen, size_t destlen) {
    nexrad_text_packet *text;

    if (packet == NULL || nexrad_packet_type(packet) != NEXRAD_PACKET_TYPE_TEXT) {
        return -1;
    }

    text = (nexrad_text_packet *)packet;

    if (i)
        *i = (int16_t)be16toh(text->i);

    if (j)
        *j = (int16_t)be16toh(text->j);

    if (color)
        *color = be16toh(text->color);

    if (data && textlen) {
        size_t len = be16toh(text->header.size) - sizeof(nexrad_text_packet);

        if (safecpy(data, (char *)text + sizeof(nexrad_text_packet), destlen, len) < 0) {
            goto error_safecpy;
        }

        *textlen = len;
    }

    return 0;

error_safecpy:
    return -1;
}

int nexrad_packet_read_cell_data(nexrad_packet *packet, int *i, int *j, char *id, size_t destlen) {
    nexrad_cell_packet *cell;

    if (packet == NULL) {
        return -1;
    }

    cell = (nexrad_cell_packet *)packet;

    if (i)
        *i = (int16_t)be16toh(cell->i);

    if (j)
        *j = (int16_t)be16toh(cell->j);

    if (id) {
        safecpy(id, cell->id, destlen, sizeof(cell->id));
    }

    return 0;
}

int nexrad_packet_read_hail_data(nexrad_packet *packet, int *i, int *j, int *probability, int *probability_severe, int *max_size) {
    nexrad_hail_packet *hail;

    if (packet == NULL || nexrad_packet_type(packet) != NEXRAD_PACKET_TYPE_HAIL) {
        return -1;
    }

    hail = (nexrad_hail_packet *)packet;

    if (i)
        *i = (int16_t)be16toh(hail->i);

    if (j)
        *j = (int16_t)be16toh(hail->j);

    if (probability)
        *probability = (int16_t)be16toh(hail->probability);

    if (probability_severe)
        *probability_severe = (int16_t)be16toh(hail->probability_severe);

    if (max_size)
        *max_size = be16toh(hail->max_size);

    return 0;
}

int nexrad_packet_read_vector_data(nexrad_packet *packet, int *magnitude, nexrad_vector *vector) {
    nexrad_vector_packet *data;

    if (packet == NULL) {
        return -1;
    }

    data = (nexrad_vector_packet *)packet;

    if (magnitude)
        *magnitude = be16toh(vector->magnitude);

    if (vector) {
        vector->i1_start = (int16_t)be16toh(data->i1_start);
        vector->j1_start = (int16_t)be16toh(data->j1_start);
        vector->i1_end   = (int16_t)be16toh(data->i1_end);
        vector->j1_end   = (int16_t)be16toh(data->j1_end);
        vector->i2_start = (int16_t)be16toh(data->i2_start);
        vector->j2_start = (int16_t)be16toh(data->j2_start);
        vector->i2_end   = (int16_t)be16toh(data->i2_end);
        vector->j2_end   = (int16_t)be16toh(data->j2_end);
    }

    return 0;
}
