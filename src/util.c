#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "util.h"

int safecpy(char *dest, char *src, size_t destlen, size_t srclen) {
    size_t copylen = 0;

    if (dest == NULL || src == NULL) {
        return -1;
    }

    if (destlen > srclen) {
        copylen = srclen;
    } else if (destlen <= srclen) {
        copylen = destlen - 1;
    }

    memset(dest, '\0', destlen);
    memcpy(dest, src,  copylen);

    return 0;
}
