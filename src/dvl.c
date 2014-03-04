#include <math.h>
#include <nexrad/dvl.h>

int nexrad_dvl_valid(uint8_t v) {
    if (v == 0)
        return 0;

    return 1;
}

double nexrad_dvl_vil(uint8_t v) {
    double ret = 0.0;

    if (v < NEXRAD_DVL_LOG_START)
        ret = ((double)v - NEXRAD_DVL_OFFSET) / NEXRAD_DVL_SCALE;
    else
        ret = pow(M_E, ((double)v - NEXRAD_DVL_LOG_OFFSET) / NEXRAD_DVL_LOG_SCALE);
        
    return ret;
}
