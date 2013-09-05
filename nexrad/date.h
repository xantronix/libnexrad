#ifndef _NEXRAD_MESSAGE_TYPES_H
#define _NEXRAD_MESSAGE_TYPES_H

#include <stdint.h>
#include <time.h>

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_date {
    uint16_t epoch_day;  /* Days since Unix epoch */
    uint32_t gmt_second; /* Seconds since midnight GMT */
} nexrad_date;

#pragma pack(pop)

time_t nexrad_date_timestamp(nexrad_date *date);

#endif /* _NEXRAD_MESSAGE_TYPES_H */
