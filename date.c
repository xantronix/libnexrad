#include <endian.h>

#include <nexrad/date.h>

time_t nexrad_date_timestamp(nexrad_date *date) {
    if (date == NULL) {
        return -1;
    }

    return ((be16toh(date->epoch_day) - 1) * 86400) + be32toh(date->gmt_second);
}
