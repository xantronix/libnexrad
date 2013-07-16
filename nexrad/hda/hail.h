#ifndef _NEXRAD_HDA_HAIL_H
#define _NEXRAD_HDA_HAIL_H

#include <nexrad/packet.h>

typedef struct _nexrad_hda_hail {
    uint16_t code;
    uint16_t size;
     int16_t i;
     int16_t j;
    uint16_t prob_hail;
    uint16_t prob_hail_severe;
    uint16_t max_hail_size;
} nexrad_hda_hail;

#endif /* _NEXRAD_HDA_HAIL_H */
