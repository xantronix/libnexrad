#ifndef _NEXRAD_EET_H
#define _NEXRAD_EET_H

#include <stdint.h>

#define NEXRAD_EET_OFFSET         2
#define NEXRAD_EET_METERS       304.8
#define NEXRAD_EET_VALUE_MASK  0x7f
#define NEXRAD_EET_TOPPED_FLAG (1 << 7)

/*!
 * \file nexrad/eet.h
 * \brief NEXRAD Level III Enhanced Echo Tops product data access routines
 *
 * Provides routines for accessing and converting raw Enhanced Echo Top data
 * values to data in units of meters above mean sea level.
 */

/*!
 * \defgroup eet NEXRAD Level III Enhanced Echo Tops data access routines
 */

/*!
 * \ingroup eet
 * \brief Determine if given rangebin value contains valid data within range
 * \param v 8-bit data value from product file
 * \return 1 if valid, 0 if invalid
 *
 * Determine if a rangebin sample contains valid data.
 */
int nexrad_eet_valid(uint8_t v);

/*!
 * \ingroup eet
 * \brief Determine if given rangebin value reaches top of storm
 * \param v 8-bit data value from product file
 * \return 1 if topped, 0 if not topped
 *
 * Determine if a rangebin sample reaches the top of a storm.
 */
int nexrad_eet_topped(uint8_t v);

/*!
 * \ingroup eet
 * \brief Determine maximum height of echo from raw data sample
 * \param v 8-bit data value from product file
 * \return Number of meters above mean sea level
 *
 * Determine the height of echoes returned based on a given Enhanced Echo Tops
 * data sample, in meters above mean sea level.
 */
double nexrad_eet_meters(uint8_t v);

#endif /* _NEXRAD_EET_H */
