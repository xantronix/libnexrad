#ifndef _NEXRAD_RADIAL_H
#define _NEXRAD_RADIAL_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/geo.h>
#include <nexrad/image.h>

#define NEXRAD_RADIAL_RLE_FACTOR     16
#define NEXRAD_RADIAL_AZIMUTH_FACTOR  0.1
#define NEXRAD_RADIAL_RANGE_FACTOR    0.001

enum nexrad_radial_type {
    NEXRAD_RADIAL_RLE     = 0xaf1f,
    NEXRAD_RADIAL_DIGITAL = 16
};

/*!
 * \file nexrad/radial.h
 * \brief Interface to radial radar data in NEXRAD Level III product files
 *
 * The primary interface to reading and handling radial radar data encoded in
 * NEXRAD Level III product files.  Is able to understand radar data encoded
 * both in a plain, 8-bits-per-rangebin (digital) manner, as well as a run
 * length-encoded format.
 */

#pragma pack(1)
#pragma pack(push)

typedef struct _nexrad_radial_packet {
    uint16_t type;           /* 16 or 0xaf1f */
    uint16_t rangebin_first; /* Index to first range bin (whatever the hell that means) */
    uint16_t rangebin_count; /* Number of range bins per radial (resolution) */
     int16_t i;              /* I coordinate of center of sweep */
     int16_t j;              /* J coordinate of center of sweep */
    uint16_t scale;          /* Scale factor in units of 0.001 */
    uint16_t rays;           /* Number of rays in product */
} nexrad_radial_packet;

typedef struct _nexrad_radial_ray {
    uint16_t size;        /* Number of halfwords (0xaf1f) or bytes (16) in current ray */
    uint16_t angle_start; /* Scan angle of current ray */
    uint16_t angle_delta; /* Angle delta from previous ray */
} nexrad_radial_ray;

typedef struct _nexrad_radial_run { /* For 0xaf1f */
    unsigned int level  :4; /* Level (color code) of run */
    unsigned int length :4; /* Length of run */
} nexrad_radial_run;

#pragma pack(pop)

typedef struct _nexrad_radial nexrad_radial;

/*!
 * \defgroup radial NEXRAD Level III radial data handling routines
 */

/*!
 * \ingroup radial
 * \brief Unpack any radial packet into a well-ordered digitally-encoded
 * \param packet A RLE or digitally-encoded radial packet
 * \param sizep Pointer to a size_t to store size of new radial packet
 * \return A well-ordered digitally-encoded radial packet
 *
 * Given an arbitrary radial packet, whether RLE- or digitally-encoded, will
 * generate a new radial packet with 360 rays whose azimuths are ordered 0 to
 * 359, in digital encoding.  RLE-encoded values are scaled from rangebin values
 * of 0-15 to 0-255.
 *
 * The purpose of this method is to allow code performing manipulations upon
 * radial data to do so quickly in a random-access manner, allowing one to, for
 * instance, reference any rangebin by azimuth and range in a single memory
 * lookup operation.
 */
nexrad_radial_packet *nexrad_radial_packet_unpack(nexrad_radial_packet *packet,
    size_t *sizep
);

/*!
 * \ingroup radial
 * \brief Open a NEXRAD Level III radial packet for reading
 * \param packet Pointer to a raw NEXRAD Level III radial packet
 * \return An object to facilitate access of the contents of a NEXRAD Level III
 *         radial packet
 *
 * Open a NEXRAD Level III radial packet for reading, returning an object
 * representing the state necessary to traverse a radial packet of any sort.
 */
nexrad_radial *nexrad_radial_packet_open(nexrad_radial_packet *packet);

/*!
 * \ingroup radial
 * \brief Determine how many bytes of a radial packet have been read
 * \param radial A `nexrad_radial` object
 * \return Number of bytes of NEXRAD Level III radial packet read
 *
 * Determine how many bytes of a radial packet have been read.  This method is
 * useful, as NEXRAD Level III product symbology blocks themselves have a size
 * attribute, but the radial data packets therein do not.  It is therefore not
 * possible to know in advance the size (in bytes) of a RLE-encoded radial
 * packet without reading it, so keeping track of the size of a packet is needed
 * in order to properly read the next packet that may occur in a product
 * symbology block.
 */
size_t nexrad_radial_bytes_read(nexrad_radial *radial);

/*!
 * \ingroup radial
 * \brief Reset a `nexrad_radial` object to the beginning of a radial packet
 * \param radial A `nexrad_radial` object
 *
 * Reset a `nexrad_radial` object state to the beginning of a radial packet, in
 * case one wants to reuse the same `nexrad_radial` object to make multiple
 * passes over a single radial packet.
 */
void nexrad_radial_reset(nexrad_radial *radial);

/*!
 * \ingroup radial
 * \brief Close and destroy all state in a NEXRAD Level III radial packet reader object
 * \param radial A `nexrad_radial` object
 *
 * Destroys and free()s all state used to read a NEXRAD Level III radial packet.
 * Use this method instead of nexrad_radial_destroy() when the caller does not
 * wish to implicitly free() the radial packet it references.
 */
void nexrad_radial_close(nexrad_radial *radial);

/*!
 * \ingroup radial
 * \brief Close and destroy all state in a NEXRAD Level III radial object
 * \param radial A `nexrad_radial` object
 *
 * Destroys and free()s all state used to read a NEXRAD Level III radial packet.
 * Use this method instead of nexrad_radial_close() when the caller wishes for
 * the radial packet it references to also be free()d.
 */
void nexrad_radial_destroy(nexrad_radial *radial);

