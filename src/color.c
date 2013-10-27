#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <nexrad/color.h>

static inline size_t _table_size(size_t size) {
    return size * sizeof(nexrad_color_table_entry);
}

static inline size_t _table_size_total(size_t size) {
    return sizeof(nexrad_color_table) + _table_size(size);
}

nexrad_color_table *nexrad_color_table_create(size_t size) {
    nexrad_color_table *table;
    size_t table_size = _table_size_total(size);

    if (size > NEXRAD_COLOR_TABLE_MAX_SIZE) {
        errno = EINVAL;
        return NULL;
    }

    if ((table = malloc(table_size)) == NULL) {
        goto error_malloc;
    }

    memcpy(table->magic, "CLUT", 4);

    table->size = size;

    memset((uint8_t *)table + sizeof(nexrad_color_table), '\0', _table_size(size));

    return table;

error_malloc:
    return NULL;
}

void nexrad_color_table_store_entry(nexrad_color_table *table, uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    nexrad_color_table_entry *entries;

    if (table == NULL) {
        return;
    }

    entries = (nexrad_color_table_entry *)((char *)table + sizeof(nexrad_color_table));

    entries[index].r = r;
    entries[index].g = g;
    entries[index].b = b;
    entries[index].a = a;
}

nexrad_color_table *nexrad_color_table_load(const char *path) {
    nexrad_color_table header, *table;
    struct stat st;
    size_t table_size;
    int fd;

    if (path == NULL) {
        return NULL;
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

    if (read(fd, &header, sizeof(header)) < 0) {
        goto error_read;
    }

    /*
     * Check the magic header of the color lookup table file.  Fail if this is
     * not what we expect.
     */
    if (strncmp(header.magic, "CLUT", 4) != 0) {
        errno = EINVAL;
        goto error_invalid_clut;
    }

    /*
     * Ensure the table size recorded in the file does not exceed the maximum
     * color table size.
     */
    if (header.size > NEXRAD_COLOR_TABLE_MAX_SIZE) {
        errno = EINVAL;
        goto error_invalid_size;
    }

    /*
     * Calculate the expected total size of the file based on the header.  If the
     * actual file size differs, then fail.
     */
    table_size = _table_size_total(header.size);

    if (table_size != st.st_size) {
        goto error_invalid_size;
    }

    /*
     * On the other hand, if everything looks fine, allocate the whole table and
     * read the rest of the file.
     */
    if ((table = malloc(table_size)) == NULL) {
        goto error_malloc;
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        goto error_lseek;
    }

    if (read(fd, table, table_size) < 0) {
        goto error_read_full;
    }

    close(fd);

    return table;

error_read_full:
error_lseek:
    free(table);

error_malloc:
error_invalid_size:
error_invalid_clut:
error_read:
error_toosmall:
error_fstat:
    close(fd);

error_open:
    return NULL;
}

nexrad_color_table_entry *nexrad_color_table_get_entries(nexrad_color_table *table, size_t *size) {
    if (table == NULL) {
        return NULL;
    }

    if (size)
        *size = table->size;

    return (nexrad_color_table_entry *)((char *)table + sizeof(nexrad_color_table));
}

int nexrad_color_table_save(nexrad_color_table *table, const char *path) {
    int fd;
    size_t table_size;

    if (table == NULL || path == NULL) {
        return -1;
    }

    table_size = _table_size_total(table->size);

    if ((fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666)) < 0) {
        goto error_open;
    }

    if (write(fd, table, table_size) < 0) {
        goto error_write;
    }

    close(fd);

    return 0;

error_write:
    close(fd);

error_open:
    return -1;
}

void nexrad_color_table_destroy(nexrad_color_table *table) {
    size_t table_size;

    if (table == NULL) {
        return;
    }

    table_size = _table_size_total(table->size);

    memset(table, '\0', table_size);

    free(table);
}
