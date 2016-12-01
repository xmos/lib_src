// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef _SRC_DS3_H_
#define _SRC_DS3_H_

#ifdef __XC__
extern "C" {
#endif

#define SRC_DS3_N_COEFS 144 // Number of coefficients must be a multiple of 6

/* Filters with "_b_" in their filenames in FIRDS3.h have higher attenuation at
 * Nyquist (> 60dB compared with 20dB ) but with an earlier cutoff.
 */
#define   SRC_DS3_COEFS_FILE "FilterData/firds3_b_144.dat"
//#define SRC_DS3_COEFS_FILE "FilterData/firds3_144.dat"

#ifdef __XC__
}
#endif

#endif // _SRC_DS3_H_
