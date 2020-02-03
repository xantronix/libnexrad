/*
 * Copyright (c) 2016 Dynamic Weather Solutions, Inc. Distributed under the
 * terms of the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdlib.h>
#include "util.h"

#include <nexrad/product.h>

static enum nexrad_product_type compressed_products[] = {
     32,  94,  99, 134, 135, 136, 138, 149, 152,
    153, 154, 155, 159, 161, 163, 165, 170, 172,
    173, 174, 175, 176, 177, 194, 195, 199,   0
};

#define NEXRAD_PRODUCT_SPEC_TYPE_MAX 155

#define NEXRAD_PRODUCT_SPEC_TABLE_SIZE \
    ((NEXRAD_PRODUCT_SPEC_TYPE_MAX + 1) * sizeof(void *))

static nexrad_product_spec specs[] = {
    {  19,  1, "base reflectivity",               1000.0, 1.0 },
    {  20,  1, "base reflectivity",               2000.0, 1.0 },
    {  25,  2, "base velocity",                    250.0, 1.0 },
    {  26,  2, "base velocity",                    500.0, 1.0 },
    {  27,  2, "base velocity",                   1000.0, 1.0 },
    {  28,  3, "base spectrum width",              250.0, 1.0 },
    {  30,  3, "base spectrum width",             1000.0, 1.0 },
    {  94,  1, "base reflectivity",               1000.0, 1.0 },
    {  99,  2, "base velocity",                    250.0, 1.0 },
    { 134, 39, "high resolution VIL",             1000.0, 1.0 },
    { 135, 41, "enhanced echo tops",              1000.0, 1.0 },
    { 153,  1, "super resolution reflectivity",    460.0, 0.5 },
    { 154,  2, "super resolution velocity",        460.0, 0.5 },
    { 155,  3, "super resolution spectrum width",  460.0, 0.5 },
    {   0,  0, NULL,                                 0.0, 0.0 }
};

nexrad_product_spec *nexrad_product_spec_lookup(enum nexrad_product_type type) {
    static nexrad_product_spec **lookup = NULL;

    if (lookup == NULL) {
        int i, j;

        if ((lookup = malloc(NEXRAD_PRODUCT_SPEC_TABLE_SIZE)) == NULL) {
            goto error_malloc;
        }

        for (i=0, j=0; i<=NEXRAD_PRODUCT_SPEC_TYPE_MAX; i++) {
            if (i == specs[j].code) {
                lookup[i] = &specs[j++];
            } else {
                lookup[i] = NULL;
            }
        }
    }

    if (type > NEXRAD_PRODUCT_SPEC_TYPE_MAX) {
        goto error_invalid_type;
    }

    return lookup[type];

error_invalid_type:
error_malloc:
    return NULL;
}

int nexrad_product_type_supports_compression(enum nexrad_product_type type) {
    int i;

    for (i=0; compressed_products[i]; i++) {
        if (compressed_products[i] == type) {
            return 1;
        }
    }

    return 0;
}

enum nexrad_product_type nexrad_product_get_type(nexrad_product_description *product) {
    if (product == NULL) {
        return -1;
    }

    return be16toh(product->type);
}

int nexrad_product_read_dvil_attributes(nexrad_product_description *product, int *avset_angle, int *max_dvil, int *edited_radials, int *compression, size_t *size) {
    nexrad_dvil_attributes *dvil;

    if (nexrad_product_get_type(product) != NEXRAD_PRODUCT_DVL) {
        return -1;
    }

    dvil = &product->attributes.dvil;

    if (avset_angle)
        *avset_angle = NEXRAD_PRODUCT_AVSET_MAGNITUDE * (int16_t)be16toh(dvil->avset_angle);

    if (max_dvil)
        *max_dvil = be16toh(dvil->max_dvil);

    if (edited_radials)
        *edited_radials = be16toh(dvil->edited_radials);

    if (compression)
        *compression = be16toh(dvil->compression);

    if (size)
        *size = be32toh(dvil->size);

    return 0;
}
