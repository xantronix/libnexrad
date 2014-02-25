#ifndef _NEXRAD_MESSAGE_H
#define _NEXRAD_MESSAGE_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/header.h>
#include <nexrad/chunk.h>
#include <nexrad/product.h>
#include <nexrad/symbology.h>
#include <nexrad/graphic.h>
#include <nexrad/tabular.h>
#include <nexrad/packet.h>

#define NEXRAD_MESSAGE_MAX_BODY_SIZE 8388608
#define NEXRAD_MESSAGE_MAX_SIZE     10485760

/*!
 * \file nexrad/message.h
 * \brief Interface to NEXRAD Level III product message files
 *
 * An interface to NEXRAD Level III product message files, allowing one to
 * access the contents of any of the five Product Message Blocks in an
 * iterable fashion.
 */

typedef struct _nexrad_message nexrad_message;

/*!
 * \defgroup message NEXRAD Level III product message functions
 */

/*!
 * \ingroup message
 * \brief Load a NEXRAD Level III product message file from memory
 * \param buf Pointer to a memory buffer
 * \param len Size of memory buffer in `len`
 * \return An object representing a NEXRAD Level III product message file
 *
 * Load a NEXRAD Level III product message from a sized memory buffer.
 */
nexrad_message *nexrad_message_open_buf(void *buf, size_t len);

/*!
 * \ingroup message
 * \brief Load a NEXRAD Level III product message file from disk
 * \param path A path to a NEXRAD Level III product message file on disk
 * \return An object representing a NEXRAD Level III Product message file
 *
 * Load a NEXRAD Level III product message file from disk.
 */
nexrad_message *nexrad_message_open(const char *path);

/*!
 * \ingroup message
 * \brief Destroy a nexrad_message object
 * \param message An opened NEXRAD Level III message file
 *
 * Free any state associated with an opened message file, and deallocate the
 * memory storing the object itself.  Furthermore, any memory-mapped state is
 * unmapped from the address space.
 */
void nexrad_message_destroy(nexrad_message *message);

/*!
 * \ingroup message
 * \brief Close a `nexrad_message` object
 * \param message An opened NEXRAD Level III message file
 *
 * A wrapper to `nexrad_message_destroy()`.
 */
void nexrad_message_close(nexrad_message *message);

nexrad_message_header *nexrad_message_get_header(nexrad_message *message);

nexrad_product_description *nexrad_message_get_product_description(nexrad_message *message);

nexrad_symbology_block *nexrad_message_get_symbology_block(nexrad_message *message);

nexrad_graphic_block *nexrad_message_get_graphic_block(nexrad_message *message);

nexrad_tabular_block *nexrad_message_get_tabular_block(nexrad_message *message);

/*!
 * \ingroup message
 * \brief Obtain the time at which radar data was scanned
 * \param message An opened NEXRAD Level III message file
 * \return Unix epoch timestamp
 *
 * Obtain the radar scan timestamp of an opened NEXRAD Level III product
 * message file.
 */
time_t nexrad_message_get_scan_timestamp(nexrad_message *message);

/*!
 * \ingroup message
 * \brief Obtain the time at which the product file was generated
 * \param message An opened NEXRAD Level III message file
 * \return Unix epoch timestamp
 *
 * Obtain the product generation timestamp of an opened NEXRAD Level III product
 * message file.
 */
time_t nexrad_message_get_gen_timestamp(nexrad_message *message);

/*!
 * \ingroup message
 * \brief Obtain product type code
 * \param message An opened NEXRAD Level III message file
 * \return NEXRAD Level III product type code as integer, or -1 on failure
 *
 * Return the product type code of an opened NEXRAD Level III product message
 * file.
 */
int nexrad_message_get_product_type(nexrad_message *message);

/*!
 * \ingroup message
 * \brief Obtain product code string
 * \param message An opened NEXRAD Level III message file
 * \param code Address of a string pointer to return string containing product
 *             code
 * \param len Size of string returned in `code`, in bytes
 * \return 0 on success, -1 on failure
 *
 * Return the symbolic string type code of an opened NEXRAD Level III message
 * file.
 */
int nexrad_message_find_product_code(nexrad_message *message,
    char **code,
    size_t *len
);

int nexrad_message_find_region(
    nexrad_message *message,
    char **region,
    size_t *len
);

int nexrad_message_find_office(nexrad_message *message,
    char **office, size_t *len
);

char nexrad_message_get_station_prefix(nexrad_message *message);

int nexrad_message_find_station_suffix(nexrad_message *message,
    char **suffix, size_t *len
);

/*!
 * \ingroup message
 * \brief Determine the WSR-88D station identifier of a product file
 * \param message An opened NEXRAD Level III message file
 * \param station Pointer to a string to write station identifier into
 * \param destlen Maximum size, in bytes, of the buffer to contain the station
 *                identifier
 * \return 0 on success, -1 on failure
 *
 * Determine the WSR-88D station identifier based on information contained
 * within the NEXRAD Level III product file.
 */
int nexrad_message_read_station(nexrad_message *message,
    char *station,
    size_t destlen
);

/*!
 * \ingroup message
 * \brief Determine the location of the radar which produced a product file
 * \param message An opened NEXRAD Level III message file
 * \param lat Pointer to a double to store station latitude
 * \param lon Pointer to a double to store station longitude
 * \param alt Pointer to a double to store station altitude
 * \return 0 on success, -1 on failure
 *
 * Obtain the latitude, longitude and altitude of the WSR-88D station antenna.
 */
int nexrad_message_read_station_location(nexrad_message *message,
    double *lat,
    double *lon,
    double *alt
);

/*!
 * \ingroup message
 * \brief Obtain the first data packet within a product symbology block
 * \param message An opened NEXRAD Level III message file
 * \param type Expected type of product symbology block packet
 * \return Pointer to a product symbology block packet, or NULL on failure
 *
 * Return the first packet in the product symbology block for an opened message
 * file, so long as its type matches the identifier in `type`.
 */
nexrad_packet *nexrad_message_find_symbology_packet_by_type(nexrad_message *message,
    enum nexrad_packet_type type
);

#endif /* _NEXRAD_MESSAGE_H */
