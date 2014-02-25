#ifndef _NEXRAD_POLY_H
#define _NEXRAD_POLY_H

#include <nexrad/radial.h>

#define NEXRAD_POLY_BYTE_ORDER_LE 1
#define NEXRAD_POLY_TYPE          3
#define NEXRAD_POLY_MULTI_TYPE    6
#define NEXRAD_POLY_RINGS         1
#define NEXRAD_POLY_POINTS        4

/*!
 * \file nexrad/poly.h
 * \brief Support for generating Well-Known Binary geometries from radar data
 *
 * Provides functions for generating Well-Known Binary geometry boundary
 * representations of radial radar data.
 */

#pragma pack(1)
#pragma pack(push)

typedef struct _nexrad_poly_point {
    double lon;
    double lat;
} nexrad_poly_point;

typedef struct _nexrad_poly_ring {
    uint32_t points;
} nexrad_poly_ring;

typedef struct _nexrad_poly {
    uint8_t  byte_order;
    uint32_t type;
    uint32_t rings;
} nexrad_poly;

typedef struct _nexrad_poly_multi {
    uint8_t  byte_order;
    uint32_t type;
    uint32_t polys;
} nexrad_poly_multi;

#pragma pack(pop)

/*!
 * \defgroup poly Well-Known Binary geographic polygon generation routines
 */

/*!
 * \ingroup poly
 * \brief Determine size of Well-Known Binary output for a given radial and
 *        value ranges
 * \param radial A radial reader object
 * \param min Minimum intensity value of radar data to generate polygons for
 * \param max Maximum intensity value of radar data to generate polygons for
 * \param sizep A pointer to a size_t to write size, in bytes, of WKB data
 * \param rangebinsp A pointer to an int to write number of matching rangebins
 * \return 0 on success, -1 on failure
 *
 * Determine the precise size of a Well-Known Binary representation of radar
 * data matching the minimum and maximum value thresholds specified, as well as
 * the number of matching rangebins (or, number of polygons to be generated).
 * This is necessary as one must allocate an adequately-sized buffer to
 * subsequently write Well-Known Binary data to later.
 */
int nexrad_poly_multi_size_for_radial(nexrad_radial *radial,
    int      min,
    int      max,
    size_t * sizep,
    int *    rangebinsp
);

/*!
 * \ingroup poly
 * \brief Write a Well-Known Binary representation of radial data to a buffer
 * \param radial A radial packet reader object
 * \param min Minimum intensity value of radar data to generate polygons for
 * \param max Maximum intensity value of radar data to generate polygons for
 * \param rangebins Number of matching rangebins to create polygons for
 * \param size Size of buffer to write rangebins into
 * \param radar Cartesian location of radar site
 * \param spheroid WGS-84 spheroid datum object
 * \return 0 on success, -1 on failure
 *
 * Generate and write a Well-Known Binary representation of radial data matching
 * the minimum and maximum intensity values as specified by the caller.  Polygon
 * points are generated from geodesic calculations based on WGS-84 datums, with
 * each polygon a crude trapezoidal representation of a radar rangebin's corner
 * extents.
 */
int nexrad_poly_multi_write_from_radial(
    nexrad_radial *        radial,
    int                    min,
    int                    max,
    int                    rangebins,
    nexrad_poly_multi *    multi,
    size_t                 size,
    nexrad_geo_cartesian * radar,
    nexrad_geo_spheroid *  spheroid
);

/*!
 * \ingroup poly
 * \brief Create a Well-Known Binary representation of radial data
 * \param radial A radial packet reader object
 * \param min Minimum intensity value of radar data to generate polygons for
 * \param max Maximum intensity value of radar data to generate polygons for
 * \param radar Cartesian location of radar site
 * \param spheroid WGS-84 spheroid datum object
 * \param sizep Pointer to a size_t to write size, in bytes, of WKB data to
 * \return 0 on success, -1 on failure
 *
 * Create and populate a new buffer containing a Well-Known Binary
 * representation of radial data matching the minimum and maximum intensity
 * values as specified by the caller.  Polygon points are generated from
 * geodesic calculations based on WGS-84 datums, with each polygon a crude
 * trapezoidal representation of a radar rangebin's corner extents.
 *
 * Same as calling nexrad_poly_multi_size_for_radial(), then
 * nexrad_poly_multi_write_from_radial().
 */
nexrad_poly_multi *nexrad_poly_multi_create_from_radial(
    nexrad_radial *        radial,
    int                    min,
    int                    max,
    nexrad_geo_cartesian * radar,
    nexrad_geo_spheroid *  spheroid,
    size_t *               sizep
);

/*!
 * \ingroup poly
 * \brief free()s a Well-Known Binary buffer
 * \param multi A multipolygon Well-Known Binary buffer
 *
 * free()s a Well-Known Binary buffer created by 
 * nexrad_poly_multi_create_from_radial().
 */
void nexrad_poly_multi_destroy(nexrad_poly_multi *multi);

#endif /* _NEXRAD_POLY_H */
