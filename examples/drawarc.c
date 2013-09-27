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

    if ((image = nexrad_image_create(920, 920)) == NULL) {
        goto error_image_create;
    }

    int a;

    for (a=0; a<360; a+=45) {
        int color = 0x10;
        int r;

        for (r=0; r<180; r+=16) {
            if (color >= 0xf0) color = 0x10;

            nexrad_image_draw_arc_segment(image, color, color, color, a, a + 35, r - 16, r);

            color += 0x10;
        }
    }

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
