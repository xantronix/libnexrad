#include <nexrad/eet.h>

int nexrad_eet_valid(uint8_t v) {
    switch (v & NEXRAD_EET_VALUE_MASK) {
        case 0:
        case 1:
            return 0;
    }

    return 1;
}

int nexrad_eet_topped(uint8_t v) {
    if (v & NEXRAD_EET_TOPPED_FLAG)
        return 1;

    return 0;
}

double nexrad_eet_meters(uint8_t v) {
    return NEXRAD_EET_METERS
        * (double)((v & NEXRAD_EET_VALUE_MASK) - NEXRAD_EET_OFFSET);
}
