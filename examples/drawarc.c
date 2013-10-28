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
        int level = 0x10;
        int r;

        for (r=0; r<180; r+=8) {
            if (level >= 0xf0) level = 0x10;
            nexrad_color color = { level, level % 0x3f, level % 0x7f, 0xff };

            nexrad_image_draw_arc_segment(image, color, a, a + 35, r, r+2);

            level += 0x10;
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
