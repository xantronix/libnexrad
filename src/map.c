#include <stdlib.h>
#include <math.h>

#include <nexrad/map.h>

void nexrad_map_find_point(nexrad_map_point start,
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

void nexrad_map_find_heading(nexrad_map_point start,
                             nexrad_map_point end,
                             nexrad_map_heading *heading) {
    float phi1 = start.lat * (M_PI / 180.0),
          phi2 =   end.lat * (M_PI / 180.0),
            l1 = start.lon * (M_PI / 180.0),
            l2 =   end.lon * (M_PI / 180.0);

    float phi12 = phi2 - phi1,
            l12 =   l2 -   l1;

    float sinphi1 = sinf(phi1),
          cosphi1 = cosf(phi1),
          cosphi2 = cosf(phi2),
          tanphi2 = tanf(phi2);

    float sinl12 = sinf(l12),
          cosl12 = cosf(l12);

    float a1 = atan2f(sinl12,  (cosphi1 * tanphi2) - (sinphi1 * cosl12));

    float a = powf(sinf(phi12 / 2.0), 2.0)
        + cosphi1 * cosphi2 * powf(sinf(l12 / 2.0), 2.0);

    float c = 2.0 * atan2f(sqrtf(a), sqrtf(1.0 - a));

    heading->azimuth = a1 * (180.0 / M_PI);
    heading->range   = NEXRAD_MAP_EARTH_RADIUS * c;
}

static inline float _mercator_find_lon(size_t x, size_t width) {
    return (360.0 * ((float)x / (float)width)) - 180.0;
}

static size_t _mercator_find_x(float lon, size_t width) {
    return (size_t)roundf((float)width * ((lon + 180.0) / 360.0));
}

static float _mercator_find_lat(size_t y, size_t height) {
    static const float deg = 180.0 / M_PI;

    size_t cy = height / 2;
    float r   = M_PI * (((float)cy - (float)y) / (float)cy);

    return deg * atanf(sinhf(r));
}

static size_t _mercator_find_y(float lat, size_t height) {
    static const float rad = M_PI / 180;

    size_t cy  = height / 2;
    float sign = (lat >= 0)? 1: -1;

    float sinl = sinf(sign * rad * lat);
    float yrad = sign * logf((1.0 + sinl) / (1.0 - sinl)) / 2.0;

    return cy - (size_t)roundf(height * (yrad / (2.0 * M_PI)));
}

nexrad_image *nexrad_map_project_radial(nexrad_radial *radial,
                                        nexrad_map_point *radar,
                                        nexrad_map_point *extents,
                                        nexrad_color_table *clut,
                                        float tilt,
                                        float resolution,
                                        int zoom) {
    float range_factor = NEXRAD_MAP_REFRACTION_FACTOR
        * cosf(tilt * (M_PI / 180.0));

    size_t world_size, width, height,
        world_x, world_y;

    size_t x, y;

    nexrad_image *image;

    nexrad_map_heading heading = {
        .range = radial->bins * resolution * range_factor
    };

    /*
     * First, determine the Cartesian extents of the image for the given radar
     * location and range factor based on tilt, resolution and refraction.
     */
    heading.azimuth =   0.0;
    nexrad_map_find_point(*radar, heading, &extents[0]);

    heading.azimuth =  90.0;
    nexrad_map_find_point(*radar, heading, &extents[1]);

    heading.azimuth = 180.0;
    nexrad_map_find_point(*radar, heading, &extents[2]);

    heading.azimuth = 270.0;
    nexrad_map_find_point(*radar, heading, &extents[3]);

    /*
     * Next, determine the width and height of the output image.
     */
    world_size = NEXRAD_MAP_TILE_SIZE * pow(2, zoom);
    world_x    = _mercator_find_x(extents[3].lon, world_size);
    world_y    = _mercator_find_y(extents[0].lat, world_size);
    width      = _mercator_find_x(extents[1].lon, world_size) - world_x;
    height     = _mercator_find_y(extents[2].lat, world_size) - world_y;

    if ((image = nexrad_image_create(width, height)) == NULL) {
        goto error_image_create;
    }

    for (y=0; y<height; y++) {
        float lat = _mercator_find_lat(y + world_y, world_size);

        for (x=0; x<width; x++) {
            nexrad_map_point point = {
                .lat = lat,
                .lon = _mercator_find_lon(x + world_x, world_size)
            };

            size_t pixel = NEXRAD_IMAGE_PIXEL_BYTES * (y * width + x);

            uint16_t a, r;
             uint8_t v;

            nexrad_map_find_heading(*radar, point, &heading);

            while (heading.azimuth >= 360.0) heading.azimuth -= 360.0;
            while (heading.azimuth <    0.0) heading.azimuth += 360.0;

            a = (uint16_t)abs((int)floorf(heading.azimuth * 10));
            r = (uint16_t)roundf((heading.range * range_factor) / resolution);

            if (r >= radial->bins)
                continue;

            v = ((uint8_t *)(radial + 1))[a*radial->bins+r];

            image->buf[pixel]   = ((nexrad_color *)(clut + 1))[v].r;
            image->buf[pixel+1] = ((nexrad_color *)(clut + 1))[v].g;
            image->buf[pixel+2] = ((nexrad_color *)(clut + 1))[v].b;
            image->buf[pixel+3] = ((nexrad_color *)(clut + 1))[v].a;
        }
    }

    return image;

error_image_create:
    return NULL;
}
