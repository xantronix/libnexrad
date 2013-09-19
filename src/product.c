#include <nexrad/product.h>

int nexrad_product_read_dvil_attributes(nexrad_product_description *product, int *avset_angle, int *max_dvil, int *edited_radials, int *compression, size_t *size) {
    nexrad_dvil_attributes *dvil;

    if (product == NULL || be16toh(product->product_id) != NEXRAD_PRODUCT_DVL) {
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
