#ifndef _NEXRAD_IMAGE_H
#define _NEXRAD_IMAGE_H

#include <stdint.h>
#include <sys/types.h>

typedef struct _nexrad_image nexrad_image;

nexrad_image *nexrad_image_create(
    uint16_t width,
    uint16_t height
);

int nexrad_image_get_info(nexrad_image *image,
    uint16_t *width,
    uint16_t *height
);

unsigned char *nexrad_image_get_buf(nexrad_image *image, size_t *size);

void nexrad_image_draw_pixel(nexrad_image *image,
    uint8_t r, uint8_t g, uint8_t b,
    uint16_t x, uint16_t y
);

void nexrad_image_draw_run(nexrad_image *image,
    uint8_t r, uint8_t g, uint8_t b,
    uint16_t x, uint16_t y,
    uint16_t length
);

void nexrad_image_draw_arc_segment(nexrad_image *image,
    uint8_t r, uint8_t g, uint8_t b,
    int amin, int amax,
    int rmin, int rmax
);

int nexrad_image_save_png(nexrad_image *image, const char *path);

void nexrad_image_destroy(nexrad_image *image);

#endif /* _NEXRAD_IMAGE_H */
