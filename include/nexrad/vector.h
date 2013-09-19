#ifndef _NEXRAD_VECTOR_H
#define _NEXRAD_VECTOR_H

typedef struct _nexrad_vector {
    int magnitude;
    int i1_start;
    int j1_start;
    int i1_end;
    int j1_end;
    int i2_start;
    int j2_start;
    int i2_end;
    int j2_end;
} nexrad_vector;

#endif /* _NEXRAD_VECTOR_H */
