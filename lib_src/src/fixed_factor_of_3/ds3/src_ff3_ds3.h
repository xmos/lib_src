// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _SRC_FF3_DS3_H_
#define _SRC_FF3_DS3_H_

#ifdef __XC__
extern "C" {
#endif

#define SRC_FF3_DS3_N_COEFS 144 // Number of coefficients must be a multiple of 6

/* Filters with "_b_" in their filenames have higher attenuation at
 * Nyquist (> 60dB compared with 20dB ) but with an earlier cutoff.
 */
#define   SRC_FF3_DS3_COEFS_FILE "FilterData/firds3_b_144.dat"
//#define SRC_FF3_DS3_COEFS_FILE "FilterData/firds3_144.dat"

#ifdef __XC__
}
#endif

#endif // _SRC_FF3_DS3_H_
