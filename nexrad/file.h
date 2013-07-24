#ifndef _NEXRAD_FILE_H
#define _NEXRAD_FILE_H

#include <nexrad/message.h>

typedef struct _nexrad_message_file {
    void *                       message;
    nexrad_message_header      * header;
    nexrad_product_description * description;
    nexrad_symbology_block *     symbology;
    nexrad_graphic_block *       graphic;
} nexrad_message_file;

#endif /* _NEXRAD_FILE_H */
