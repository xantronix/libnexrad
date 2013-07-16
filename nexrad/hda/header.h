#ifndef _NEXRAD_HDA_HEADER_H
#define _NEXRAD_HDA_HEADER_H

#include <stdint.h>

typedef struct _nexrad_hda_header {
    uint16_t code;
    uint16_t size;
} nexrad_hda_header;

#endif /* _NEXRAD_HDA_HEADER_H */
