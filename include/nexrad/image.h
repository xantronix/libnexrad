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
    size_t width,
    size_t height,
    enum nexrad_image_depth depth,
    enum nexrad_image_color color
);

int nexrad_image_get_info(nexrad_image *image,
    size_t *width,
    size_t *height,
    enum nexrad_image_depth *depth,
    enum nexrad_image_color *color
);

unsigned char * nexrad_image_get_buf(nexrad_image *image, size_t *size);

void nexrad_image_draw_arc_section(nexrad_image *image,
    int level,
    float angle_min,
    float angle_max,
    float radius_min,
    float radius_max
);

int nexrad_image_save_png(nexrad_image *image, const char *path);

void nexrad_image_destroy(nexrad_image *image);

#endif /* _NEXRAD_IMAGE_H */
