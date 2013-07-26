#ifndef _NEXRAD_TYPES_H
#define _NEXRAD_TYPES_H

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_date {
    uint16_t epoch_day;  /* Days since Unix epoch */
    uint32_t gmt_second; /* Seconds since midnight GMT */
} nexrad_date;

#pragma pack(pop)

#endif /* _NEXRAD_TYPES_H */
