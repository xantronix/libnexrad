#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <nexrad/archive.h>
#include "../src/util.h"

static void usage(int argc, char **argv, const char *message, ...) {
    if (message) {
        va_list args;

        va_start(args, message);
        vfprintf(stderr, message, args);
        fputc('\n', stderr);
        va_end(args);
    }

    fprintf(stderr, "usage: %s L2_ARCHIVE_FILE\n", argv[0]);

    exit(1);
}

int main(int argc, char **argv) {
    int fd;
    ssize_t readlen;
    off_t total;

    nexrad_archive_header archive_header;
    nexrad_archive_message_header message_header;

    if (argc != 2) {
        usage(argc, argv, "No Level II archive file provided");
    }

    if ((fd = open(argv[1], O_RDONLY)) < 0) {
        fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1], strerror(errno));

        goto error_open;
    }

    total = 0;

    if ((readlen = read(fd, &archive_header, sizeof(archive_header))) < 0) {
        fprintf(stderr, "%s: %s: %s: %s\n",
            argv[0], argv[1], "read()", strerror(errno));

        goto error_io;
    }

    total += readlen;

    printf("ICAO %.*s date %"PRIu32" msec %" PRIu32 "\n",
        4,
        archive_header.icao,
        be32toh(archive_header.date),
        be32toh(archive_header.ms));

    while (1) {
        size_t len;
        off_t seeklen;

        if ((readlen = read(fd, &message_header, sizeof(message_header))) < 0) {
            fprintf(stderr, "%s: %s: %s: %s\n",
                argv[0], argv[1], "read()", strerror(errno));

            goto error_io;
        } else if (readlen == 0) {
            break;
        } else if (readlen != sizeof(message_header)) {
            fprintf(stderr, "%s: %s: short read\n",
                argv[0], argv[1]);

            goto error_io;
        }

        total += readlen;

        printf("Message halfwords %"PRIu16" channel %"PRIu8" type %"PRIu8"\n",
            be16toh(message_header.halfwords),
            message_header.rda_redundant_channel,
            message_header.type);

        len = be16toh(message_header.halfwords) << 1;

        if ((seeklen = lseek(fd, len, SEEK_CUR)) < 0) {
            fprintf(stderr, "%s: %s: %s: %s\n",
                argv[0], argv[1], "lseek()", strerror(errno));

            goto error_io;
        }

        total += len;

        if (seeklen != total) {
            fprintf(stderr, "seeklen %lld total %lld\n",
                seeklen, total);
            fprintf(stderr, "%s: %s: short seek\n",
                argv[0], argv[1]);

            goto error_io;
        }
    }

    close(fd);

    return 0;

error_io:
    close(fd);

error_open:
    return 1;
}