/*!
 * \ingroup radial
 * \brief Search radial packet for radial ray at given azimuth
 * \param radial A radial reader object
 * \param azimuth The azimuth 0-359 of the desired ray
 * \param values Pointer to write address of digitally encoded rangebin values
 *               to
 * \return A radial ray object
 *
 * With the aid of the radial packet reader in `radial`, search for, and return
 * the NEXRAD Level III radial ray whose azimuth is at the desired value.  A
 * pointer to uint8_t values containing one 1km rangebin value per byte is
 * written in `values` just prior to return.
 *
 * Note that the data returned in `values` is only valid until the next call to
 * nexrad_radial_get_ray() or nexrad_radial_read_ray().
 */
nexrad_radial_ray *nexrad_radial_get_ray(nexrad_radial *radial,
    int azimuth,
    uint8_t **values
);

/*!
 * \ingroup radial
 * \brief Determine the azimuth of a given NEXRAD Level III radial ray
 * \param ray A NEXRAD Level III radial ray
 * \return An integer 0-359 indicating azimuth of the radial ray, or -1 on
 *         error
 *
 * Return the azimuth of the radial ray, or -1 when given erroneous input.
 */
int nexrad_radial_ray_get_azimuth(nexrad_radial_ray *ray);

/*!
 * \ingroup radial
 * \brief Determine a rangebin value for a given azimuth and range
 * \param radial A radial packet reader object
 * \param azimuth Azimuth 0-359
 * \param range Distance from radar in 1km increments
 * \return An integer 0-255 denoting the observed value, or -1 on failure
 *
 * Determine the value of a rangebin at a given azimuth and range.
 */
int nexrad_radial_get_rangebin(nexrad_radial *radial,
    int azimuth,
    int range
);

/*!
 * \ingroup radial
 * \brief Read the next available ray in a NEXRAD Level III radial packet
 * \param radial A radial packet reader object
 * \param values Pointer to an address to store pointer referencing rangebin
 *               values
 * \return A NEXRAD Level III radial ray, or NULL if no more rays are available
 *
 * Read the next available ray in a NEXRAD Level III radial packet.  If no more
 * rays are available to be read, then NULL will be returned instead.
 */
nexrad_radial_ray *nexrad_radial_read_ray(nexrad_radial *radial,
    uint8_t **values
);

/*!
 * \ingroup radial
 * \brief Return the type of packet referenced by the current radial reader
 * \param radial A radial packet reader object
 * \return A number indicating whether the radial packet referenced by the
 *         radial reader is RLE- or digitally-encoded, or -1 on error
 *
 * Returns a value, either NEXRAD_RADIAL_DIGITAL, or NEXRAD_RADIAL_RLE,
 * indicating the type of packet referenced by the current radial packet reader.
 */
enum nexrad_radial_type nexrad_radial_get_type(nexrad_radial *radial);

/*!
 * \ingroup radial
 * \brief Ascertain the dimensions and properties of a radial
 * \param radial A radial packet reader object
 * \param rangebin_first Pointer to a uint16_t to write distance offset of
 *        first rangebin
 * \param rangebin_count Pointer to a uint16_t to write number of rangebins per
 *        ray
 * \param i Mostly unused
 * \param j Mostly unused
 * \param scale Number of meters per rangebin
 * \param rays Number of rays in radial packet
 * \return 0 on success, -1 on failure
 *
 * Determine various characteristics of a radial packet referenced by the radial
 * reader object passed.
 */
int nexrad_radial_get_info(nexrad_radial *radial,
    uint16_t *rangebin_first,
    uint16_t *rangebin_count,
     int16_t *i,
     int16_t *j,
    uint16_t *scale,
    uint16_t *rays
);

/*!
 * \ingroup radial
 * \brief Determine number of rays left to be read
 * \param radial A radial reader object
 * \return Number of rays left to be read
 * 
 * Deteremine the number of rays left to be read in a radial packet referenced
 * by a radial reader object.
 */
uint16_t nexrad_radial_rays_left(nexrad_radial *radial);

/*!
 * \ingroup radial
 * \brief Get a pointer to the radial packet held by the radial reader object
 * \param radial A radial reader object
 * \return A pointer to the NEXRAD Level III radial packet referenced by the
 *         radial reader object, or NULL on error
 *
 * Return a pointer to the NEXRAD Level III radial packet referenced by the
 * current radial reader object.
 */
nexrad_radial_packet *nexrad_radial_get_packet(nexrad_radial *radial);

/*!
 * \ingroup radial
 * \brief Create a top-down image render of a NEXRAD Level III radial packet
 * \param radial A radial reader object
 * \param table A color table object
 * \return A `nexrad_image` object containing rasterized radar data
 *
 * Rasterize a radial packet referenced by the radial reader object in a simple,
 * polar-distorted, top-down projection, using the color intensity values
 * represented in the color table provided.
 */
nexrad_image *nexrad_radial_create_image(nexrad_radial *radial,
    nexrad_color_table *table
);

/*!
 * \ingroup radial
 * \brief Create a map projected render of a NEXRAD Level III radial packet
 * \param radial A radial reader object
 * \param table A color table
 * \param proj A cartographic radar projection object
 * \return A `nexrad_image` object containing rasterized radar data
 *
 * Rasterize a radial packet referenced by the radial reader object in an image
 * which conforms to a precomputed projection referenced by `proj`, using the
 * color intensity values represented in the provided color table.
 */
nexrad_image *nexrad_radial_create_projected_image(nexrad_radial *radial,
    nexrad_color_table *table,
    nexrad_geo_projection *proj
);

#endif /* _NEXRAD_RADIAL_H */
