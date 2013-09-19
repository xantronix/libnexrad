#include <stdlib.h>
#include <endian.h>

#include <nexrad/packet.h>

enum nexrad_packet_type_id nexrad_packet_type(nexrad_packet *packet) {
    if (packet == NULL) return 0;

    return be16toh(packet->type);
}

int nexrad_packet_read_hail_data(nexrad_packet *packet, int *i, int *j, int *probability, int *probability_severe, int *max_size) {
    nexrad_hail_packet *hail;

    if (packet == NULL || nexrad_packet_type(packet) != NEXRAD_PACKET_TYPE_HAIL) {
        return -1;
    }

    hail = (nexrad_hail_packet *)packet;

    if (i)
        *i = be16toh(hail->i);

    if (j)
        *j = be16toh(hail->j);

    if (probability)
        *probability = be16toh(hail->probability);

    if (probability_severe)
        *probability_severe = be16toh(hail->probability_severe);

    if (max_size)
        *max_size = be16toh(hail->max_size);

    return 0;
}
