#ifndef _NEXRAD_IMAGE_H
#define _NEXRAD_IMAGE_H

#include <sys/types.h>
#include <stdint.h>

typedef struct _nexrad_image nexrad_image;

nexrad_image *nexrad_image_create(
    size_t width,
    size_t height
);

int nexrad_image_get_info(nexrad_image *image,
    size_t *width,
    size_t *height
);

unsigned char *nexrad_image_get_buf(nexrad_image *image, size_t *size);

void nexrad_image_draw_arc_segment(nexrad_image *image,
    uint8_t level,
    int amin,
    int amax,
    int rmin,
    int rmax
);

int nexrad_image_save_png(nexrad_image *image, const char *path);

void nexrad_image_destroy(nexrad_image *image);

#endif /* _NEXRAD_IMAGE_H */
