#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <nexrad/image.h>

static void usage(int argc, char **argv) {
    fprintf(stderr, "usage: %s outfile.png\n", argv[0]);
    exit(1);
}

int main(int argc, char **argv) {
    nexrad_image *image;
    char *path;

    if (argc != 2) {
        usage(argc, argv);
    }

    path = argv[1];

    if ((image = nexrad_image_create(360, 360, NEXRAD_IMAGE_8BPP, NEXRAD_IMAGE_GRAYSCALE)) == NULL) {
        goto error_image_create;
    }

    nexrad_image_draw_arc_section(image, 0x7f, 0.0, 1.0, 13, 34);

    if (nexrad_image_save_png(image, path) < 0) {
        goto error_image_save_png;
    }

    nexrad_image_destroy(image);

    return 0;

error_image_save_png:
    nexrad_image_destroy(image);

error_image_create:
    perror("nexrad_image_create()");

    return 1;
}
