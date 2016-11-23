/*
 * Copyright (c) 2016 Dynamic Weather Solutions, Inc. Distributed under the
 * terms of the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

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
