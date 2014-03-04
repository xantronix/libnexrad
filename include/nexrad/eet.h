#ifndef _NEXRAD_EET_H
#define _NEXRAD_EET_H

#include <stdint.h>

#define NEXRAD_EET_OFFSET         2
#define NEXRAD_EET_METERS       304.8
#define NEXRAD_EET_VALUE_MASK  0x7f
#define NEXRAD_EET_TOPPED_FLAG (1 << 7)

int nexrad_eet_valid(uint8_t v);

int nexrad_eet_topped(uint8_t v);

double nexrad_eet_meters(uint8_t v);

#endif /* _NEXRAD_EET_H */
