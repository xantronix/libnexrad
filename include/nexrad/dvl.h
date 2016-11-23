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

#ifndef _NEXRAD_DVL_H
#define _NEXRAD_DVL_H

#include <stdint.h>

#define NEXRAD_DVL_KG_MSQ     1000.0
#define NEXRAD_DVL_SCALE        90.6875
#define NEXRAD_DVL_OFFSET        2.0
#define NEXRAD_DVL_LOG_START    20
#define NEXRAD_DVL_LOG_SCALE    38.875
#define NEXRAD_DVL_LOG_OFFSET   83.875

/*!
 * \file nexrad/dvl.h
 * \brief Functions for accessing Digital (High Resolution) VIL data
 *
 * An interface providing routines for determining the validity of a DVL
 * reading, as well as for converting DVL samples into kg/m^-2 quantities.
 */

/*!
 * \defgroup dvl NEXRAD Level III Digital Vertically Integrated Liquid data access routines
 */

/*!
 * \ingroup dvl
 * \brief Determine if DVL sample is valid
 * \param v 8-bit DVL sample from product file
 * \return 1 if valid, 0 if invalid
 *
 * Determine the validity of a sample of digital VIL data.
 */
int nexrad_dvl_valid(uint8_t v);

/*!
 * \ingroup dvl
 * \brief Determine DVL value in units of kg/m^-2
 * \param v 8-bit DVL sample fro mproduct file
 * \return Quantity of liquid in units of kg/m^-2
 *
 * Determine the average quantity of liquid in units of kg/m^-2.
 */
double nexrad_dvl_vil(uint8_t v);

#endif /* _NEXRAD_DVL_H */
