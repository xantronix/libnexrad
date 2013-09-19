#include <stdlib.h>
#include <endian.h>

#include <nexrad/radial.h>

struct _nexrad_radial {
    nexrad_radial_packet *     packet;
    enum nexrad_radial_type_id type;
    size_t                     bytes_read;
    size_t                     rays_left;
    nexrad_radial_ray *        current;
};

static int _valid_packet(nexrad_radial_packet *packet, enum nexrad_radial_type_id type) {
    switch (type) {
        case NEXRAD_RADIAL_RLE:
        case NEXRAD_RADIAL_DIGITAL: {
            break;
        }

        default: {
            return 0;
        }
    }

    if (
      be16toh(packet->rangebin_first) >   460 ||
      be16toh(packet->rangebin_count) >   460 ||
      be16toh(packet->scale)          >  8000 ||
      be16toh(packet->scale)          <     1 ||
      be16toh(packet->rays)           >   400
    ) {
        return 0;
    }

    return 1;
}

nexrad_radial *nexrad_radial_packet_open(nexrad_radial_packet *packet) {
    nexrad_radial *radial;
    enum nexrad_radial_type_id type;

    if (packet == NULL) {
        return NULL;
    }

    type = be16toh(packet->type);

    if (!_valid_packet(packet, type)) {
        return NULL;
    }

    if ((radial = malloc(sizeof(*radial))) == NULL) {
        goto error_malloc;
    }

    radial->packet     = packet;
    radial->type       = type;
    radial->bytes_read = sizeof(nexrad_radial_packet);
    radial->rays_left  = be16toh(packet->rays);
    radial->current    = (nexrad_radial_ray *)((char *)packet + sizeof(nexrad_radial_packet));

    return radial;

error_malloc:
    return NULL;
}

ssize_t nexrad_radial_ray_size(nexrad_radial_ray *ray, enum nexrad_radial_type_id type) {
    if (ray == NULL) {
        return -1;
    }

    switch (type) {
        case NEXRAD_RADIAL_RLE: {
            return sizeof(nexrad_radial_ray) + be16toh(ray->size) * 2;
        }

        case NEXRAD_RADIAL_DIGITAL: {
            return sizeof(nexrad_radial_ray) + be16toh(ray->size);
        }
    }

    return -1;
}

nexrad_radial_ray *nexrad_radial_read_ray(nexrad_radial *radial, size_t *sizep, void **runs) {
    nexrad_radial_ray *ray;
    size_t size;

    if (radial == NULL) {
        return NULL;
    }

    if (radial->rays_left == 0) {
        return NULL;
    }

    ray  = radial->current;
    size = nexrad_radial_ray_size(ray, radial->type);

    /*
     * Advance the current ray pointer beyond the ray to follow.
     */
    radial->current = (nexrad_radial_ray *)((char *)ray + size);

    /*
     * Increase the number of bytes read in the current radial packet.
     */
    radial->bytes_read += size;

    /*
     * Decrement the number of rays left to read.
     */
     radial->rays_left--;

    /*
     * If the caller provided a pointer to an address in which to store the size
     * of the current ray, then populate that value.
     */
    if (sizep)
        *sizep = size;

    /*
     * If the caller provided a pointer to an address to populate with a pointer
     * to the runs within the current ray, then provide that.
     */
    if (runs != NULL)
        *runs = (nexrad_radial_run *)((char *)ray + sizeof(nexrad_radial_ray));

    return ray;
}

size_t nexrad_radial_bytes_read(nexrad_radial *radial) {
    if (radial == NULL) {
        return 0;
    }

    return radial->bytes_read;
}

void nexrad_radial_close(nexrad_radial *radial) {
    if (radial == NULL) return;

    radial->packet     = NULL;
    radial->bytes_read = 0;
    radial->rays_left  = 0;
    radial->current    = NULL;

    free(radial);
}
