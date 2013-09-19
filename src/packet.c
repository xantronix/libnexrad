#include <stdlib.h>
#include <endian.h>
#include "util.h"

#include <nexrad/packet.h>

enum nexrad_packet_type_id nexrad_packet_type(nexrad_packet *packet) {
    if (packet == NULL) return 0;

    return be16toh(packet->type);
}

int nexrad_packet_find_text_data(nexrad_packet *packet, int *i, int *j, int *color, char **data, size_t *textlen) {
    nexrad_text_packet *text;

    if (packet == NULL) {
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
        *data    = (char *)text + sizeof(nexrad_text_packet);
        *textlen = be16toh(text->header.size) - sizeof(nexrad_text_packet);
    }

    return 0;
}

int nexrad_packet_read_text_data(nexrad_packet *packet, int *i, int *j, int *color, char *data, size_t *textlen, size_t destlen) {
    char *text;

    if (nexrad_packet_find_text_data(packet, i, j, color, &text, textlen) < 0) {
        return -1;
    }

    if (safecpy(data, text, destlen, *textlen) < 0) {
        goto error_safecpy;
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
