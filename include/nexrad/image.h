#ifndef _NEXRAD_IMAGE_H
#define _NEXRAD_IMAGE_H

enum nexrad_image_type {
    NEXRAD_IMAGE_RASTER,
    NEXRAD_IMAGE_RADIAL
};

enum nexrad_image_depth {
    NEXRAD_IMAGE_32BPP = 4,
    NEXRAD_IMAGE_24BPP = 3,
    NEXRAD_IMAGE_16BPP = 2,
    NEXRAD_IMAGE_8BPP  = 1
};

enum nexrad_image_color {
    NEXRAD_IMAGE_GRAYSCALE       = 0,
    NEXRAD_IMAGE_TRUECOLOR       = 2,
    NEXRAD_IMAGE_INDEXED         = 3,
    NEXRAD_IMAGE_GRAYSCALE_ALPHA = 4,
    NEXRAD_IMAGE_TRUECOLOR_ALPHA = 6
};

typedef struct _nexrad_image nexrad_image;

nexrad_image * nexrad_image_create(
    size_t width, size_t height, enum nexrad_image_depth depth, enum nexrad_image_color color
);

int nexrad_image_get_info(nexrad_image *image,
    size_t *width, size_t *height, enum nexrad_image_depth *depth, enum nexrad_image_color *color
);

ssize_t nexrad_image_get_size(nexrad_image *image);

unsigned char * nexrad_image_get_buf(nexrad_image *image);

void nexrad_image_destroy(nexrad_image *image);

#endif /* _NEXRAD_IMAGE_H */
