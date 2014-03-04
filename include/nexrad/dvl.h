#ifndef _NEXRAD_DVL_H
#define _NEXRAD_DVL_H

#include <stdint.h>

#define NEXRAD_DVL_KG_MSQ     1000.0
#define NEXRAD_DVL_SCALE        90.6875
#define NEXRAD_DVL_OFFSET        2.0
#define NEXRAD_DVL_LOG_START    20
#define NEXRAD_DVL_LOG_SCALE    38.875
#define NEXRAD_DVL_LOG_OFFSET   83.875

int nexrad_dvl_valid(uint8_t v);

double nexrad_dvl_vil(uint8_t v);

#endif /* _NEXRAD_DVL_H */
