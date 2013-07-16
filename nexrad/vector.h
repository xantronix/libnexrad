#ifndef _NEXRAD_VECTOR_H
#define _NEXRAD_VECTOR_H

#include <stdint.h>

typedef struct _nexrad_vector {
    uint16_t code;
    uint16_t size;
    uint16_t magnitude; /* in 1/4km increments */
     int16_t i1_start;
     int16_t j1_start;
     int16_t i1_end;
     int16_t j1_end;
     int16_t i2_start;
     int16_t j2_start;
     int16_t i2_end;
     int16_t j2_end;
} nexrad_vector;

#endif /* _NEXRAD_VECTOR_H */
