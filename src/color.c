#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <nexrad/color.h>

static inline size_t _table_size(uint8_t size) {
    return size * sizeof(nexrad_color_table_entry);
}

static inline size_t _table_size_total(uint8_t size) {
    return sizeof(nexrad_color_table) + _table_size(size);
}

nexrad_color_table *nexrad_color_table_create(uint8_t size) {
    nexrad_color_table *table;
    size_t total_size = _table_size_total(size);

    if ((table = malloc(total_size)) == NULL) {
        goto error_malloc;
    }

    memcpy(table->magic, "CLUT", 4);

    table->size = size;

    memset((uint8_t *)table + sizeof(nexrad_color_table), '\0', _table_size(size));

    return table;

error_malloc:
    return NULL;
}

void nexrad_color_table_store_entry(nexrad_color_table *table, uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    nexrad_color_table_entry *entries;

    if (table == NULL) {
        return;
    }

    entries = (nexrad_color_table_entry *)((char *)table + sizeof(nexrad_color_table));

    entries[index].r = r;
    entries[index].g = g;
    entries[index].b = b;
}

nexrad_color_table *nexrad_color_table_open(const char *path) {
    nexrad_color_table *table;
    struct stat st;
    size_t total_size;
    int fd;

    if (path == NULL) {
        return NULL;
    }

    if ((table = malloc(sizeof(*table))) == NULL) {
        goto error_malloc;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        goto error_open;
    }

    if (fstat(fd, &st) < 0) {
        goto error_fstat;
    }

    if (st.st_size < sizeof(*table)) {
        goto error_toosmall;
    }

    if (read(fd, table, sizeof(*table)) < 0) {
        goto error_read;
    }

    /*
     * Check the magic header of the color lookup table file.  Fail if this is
     * not what we expect.
     */
    if (strncmp(table->magic, "CLUT", 4) != 0) {
        errno = EINVAL;
        goto error_invalid_clut;
    }

    /*
     * Calculate the expected total size of the file based on the header.  If the
     * actual file size differs, then fail.
     */
    total_size = _table_size_total(table->size);

    if (total_size != st.st_size) {
        goto error_invalid_size;
    }

    /*
     * On the other hand, if everything appears to be fine, then reallocate our
     * lookup table buffer and read the rest of the file.
     */
    if ((table = realloc(table, total_size)) == NULL) {
        goto error_realloc;
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        goto error_lseek;
    }

    if (read(fd, table, total_size) < 0) {
        goto error_read_full;
    }

    close(fd);

    return table;

error_read_full:
error_lseek:
error_realloc:
error_invalid_size:
error_invalid_clut:
error_read:
error_toosmall:
error_fstat:
    close(fd);

error_open:
    free(table);

error_malloc:
    return NULL;
}

void nexrad_color_table_close(nexrad_color_table *table) {
    size_t total_size;

    if (table == NULL) {
        return;
    }

    total_size = _table_size_total(table->size);

    memset(table, '\0', total_size);

    free(table);
}
