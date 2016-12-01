// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef _SRC_OS3_H_
#define _SRC_OS3_H_

#ifdef __XC__
extern "C" {
#endif

#define SRC_OS3_N_COEFS 144 // Number of coefficients must be a multiple of 6
#define SRC_OS3_N_PHASES 3  // Number of output phases (3 as OS3 over-sample by 3)

/* Filters with "_b_" in their filenames in FIROS3.h have higher attenuation at
 * 8kHz (> 60dB compared with 20dB ) but with an earlier cutoff.
 */
#define   SRC_OS3_COEFS_FILE  "FilterData/firos3_b_144.dat"
//#define SRC_OS3_COEFS_FILE  "FilterData/firos3_144.dat"

#ifdef __XC__
}
#endif

#endif // _SRC_OS3_H_
