#ifndef _NEXRAD_MESSAGE_TABULAR_H
#define _NEXRAD_MESSAGE_TABULAR_H

#include <stdint.h>
#include <sys/types.h>

#include <nexrad/message/block.h>

#pragma pack(push)
#pragma pack(1)

typedef struct _nexrad_tabular_block {
    nexrad_block_header header;

    /*
     * For some inexplicable reason, the tabular alphanumeric block duplicates
     * the NEXRAD product message header and product description blocks.  Of
     * particular forensic interest is the fact that this second product
     * description block refers only to an offset to the previous product
     * symbology block, but provides null values for the offsets to the graphic
     * product message block and tabular alphanumeric block.  It has not yet
     * been determined what the significance of this construct is.
     */
    nexrad_message_header      message_header;
    nexrad_product_description product_description;

     int16_t divider; /* Standard block divider */
    uint16_t pages;   /* Number of pages to follow */
} nexrad_tabular_block;

#pragma pack(pop)

#define NEXRAD_TABULAR_BLOCK_MAX_LINE_SIZE 80

typedef struct _nexrad_tabular_text {
    char * current;    /* Current pointer */
    int    page;       /* Current page number */
    int    line;       /* Line number in current page */
    int    pages_left; /* Number of pages left in text */
    size_t bytes_left; /* Number of bytes left in text */
} nexrad_tabular_text;

#endif /* _NEXRAD_MESSAGE_TABULAR_H */
