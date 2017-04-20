#include <stdlib.h>
#include <math.h>

#include <nexrad/map.h>

static void _find_point(nexrad_map_point start,
                        nexrad_map_heading heading,
                        nexrad_map_point *end) {
    float phi1 =       start.lat * (M_PI / 180.0),
            l1 =       start.lon * (M_PI / 180.0),
            a1 = heading.azimuth * (M_PI / 180.0);

    float sigma12 = heading.range / NEXRAD_MAP_EARTH_RADIUS,
            cosa1 = cosf(a1),
            sina1 = sinf(a1),
          sinphi1 = sinf(phi1),
          cosphi1 = cosf(phi1),
       cossigma12 = cosf(sigma12),
       sinsigma12 = sinf(sigma12);

    float phi2 = atan2f(sinphi1 * cossigma12 + cosphi1 * sinsigma12 * cosa1,
        sqrtf(powf(cosphi1 * cossigma12 - sinphi1 * sinsigma12 * cosa1, 2.0)
            + powf(sinsigma12 * sina1, 2.0)));

    float l12 = atan2f(sinsigma12 * sina1,
        cosphi1 * cossigma12 - sinphi1 * sinsigma12 * cosa1);

    float l2 = l1 + l12;

    end->lat = phi2 * (180.0 / M_PI);
    end->lon =   l2 * (180.0 / M_PI);
}

static void _find_heading(nexrad_map_point start,
                          nexrad_map_point end,
                          nexrad_map_heading *heading) {
    float phi1 = start.lat * (M_PI / 180.0),
          phi2 =   end.lat * (M_PI / 180.0),
            l1 = start.lon * (M_PI / 180.0),
            l2 =   end.lon * (M_PI / 180.0);

    float phi12 = phi2 - phi1,
            l12 =   l2 -   l1;

    float sinphi1 = sinf(phi1),
          sinphi2 = sinf(phi2),
          cosphi1 = cosf(phi1),
          cosphi2 = cosf(phi2),
          tanphi1 = tanf(phi1),
          tanphi2 = tanf(phi2);

    float sinl12 = sinf(l12),
          cosl12 = cosf(l12);

    float a1 = atan2f(sinl12,  (cosphi1 * tanphi2) - (sinphi1 * cosl12)),
          a2 = atan2f(sinl12, (-cosphi2 * tanphi1) + (sinphi2 * cosl12));

    float a = powf(sinf(phi12 / 2.0), 2.0)
        + cosphi1 * cosphi2 * powf(sinf(l12 / 2.0), 2.0);

    float c = 2.0 * atan2f(sqrtf(a), sqrtf(1.0 - a));

    heading->azimuth = a1 * (180.0 / M_PI);
    heading->range   = NEXRAD_MAP_EARTH_RADIUS * c;
}

nexrad_image *nexrad_map_project_radial(nexrad_radial *radial, nexrad_map_radar *radar, nexrad_color_table *clut, float tilt, float resolution, int zoom) {
    nexrad_image *image;

    /*
     * First, determine the north and westernmost Cartesian extents of the
     * image for the given radar location and tilt, and the resolution of the
     * radial data provided.
     */
    return NULL;
}
