#ifndef _NEXRAD_MESSAGE_FILE_H
#define _NEXRAD_MESSAGE_FILE_H

typedef struct _nexrad_file_header {
    char region[6];
    char _whitespace1;
    char office[4];
    char _whitespace2;
    char timestamp[6];
    char _whitespace3[3];
    char product[3];
    char station[3];
    char _whitespace4[2];
    char nul;
} nexrad_file_header;

#endif /* _NEXRAD_MESSAGE_FILE_H */
