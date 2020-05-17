#ifndef _NEXRAD_ARCHIVE_H
#define _NEXRAD_ARCHIVE_H

/*
 * This header implements the format of the NEXRAD Level II volume scan product
 * file and block headers as defined in the following official Radar Operations
 * Center document:
 *
 * http://www.unidata.ucar.edu/data/WSR-88D_ICD.pdf
 */

#include <stdint.h>

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_archive_header {
    char filename[8],
         dot,
         extension[3];

    uint32_t date;
    uint32_t ms;

    char icao[4];
} nexrad_archive_header;

typedef struct _nexrad_archive_message_header {
    uint8_t _reserved1[12];
    uint16_t halfwords;
    uint8_t  rda_redundant_channel;
    uint8_t  type;
    uint16_t sequence;
    uint16_t date;
    uint32_t ms;
    uint16_t segments;
    uint16_t segment;
} nexrad_archive_message_header;

#pragma pack(pop)

#endif /* _NEXRAD_ARCHIVE_H */
