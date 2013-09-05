#include <stdlib.h>
#include <endian.h>

#include <nexrad/packet.h>

enum nexrad_packet_type_id nexrad_packet_type(nexrad_packet *packet) {
    if (packet == NULL) return 0;

    return be16toh(packet->type);
}
