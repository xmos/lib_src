// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
//
// This file is generated using src_ff3v_fir_generator.py
//
// Do not modify it directly, changes may be overwritten!
//

#include "src_ff3v_fir.h"
#include <stdint.h>

/** Used for FIR compensation for decimation*/
const unsigned src_ff3v_fir_comp_q_ds = 29;

/** Used for FIR compensation for decimation*/
const int32_t src_ff3v_fir_comp_ds = 1112549167;

/** Used for FIR compensation for upsampling*/
const unsigned src_ff3v_fir_comp_q_us = 27;

/** Used for FIR compensation for upsampling*/
const int32_t src_ff3v_fir_comp_us = 834411870;

/** Used for self testing src_ds3_voice and src_us3_voice functionality */
int32_t src_ff3v_fir_coefs_debug[SRC_FF3V_FIR_NUM_PHASES * SRC_FF3V_FIR_TAPS_PER_PHASE] = {
      1005142,      1367390,        29412,     -4146838,    -10154637,    -14619962,
    -13776186,     -6744830,      2692812,      8121360,      5508436,     -2814524,
     -9285443,     -7351357,      2193307,     10930750,      9953155,     -1338213,
    -13227624,    -13683020,      -123797,     16201217,     19008511,      2582573,
    -20205295,    -27041699,     -6837031,     26249516,     40760231,     15085431,
    -37763299,    -71330011,    -37235961,     74585089,    219245690,    320542055,
    320542055,    219245690,     74585089,    -37235961,    -71330011,    -37763299,
     15085431,     40760231,     26249516,     -6837031,    -27041699,    -20205295,
      2582573,     19008511,     16201217,      -123797,    -13683020,    -13227624,
     -1338213,      9953155,     10930750,      2193307,     -7351357,     -9285443,
     -2814524,      5508436,      8121360,      2692812,     -6744830,    -13776186,
    -14619962,    -10154637,     -4146838,        29412,      1367390,      1005142,

};

/** Coefficients for use with src_ds3_voice and src_us3_voice functions */
static const int32_t [[aligned(8)]] src_ff3v_fir_coefs_i[SRC_FF3V_FIR_NUM_PHASES][SRC_FF3V_FIR_TAPS_PER_PHASE] = {
    {
            29412,    -14619962,      2692812,     -2814524,      2193307,     -1338213,
          -123797,      2582573,     -6837031,     15085431,    -37235961,    320542055,
         74585089,    -37763299,     26249516,    -20205295,     16201217,    -13227624,
         10930750,     -9285443,      8121360,    -13776186,     -4146838,      1005142,
    },
    {
          1367390,    -10154637,     -6744830,      5508436,     -7351357,      9953155,
        -13683020,     19008511,    -27041699,     40760231,    -71330011,    219245690,
        219245690,    -71330011,     40760231,    -27041699,     19008511,    -13683020,
          9953155,     -7351357,      5508436,     -6744830,    -10154637,      1367390,
    },
    {
          1005142,     -4146838,    -13776186,      8121360,     -9285443,     10930750,
        -13227624,     16201217,    -20205295,     26249516,    -37763299,     74585089,
        320542055,    -37235961,     15085431,     -6837031,      2582573,      -123797,
         -1338213,      2193307,     -2814524,      2692812,    -14619962,        29412,
    },
};

unsafe {
    const int32_t (* unsafe src_ff3v_fir_coefs_c)[SRC_FF3V_FIR_TAPS_PER_PHASE] = src_ff3v_fir_coefs_i;
}

const int32_t (*src_ff3v_fir_coefs_xc)[SRC_FF3V_FIR_TAPS_PER_PHASE] = src_ff3v_fir_coefs_i;
