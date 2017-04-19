/*
 * Copyright (c) 2016 Dynamic Weather Solutions, Inc. Distributed under the
 * terms of the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef _NEXRAD_GEO_H
#define _NEXRAD_GEO_H

#define NEXRAD_GEO_SPHEROID_RADIUS     6378137.0
#define NEXRAD_GEO_SPHEROID_FLATTENING  (1/298.257223563)

#define NEXRAD_GEO_NM_METERS       1852
#define NEXRAD_GEO_COORD_MAGNITUDE    0.001
#define NEXRAD_GEO_AZIMUTH_FACTOR     0.1

#define NEXRAD_GEO_PROJECTION_MAGIC   "PROJ"
#define NEXRAD_GEO_PROJECTION_VERSION 0x01

#define NEXRAD_GEO_MERCATOR_MAX_LAT    85.05112878
#define NEXRAD_GEO_MERCATOR_TILE_SIZE 256
#define NEXRAD_GEO_MERCATOR_MIN_ZOOM    4
#define NEXRAD_GEO_MERCATOR_MAX_ZOOM   10

#include <stdint.h>

enum nexrad_geo_projection_type {
    NEXRAD_GEO_PROJECTION_NONE,
    NEXRAD_GEO_PROJECTION_SPHEROID,
    NEXRAD_GEO_PROJECTION_EQUIRECT,
    NEXRAD_GEO_PROJECTION_MERCATOR
};

typedef struct _nexrad_geo_polar {
    double azimuth;
    double range;
} nexrad_geo_polar;

typedef struct _nexrad_geo_cartesian {
    double lat;
    double lon;
} nexrad_geo_cartesian;

/*!
 * \file nexrad/geo.h
 * \brief Geodesic calculations and geographic projection support
 */

#pragma pack(1)
#pragma pack(push)

typedef struct _nexrad_geo_projection_header {
    char     magic[4];
    uint16_t version;
    uint16_t type;
    uint16_t width;
    uint16_t height;
    uint32_t world_width;    /* Width of world in projected pixels */
    uint32_t world_height;   /* Height of world in projected pixels */
    uint32_t world_offset_x; /* Offset of projection in world */
    uint32_t world_offset_y; /* Offset of projection in world */
    uint16_t rangebins;
    uint16_t rangebin_meters;
    uint16_t angle; /* Scan elevation angle */

    struct {
        int32_t lat;
        int32_t lon;
    } extents[4];

    union {
        char unknown[32];

        struct {
            uint32_t scale;
            char unused[28];
        } equirect;

        struct {
            uint32_t scale;
            uint16_t zoom;
            char unused[26];
        } mercator;
    } opts;

    /*
     * Information pertaining to radar at center of projection
     */
     int32_t station_lat;
     int32_t station_lon;
} nexrad_geo_projection_header;

typedef struct _nexrad_geo_projection_point {
    uint16_t azimuth;
    uint16_t range;
} nexrad_geo_projection_point;

#pragma pack(pop)

typedef struct _nexrad_geo_spheroid   nexrad_geo_spheroid;
typedef struct _nexrad_geo_projection nexrad_geo_projection;

/*!
 * \defgroup spheroid WGS-84 spheroid functions
 */

/*!
 * \ingroup spheroid
 * \brief Create a new spheroid object initialized with WGS-84 datums.
 * \return A new spheroid object, or NULL on error
 */
nexrad_geo_spheroid *nexrad_geo_spheroid_create();

/*!
 * \ingroup spheroid
 * \brief Determine spheroid radius
 * \param spheroid A spheroid object
 * \return Radius of spheroid, in meters
 */
double nexrad_geo_spheroid_get_radius(nexrad_geo_spheroid *spheroid);

/*!
 * \ingroup spheroid
 * \brief Determine spheroid flattening ratio
 * \param spheroid A spheroid object
 * \return Flattening factor of spheroid
 */
double nexrad_geo_spheroid_get_flattening(nexrad_geo_spheroid *spheroid);

/*!
 * \ingroup spheroid
 * \brief Destroy and free() a spheroid object
 * \param object A spheroid object
 */
void nexrad_geo_spheroid_destroy(nexrad_geo_spheroid *spheroid);

/*!
 * \defgroup geodesy Geodesic calculation functions
 */

