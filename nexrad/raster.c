#include <stdlib.h>
#include <endian.h>

#include <nexrad/raster.h>

static int _valid_packet(nexrad_raster_packet *packet) {
    if (packet == NULL) {
        return 0;
    }

    switch (be16toh(packet->type)) {
        case 0xba0f:
        case 0xba07: {
            break;
        }

        default: {
            return 0;
        }
    }

    if (
               be16toh(packet->flags_1) != 0x8000 ||
               be16toh(packet->flags_2) != 0x00c0 ||
      (int16_t)be16toh(packet->i)        >   2047 ||
      (int16_t)be16toh(packet->i)        <  -2048 ||
      (int16_t)be16toh(packet->j)        >   2047 ||
      (int16_t)be16toh(packet->j)        <  -2048 ||
               be16toh(packet->x_scale)  <      1 ||
               be16toh(packet->x_scale)  >     67 ||
               be16toh(packet->y_scale)  <      1 ||
               be16toh(packet->y_scale)  <     67 ||
               be16toh(packet->lines)    <      1 ||
               be16toh(packet->lines)    >     67
    ) {
        return 0;
    }

    return 1;
}

nexrad_raster *nexrad_raster_packet_open(nexrad_raster_packet *packet) {
    nexrad_raster *raster;

    if (!_valid_packet(packet)) {
        return NULL;
    }

    if ((raster = malloc(sizeof(nexrad_raster))) == NULL) {
        goto error_malloc;
    }

    raster->packet     = packet;
    raster->bytes_read = sizeof(nexrad_raster_packet);
    raster->lines_left = be16toh(packet->lines);
    raster->current    = (nexrad_raster_line *)((char *)packet + sizeof(nexrad_raster_packet));

    return raster;

error_malloc:
    return NULL;
}

size_t nexrad_raster_line_size(nexrad_raster_line *line) {
    if (line == NULL) {
        return 0;
    }

    return sizeof(nexrad_raster_line) + be16toh(line->runs);
}

nexrad_raster_line *nexrad_raster_read_line(nexrad_raster *raster, size_t *sizep, nexrad_raster_run **runs) {
    nexrad_raster_line *line;
    size_t size;

    if (raster == NULL) {
        return NULL;
    }

    if (raster->lines_left == 0) {
        return NULL;
    }

    line = raster->current;
    size = nexrad_raster_line_size(line);

    /*
     * Advance the current line pointer beyond the line to follow.
     */
    raster->current = (nexrad_raster_line *)((char *)line + size);

    /*
     * Increase the number of bytes read in the current raster packet.
     */
    raster->bytes_read += size;

    /*
     * Decrement the number of lines left in the current raster packet.
     */
    raster->lines_left--;

    /*
     * If the caller provided a pointer to an address to store the resultant
     * raster line size, then populate that address with that value.
     */
    if (sizep != NULL) {
        *sizep = size;
    }

    /*
     * If the caller provided a pointer to an address to store a pointer to
     * the raster line's RLE-encoded runs, then provide that address.
     */
    if (runs != NULL) {
        *runs = (nexrad_raster_run *)((char *)line + sizeof(nexrad_raster_line));
    }

    return line;
}

size_t nexrad_raster_bytes_read(nexrad_raster *raster) {
    if (raster == NULL) {
        return 0;
    }

    return raster->bytes_read;
}

void nexrad_raster_close(nexrad_raster *raster) {
    if (raster == NULL) {
        return;
    }

    raster->packet     = NULL;
    raster->bytes_read = 0;
    raster->lines_left = 0;
    raster->current    = NULL;

    free(raster);
}
