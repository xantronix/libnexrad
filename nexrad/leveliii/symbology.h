#ifndef _NEXRAD_LEVELIII_SYMBOLOGY_H
#define _NEXRAD_LEVELIII_SYMBOLOGY_H

#include <stdint.h>

#include <nexrad/consts.h>

typedef struct _nexrad_leveliii_symbology {
     int16_t divider;
    uint16_t id;
    uint32_t length;
    uint16_t layers;
     int16_t layer_divider;
} nexrad_leveliii_symbology;

#endif /* _NEXRAD_LEVELIII_SYMBOLOGY_H */
