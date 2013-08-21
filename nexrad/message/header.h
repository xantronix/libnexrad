#ifndef _NEXRAD_MESSAGE_HEADER_H
#define _NEXRAD_MESSAGE_HEADER_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/message/types.h>

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_message_header {
     int16_t    type;    /* Product type */
    nexrad_date date;    /* Date of message transmission */
    uint32_t    size;    /* Size of message, including header */
    uint16_t    src_id;  /* Message source */
    uint16_t    dest_id; /* Message destination */
    uint16_t    blocks;  /* Number of blocks in message (5 or less) */
} nexrad_message_header;

#pragma pack(pop)

#endif /* _NEXRAD_MESSAGE_HEADER_H */
