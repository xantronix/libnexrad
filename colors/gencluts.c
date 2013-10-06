#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <nexrad/color.h>

static nexrad_color_table_entry reflectivity_clut4_entries[] = {
    { 0x00, 0x00, 0x00 }, { 0x00, 0xec, 0xec }, { 0x01, 0xa0, 0xf6 }, { 0x00, 0x00, 0xf6 },
    { 0x00, 0xff, 0x00 }, { 0x00, 0xc8, 0x00 }, { 0x00, 0x90, 0x00 }, { 0xff, 0xff, 0x00 },
    { 0xe7, 0xc0, 0x00 }, { 0xff, 0x90, 0x00 }, { 0xff, 0x00, 0x00 }, { 0xd6, 0x00, 0x00 },
    { 0xc0, 0x00, 0x00 }, { 0xff, 0x00, 0xff }, { 0x99, 0x55, 0xc9 }, { 0xff, 0xff, 0xff }
};

static nexrad_color_table_entry velocity_clut4_entries[] = {
    { 0x00, 0x00, 0x00 }, { 0x02, 0xfc, 0x02 }, { 0x01, 0xe4, 0x01 }, { 0x01, 0xc5, 0x01 },
    { 0x07, 0xac, 0x04 }, { 0x06, 0x8f, 0x03 }, { 0x04, 0x72, 0x02 }, { 0x7c, 0x97, 0x7b },
    { 0x98, 0x77, 0x77 }, { 0x89, 0x00, 0x00 }, { 0xa2, 0x00, 0x00 }, { 0xb9, 0x00, 0x00 },
    { 0xd8, 0x00, 0x00 }, { 0xef, 0x00, 0x00 }, { 0xfe, 0x00, 0x00 }, { 0x90, 0x00, 0xa0 }
};

static nexrad_color_table *create_full_table(nexrad_color_table_entry *entries) {
    nexrad_color_table *table;
    int i;

    if ((table = nexrad_color_table_create(256)) == NULL) {
        goto error_color_table_create;
    }

    for (i=0; i<16; i++) {
        int x;

        for (x=0; x<16; x++) {
            nexrad_color_table_store_entry(table,
                i*16 + x,
                entries[i].r,
                entries[i].g,
                entries[i].b
            );
        }
    }

    return table;

error_color_table_create:
    return NULL;
}

static int create_table(nexrad_color_table_entry *entries, const char *path) {
    nexrad_color_table *table;

    if ((table = create_full_table(entries)) == NULL) {
        goto error_create_full_table;
    }

    if (nexrad_color_table_save(table, path) < 0) {
        goto error_color_table_save;
    }

    nexrad_color_table_destroy(table);

    return 0;

error_color_table_save:
    nexrad_color_table_destroy(table);

error_create_full_table:
    return -1;
}

int main(int argc, char **argv) {
    if (create_table(reflectivity_clut4_entries, "reflectivity.clut") < 0) {
        perror("create_table()");
        exit(1);
    }

    if (create_table(velocity_clut4_entries, "velocity.clut") < 0) {
        perror("create_table()");
        exit(1);
    }

    return 0;
}
