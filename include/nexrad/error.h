#ifndef _NEXRAD_ERROR_H
#define _NEXRAD_ERROR_H

typedef struct _nexrad_error {
    int    set;
    char * message;
    size_t len;
} nexrad_error;

nexrad_error *nexrad_error_new();

int nexrad_error_isset(nexrad_error *error);

char *nexrad_error_get(nexrad_error *error, size_t *len);

char *nexrad_error_set(nexrad_error *error,
    char *message,
    size_t len
);

void nexrad_error_clear(nexrad_error *error);

void nexrad_error_destroy(nexrad_error *error);

#endif /* _NEXRAD_ERROR_H */
