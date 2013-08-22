#include <stdlib.h>
#include <endian.h>

#include <nexrad/radial.h>

static int _valid_packet(nexrad_radial_packet *packet) {
    if (
      packet                          == NULL ||
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

    if (!_valid_packet(packet)) {
        return NULL;
    }

    if ((radial = malloc(sizeof(*radial))) == NULL) {
        goto error_malloc;
    }

    radial->packet    = packet;
    radial->rays_left = be16toh(packet->rays);
    radial->current   = (nexrad_radial_ray *)((char *)packet + sizeof(nexrad_radial_packet));

    return radial;

error_malloc:
    return NULL;
}

size_t _ray_size(nexrad_radial_ray *ray) {
    return sizeof(nexrad_radial_ray) + be16toh(ray->runs);
}

nexrad_radial_ray *nexrad_radial_read_ray(nexrad_radial *radial, nexrad_radial_run **runs) {
    nexrad_radial_ray *ray;
    size_t size;

    if (radial == NULL) {
        return NULL;
    }

    if (radial->rays_left == 0) {
        return NULL;
    }

    ray  = radial->current;
    size = _ray_size(ray);

    /*
     * Advance the current ray pointer beyond the ray to follow.
     */
    radial->current = (nexrad_radial_ray *)((char *)ray + size);

    /*
     * Decrement the number of rays left to read.
     */
     radial->rays_left--;

    /*
     * If the caller provided a pointer to an address to populate with a pointer
     * to the RLE-encoded runs within the current ray, then provide that.
     */
    if (runs != NULL) {
        *runs = (nexrad_radial_run *)((char *)ray + sizeof(nexrad_radial_ray));
    }

    return ray;
}

void nexrad_radial_close(nexrad_radial *radial) {
    if (radial == NULL) return;

    radial->packet    = NULL;
    radial->rays_left = 0;
    radial->current   = NULL;

    free(radial);
}