/*!
 * \ingroup geodesy
 * \brief Perform inverse geodesic calculation between Cartesian points
 * \param spheroid A spheroid object
 * \param origin The origin Cartesian point
 * \param dest The destination cartesian point
 * \param output Pointer to an object to store polar coordinate result
 */
void nexrad_geo_find_polar_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * dest,
    nexrad_geo_polar *     output
);

/*!
 * \ingroup geodesy
 * \brief Perform direct geodesic calculation from a Cartesian point
 * \param spheroid A spheroid object
 * \param origin The origin Cartesian point
 * \param output Pointer to an object to store Cartesian coordinate result
 * \param dest Polar coordinate of destination relative to origin
 */
void nexrad_geo_find_cartesian_dest(nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian * origin,
    nexrad_geo_cartesian * output,
    nexrad_geo_polar *     dest
);

/*!
 * \defgroup projection Geographic radar projection functions
 */

/*!
 * \ingroup projection
 * \brief Determine the geographic extents of a radar beam
 * \param spheroid A spheroid object
 * \param radar A Cartesian point indicating location of radar
 * \param rangebins Number of rangebins of radar coverage
 * \param rangebin_meters Scale of a single rangebin, in meters
 * \param extents A pointer to an array of four contiguous Cartesian points
 *
 * Determine the geographic extents of a radar beam.  Extents are written to the
 * array of Cartesian points specified in `extents`, in the following order:
 *
 *     - North (0)
 *     - East  (1)
 *     - South (2)
 *     - West  (3)
 */
void nexrad_geo_projection_find_extents(
    nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian *radar,
    uint16_t rangebins,
    uint16_t rangebin_meters,
    nexrad_geo_cartesian *extents
);

/*!
 * \ingroup projection
 * \brief Create a new equirectangular projection file for a radar site
 * \param path Path to a file to create and store radial projection data to
 * \param spheroid A spheroid object
 * \param radar Cartesian coordinates of radar site
 * \param rangebins Number of rangebins in radar coverage area
 * \param rangebin_meters Size of each rangebin, in meters, in terms of distance
          from radar site
 * \param scale Number of degrees of latitude/longitude per projection point
 * \return A new radar projection object, or NULL on failure
 *
 * Create a new equirectangular projection file for a radar site, suitable for
 * creating rasterized radar images which conform to an equirectangular
 * projection at a given radar site.
 */
nexrad_geo_projection *nexrad_geo_projection_create_equirect(
    const char *path,
    nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian *radar,
    uint16_t rangebins,
    uint16_t rangebin_meters,
    double scale
);

/*!
 * \ingroup projection
 * \brief Create a new Mercator projection file for a radar site
 * \param path Path to a file to create and store radial projection data to
 * \param spheroid A spheroid object
 * \param radar Cartesian coordinates of radar site
 * \param rangebins Number of rangebins in radar coverage area
 * \param rangebin_meters Size of each rangebin, in meters, in terms of distance
          from radar site
 * \param zoom Web Mercator zoom level of resulting projection
 * \return A new radar projection object, or NULL on failure
 *
 * Create a new Mercator projection file for a radar site, suitable for creating
 * rasterized radar images which conform to a Mercator projection at a given
 * radar site.
 */
nexrad_geo_projection *nexrad_geo_projection_create_mercator(
    const char *path,
    nexrad_geo_spheroid *spheroid,
    nexrad_geo_cartesian *radar,
    uint16_t rangebins,
    uint16_t rangebin_meters,
    int zoom
);

/*!
 * \ingroup projection
 * \brief Open an existing geographic projection file from disk, using memory-
 *        mapped I/O.
 * \param path A path to a projection file on disk.
 * \return A geographic projection object, or NULL on failure
 */
nexrad_geo_projection *nexrad_geo_projection_open(const char *path);

/*!
 * \ingroup projection
 * \brief Determine the type of projection of an existing geographic projection
 * \param proj A geographic projection object
 * \return A value from enum nexrad_geo_projection_type indicating projection
 *         type
 */
enum nexrad_geo_projection_type nexrad_geo_projection_get_type(
    nexrad_geo_projection *proj
);

