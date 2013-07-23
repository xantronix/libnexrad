#ifndef _NEXRAD_FILE_H
#define _NEXRAD_FILE_H

typedef struct _nexrad_file {
    int fd;
} nexrad_file;

nexrad_file * nexrad_file_open(const char *filename);

#endif /* _NEXRAD_FILE_H */
