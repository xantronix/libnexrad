#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <nexrad/color.h>

static nexrad_color_table_entry nexrad_clut4_entries[] = {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0xec, 0xec },
    { 0x01, 0xa0, 0xf6 },
    { 0x00, 0x00, 0xf6 },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0xc8, 0x00 },
    { 0x00, 0x90, 0x00 },
    { 0xff, 0xff, 0x00 },
    { 0xe7, 0xc0, 0x00 },
    { 0xff, 0x90, 0x00 },
    { 0xff, 0x00, 0x00 },
    { 0xd6, 0x00, 0x00 },
    { 0xc0, 0x00, 0x00 },
    { 0xff, 0x00, 0xff },
    { 0x99, 0x55, 0xc9 },
    { 0xff, 0xff, 0xff }
};

static nexrad_color_table *create_reflectivity_table() {
    nexrad_color_table *table;
    int i;

    if ((table = nexrad_color_table_create(256)) == NULL) {
        goto error_color_table_create;
    }

    for (i=0; i<16; i++) {
        int x;

        for (x=0; x<16; x++) {
            nexrad_color_table_store_entry(table,
                i*16 + x, nexrad_clut4_entries[i].r, nexrad_clut4_entries[i].g, nexrad_clut4_entries[i].b
            );
        }
    }

    return table;

error_color_table_create:
    return NULL;
}

int main(int argc, char **argv) {
    nexrad_color_table *table;

    if ((table = create_reflectivity_table()) == NULL) {
        perror("create_reflectivity_table()");
        exit(1);
    }

    if (nexrad_color_table_save(table, "reflectivity.clut") < 0) {
        perror("nexrad_color_table_save()");
        exit(1);
    }

    nexrad_color_table_destroy(table);

    return 0;
}