/*!
 * \ingroup projection
 * \brief Determine the dimensions, in points, of a projection
 * \param proj A geographic projection object
 * \param width A pointer to a uint16_t to write projection width to
 * \param height A pointer to a uint16_t to write projection height to
 * \return 0 on success, -1 on failure
 */
int nexrad_geo_projection_read_dimensions(
    nexrad_geo_projection *proj,
    uint16_t *width,
    uint16_t *height
);

/*!
 * \ingroup projection
 * \brief Determine radar coverage area of projection
 * \param proj A geographic projection object
 * \param rangebins A pointer to a uint16_t to write number of rangebins covered
 *        by radar site to
 * \param rangebin_meters A pointer to a uint16_t to write number of meters per
 *        rangebin as used in this projection
 * \return 0 on success, -1 on failure
 */
int nexrad_geo_projection_read_range(
    nexrad_geo_projection *proj,
    uint16_t *rangebins,
    uint16_t *rangebin_meters
);

/*!
 * \ingroup projection
 * \brief Determine radar station location for projection object
 * \param proj A geographic projection object
 * \param radar A pointer to a Cartesian point to write station location to
 * \return 0 on success, -1 on failure
 */
int nexrad_geo_projection_read_station_location(
    nexrad_geo_projection *proj,
    nexrad_geo_cartesian *radar
);

/*!
 * \ingroup projection
 * \brief Determine geographic extents covered by radar projection, in Cartesian
 *        coordinates
 * \param proj A geographic projection object
 * \param extents A pointer to a contiguous array of four (4) Cartesian points
 * \return 0 on success, -1 on failure
 *
 * Determine geographic extents covered by radar projection, in Cartesian
 * coordinates.  Location extents are written into four (4) separate Cartesian
 * point objects, each in the following order:
 *
 *     - North (0)
 *     - East  (1)
 *     - South (2)
 *     - West  (3)
 */
int nexrad_geo_projection_read_extents(
    nexrad_geo_projection *proj,
    nexrad_geo_cartesian *extents
);

/*!
 * \ingroup projection
 * \brief Determine geographic Cartesian point at given location in projection
 * \param proj A geographic projection object
 * \param x X coordinate within projection
 * \param y Y coordinate within projection
 * \param cartesian Pointer to Cartesian point to write geographic location of
 *        point in projection to
 * \return 0 on success, -1 on failure
 *
 * Determine the geographic location of any given X,Y point in a projection
 * object.
 */
int nexrad_geo_projection_find_cartesian_point(nexrad_geo_projection *proj,
    uint16_t x,
    uint16_t y,
    nexrad_geo_cartesian *cartesian
);

/*!
 * \ingroup projection
 * \brief Determine geographic polar point, relative to radar, at given location
 * \param proj A geographic projection object
 * \param x X coordinate within projection
 * \param y Y coordinate within projection
 * \param polar Pointer to object to write radar-relative polar coordinates into
 * \return 0 on success, -1 on failure
 *
 * Determine the radar-relative polar coordinates of any given X,Y point in a
 * projection object.
 */
int nexrad_geo_projection_find_polar_point(nexrad_geo_projection *proj,
    uint16_t x,
    uint16_t y,
    nexrad_geo_polar *polar
);

/*!
 * \ingroup projection
 * \brief Obtain pointer to projection point values in projection object
 * \param proj A geographic projection object
 * \return A pointer to projection point values in projection object
 *
 * Obtain a pointer to projection values in projection object, which are pairs
 * of big endian, 16-bit unsigned integers indicating azimuth and range of point
 * relative to radar site.  Use be16toh() to reference these projection point
 * polar coordinates in host byte order.
 */
nexrad_geo_projection_point *nexrad_geo_projection_get_points(nexrad_geo_projection *proj);

/*!
 * \ingroup projection
 * \brief Save changes made to projection from memory to disk
 * \param proj A geographic projection object
 * \return 0 on success, -1 on failure
 *
 * Flush any changes made to projection metadata or points from memory to the
 * file mapped to disk.
 */
int nexrad_geo_projection_save(nexrad_geo_projection *proj);

/*!
 * \ingroup projection
 * \brief Close memory-mapped projection file and deallocate object from memory
 * \param A geographic projection object
 */
void nexrad_geo_projection_close(nexrad_geo_projection *proj);

#endif /* _NEXRAD_GEO_POINT_H */
