#ifndef _NEXRAD_HEADER_H
#define _NEXRAD_HEADER_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/date.h>

#pragma pack(push)
#pragma pack(1)

#define NEXRAD_HEADER_UNKNOWN_SIGNATURE "\x01\x0d\x0d\x0a"
#define NEXRAD_HEADER_WMO_SIGNATURE     "SDUS"

typedef struct _nexrad_unknown_header {
    char reserved1[4];
    char reserved2[4];
    char reserved3[3];
} nexrad_unknown_header;

typedef struct _nexrad_wmo_header {
    char region[6];
    char _whitespace1;
    char office[4];
    char _whitespace2;
    char timestamp[6];
    char _whitespace3[3];
    char product_code[3];
    char station[3];
    char _whitespace4[3];
} nexrad_wmo_header;

typedef struct _nexrad_message_header {
     int16_t    product_type; /* Product type ID */
    nexrad_date date;         /* Date of message transmission */
    uint32_t    size;         /* Size of message, including header */
    uint16_t    src_id;       /* Message source */
    uint16_t    dest_id;      /* Message destination */
    uint16_t    blocks;       /* Number of blocks in message (5 or less) */
} nexrad_message_header;

#pragma pack(pop)

#endif /* _NEXRAD_HEADER_H */
