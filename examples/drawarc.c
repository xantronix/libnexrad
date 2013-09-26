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

    if ((image = nexrad_image_create(920, 920, NEXRAD_IMAGE_8BPP, NEXRAD_IMAGE_GRAYSCALE)) == NULL) {
        goto error_image_create;
    }

    nexrad_image_draw_arc_segment(image, 0x1a,   0,  35, 164, 180);
    nexrad_image_draw_arc_segment(image, 0x30,  45,  80, 164, 180);
    nexrad_image_draw_arc_segment(image, 0x50,  90, 125, 164, 180);
    nexrad_image_draw_arc_segment(image, 0x70, 135, 170, 164, 180);
    nexrad_image_draw_arc_segment(image, 0x90, 180, 215, 164, 180);
    nexrad_image_draw_arc_segment(image, 0xb0, 225, 260, 164, 180);
    nexrad_image_draw_arc_segment(image, 0xc0, 270, 305, 164, 180);
    nexrad_image_draw_arc_segment(image, 0xf0, 315, 350, 164, 180);

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
