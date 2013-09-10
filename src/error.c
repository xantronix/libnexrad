#include <stdlib.h>
#include <string.h>

#include <nexrad/error.h>

nexrad_error *nexrad_error_new() {
    nexrad_error *error;

    if ((error = malloc(sizeof(nexrad_error))) == NULL) {
        goto error_malloc;
    }

    error->set     = 0;
    error->message = NULL;
    error->len     = 0;

    return error;

error_malloc:
    return NULL;
}

int nexrad_error_isset(nexrad_error *error) {
    if (error == NULL) {
        return -1;
    }

    return error->set;
}

char *nexrad_error_get(nexrad_error *error, size_t *len) {
    if (error == NULL) {
        return NULL;
    }

    if (error->message == NULL) {
        return NULL;
    }

    if (len != NULL) {
        *len = error->len;
    }

    return error->message;
}

char *nexrad_error_set(nexrad_error *error, char *message, size_t len) {
    if (error == NULL) {
        return NULL;
    }

    if (message == NULL) {
        return NULL;
    }

    if (error->set) {
        nexrad_error_clear(error);
    }

    if ((error->message = strndup(message, len)) == NULL) {
        goto error_strndup;
    }

    error->set = 1;
    error->len = len;

    return error->message;

error_strndup:
    return NULL;
}

void nexrad_error_clear(nexrad_error *error) {
    if (error == NULL) {
        return;
    }

    free(error->message);

    error->set     = 0;
    error->message = NULL;
    error->len     = 0;
}

void nexrad_error_destroy(nexrad_error *error) {
    if (error == NULL) {
        return;
    }

    nexrad_error_clear(error);

    free(error);
}
