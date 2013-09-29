#include <nexrad/product.h>

static enum nexrad_product_type compressed_products[] = {
    NEXRAD_PRODUCT_DVL,
    NEXRAD_PRODUCT_EET,
    NEXRAD_PRODUCT_N0X,
    0
};

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
