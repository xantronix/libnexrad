#include <stdlib.h>
#include <sys/types.h>

#include <nexrad/image.h>

struct _nexrad_image {
    unsigned char * buf;
    size_t          size;

    size_t width;
    size_t height;

    enum nexrad_image_depth depth;
    enum nexrad_image_color color;
    enum nexrad_image_type  type;
};

static size_t _image_size(int width, int height, enum nexrad_image_depth depth) {
    return width * height * depth;
}

static int _valid_args(int width, int height, enum nexrad_image_depth depth, enum nexrad_image_color color) {
    if (width == 0 || height == 0) {
        return 0;
    }

    switch (depth) {
        case NEXRAD_IMAGE_32BPP:
        case NEXRAD_IMAGE_24BPP:
        case NEXRAD_IMAGE_16BPP:
        case NEXRAD_IMAGE_8BPP: {
            break;
        }

        default: {
            return 0;
        }
    }

    switch (color) {
        case NEXRAD_IMAGE_GRAYSCALE:
        case NEXRAD_IMAGE_TRUECOLOR:
        case NEXRAD_IMAGE_INDEXED:
        case NEXRAD_IMAGE_GRAYSCALE_ALPHA:
        case NEXRAD_IMAGE_TRUECOLOR_ALPHA: {
            break;
        }

        default: {
            return 0;
        }
    }

    return 1;
}

nexrad_image *nexrad_image_create(int width, int height, enum nexrad_image_depth depth, enum nexrad_image_color color) {
    nexrad_image *image;
    size_t size;
    unsigned char *buf;

    if (!_valid_args(width, height, depth, color)) {
        return NULL;
    }

    if ((image = malloc(sizeof(nexrad_image))) == NULL) {
        goto error_malloc_image;
    }

    size = _image_size(width, height, depth);

    if ((buf = malloc(size)) == NULL) {
        goto error_malloc_buf;
    }

    image->buf    = buf;
    image->size   = size;
    image->width  = width;
    image->height = height;
    image->depth  = depth;
    image->color  = color;

    return image;

error_malloc_buf:
    free(image);

error_malloc_image:
    return NULL;
}

ssize_t nexrad_image_get_size(nexrad_image *image) {
    if (image == NULL) {
        return -1;
    }

    return image->size;
}

unsigned char * nexrad_image_get_buf(nexrad_image *image) {
    if (image == NULL) {
        return NULL;
    }

    return image->buf;
}

void nexrad_image_destroy(nexrad_image *image) {
    if (image == NULL) {
        return;
    }

    if (image->buf) {
        free(image->buf);
    }

    image->buf    = NULL;
    image->size   = 0;
    image->width  = 0;
    image->height = 0;
    image->depth  = 0;
    image->color  = 0;

    free(image);
}
