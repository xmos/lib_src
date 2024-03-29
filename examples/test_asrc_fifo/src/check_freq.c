// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xclib.h>
#include "check_freq.h"
#include "xmath/xmath.h"

int raised_cosine8[256] = {
    0,
    2526,
    10104,
    22729,
    40393,
    63086,
    90794,
    123500,
    161184,
    203825,
    251396,
    303868,
    361210,
    423388,
    490363,
    562097,
    638544,
    719660,
    805396,
    895699,
    990516,
    1089789,
    1193458,
    1301462,
    1413735,
    1530209,
    1650814,
    1775478,
    1904126,
    2036679,
    2173059,
    2313183,
    2456966,
    2604322,
    2755163,
    2909397,
    3066931,
    3227671,
    3391520,
    3558378,
    3728147,
    3900722,
    4076001,
    4253878,
    4434245,
    4616994,
    4802015,
    4989197,
    5178426,
    5369589,
    5562571,
    5757254,
    5953523,
    6151259,
    6350342,
    6550653,
    6752071,
    6954475,
    7157743,
    7361752,
    7566380,
    7771503,
    7976998,
    8182741,
    8388608,
    8594474,
    8800217,
    9005712,
    9210835,
    9415463,
    9619472,
    9822740,
    10025144,
    10226562,
    10426873,
    10625956,
    10823692,
    11019961,
    11214644,
    11407626,
    11598789,
    11788018,
    11975200,
    12160221,
    12342970,
    12523337,
    12701214,
    12876493,
    13049068,
    13218837,
    13385695,
    13549544,
    13710284,
    13867818,
    14022052,
    14172893,
    14320249,
    14464032,
    14604156,
    14740536,
    14873089,
    15001737,
    15126401,
    15247006,
    15363480,
    15475753,
    15583757,
    15687426,
    15786699,
    15881516,
    15971819,
    16057555,
    16138671,
    16215118,
    16286852,
    16353827,
    16416005,
    16473347,
    16525819,
    16573390,
    16616031,
    16653715,
    16686421,
    16714129,
    16736822,
    16754486,
    16767111,
    16774689,
    16777216,
    16774689,
    16767111,
    16754486,
    16736822,
    16714129,
    16686421,
    16653715,
    16616031,
    16573390,
    16525819,
    16473347,
    16416005,
    16353827,
    16286852,
    16215118,
    16138671,
    16057555,
    15971819,
    15881516,
    15786699,
    15687426,
    15583757,
    15475753,
    15363480,
    15247006,
    15126401,
    15001737,
    14873089,
    14740536,
    14604156,
    14464032,
    14320249,
    14172893,
    14022052,
    13867818,
    13710284,
    13549544,
    13385695,
    13218837,
    13049068,
    12876493,
    12701214,
    12523337,
    12342970,
    12160221,
    11975200,
    11788018,
    11598789,
    11407626,
    11214644,
    11019961,
    10823692,
    10625956,
    10426873,
    10226562,
    10025144,
    9822740,
    9619472,
    9415463,
    9210835,
    9005712,
    8800217,
    8594474,
    8388607,
    8182741,
    7976998,
    7771503,
    7566380,
    7361752,
    7157743,
    6954475,
    6752071,
    6550653,
    6350342,
    6151259,
    5953523,
    5757254,
    5562571,
    5369589,
    5178426,
    4989197,
    4802015,
    4616994,
    4434245,
    4253878,
    4076001,
    3900722,
    3728147,
    3558378,
    3391520,
    3227671,
    3066931,
    2909397,
    2755163,
    2604322,
    2456966,
    2313183,
    2173059,
    2036679,
    1904126,
    1775478,
    1650814,
    1530209,
    1413735,
    1301462,
    1193458,
    1089789,
    990516,
    895699,
    805396,
    719660,
    638544,
    562097,
    490363,
    423388,
    361210,
    303868,
    251396,
    203825,
    161184,
    123500,
    90794,
    63086,
    40393,
    22729,
    10104,
    2526,
};

int raised_cosine7[128] = {
    0,
    10104,
    40393,
    90794,
    161184,
    251396,
    361210,
    490363,
    638544,
    805396,
    990516,
    1193458,
    1413735,
    1650814,
    1904126,
    2173059,
    2456966,
    2755163,
    3066931,
    3391520,
    3728147,
    4076001,
    4434245,
    4802015,
    5178426,
    5562571,
    5953523,
    6350342,
    6752071,
    7157743,
    7566380,
    7976998,
    8388608,
    8800217,
    9210835,
    9619472,
    10025144,
    10426873,
    10823692,
    11214644,
    11598789,
    11975200,
    12342970,
    12701214,
    13049068,
    13385695,
    13710284,
    14022052,
    14320249,
    14604156,
    14873089,
    15126401,
    15363480,
    15583757,
    15786699,
    15971819,
    16138671,
    16286852,
    16416005,
    16525819,
    16616031,
    16686421,
    16736822,
    16767111,
    16777216,
    16767111,
    16736822,
    16686421,
    16616031,
    16525819,
    16416005,
    16286852,
    16138671,
    15971819,
    15786699,
    15583757,
    15363480,
    15126401,
    14873089,
    14604156,
    14320249,
    14022052,
    13710284,
    13385695,
    13049068,
    12701214,
    12342970,
    11975200,
    11598789,
    11214644,
    10823692,
    10426873,
    10025144,
    9619472,
    9210835,
    8800217,
    8388607,
    7976998,
    7566380,
    7157743,
    6752071,
    6350342,
    5953523,
    5562571,
    5178426,
    4802015,
    4434245,
    4076001,
    3728147,
    3391520,
    3066931,
    2755163,
    2456966,
    2173059,
    1904126,
    1650814,
    1413735,
    1193458,
    990516,
    805396,
    638544,
    490363,
    361210,
    251396,
    161184,
    90794,
    40393,
    10104,
};

int raised_cosine6[64] = {
    0,
    40393,
    161184,
    361210,
    638544,
    990516,
    1413735,
    1904126,
    2456966,
    3066931,
    3728147,
    4434245,
    5178426,
    5953523,
    6752071,
    7566380,
    8388608,
    9210835,
    10025144,
    10823692,
    11598789,
    12342970,
    13049068,
    13710284,
    14320249,
    14873089,
    15363480,
    15786699,
    16138671,
    16416005,
    16616031,
    16736822,
    16777216,
    16736822,
    16616031,
    16416005,
    16138671,
    15786699,
    15363480,
    14873089,
    14320249,
    13710284,
    13049068,
    12342970,
    11598789,
    10823692,
    10025144,
    9210835,
    8388607,
    7566380,
    6752071,
    5953523,
    5178426,
    4434245,
    3728147,
    3066931,
    2456966,
    1904126,
    1413735,
    990516,
    638544,
    361210,
    161184,
    40393,    
};

int raised_cosine5[32] = {
    0,
    161184,
    638544,
    1413735,
    2456966,
    3728147,
    5178426,
    6752071,
    8388608,
    10025144,
    11598789,
    13049068,
    14320249,
    15363480,
    16138671,
    16616031,
    16777216,
    16616031,
    16138671,
    15363480,
    14320249,
    13049068,
    11598789,
    10025144,
    8388607,
    6752071,
    5178426,
    3728147,
    2456966,
    1413735,
    638544,
    161184,
};


int check_freq(int data[], int x, unsigned n, int limit[], int perform_check) {
    if (perform_check) {
        // First half of check: calculate per bin energy - fits in one 192 kHz cycle
        if (n == 0) {
            unsigned int total = 0;
            for(int i = 0; i <= (1<< (LOG_FFT_BINS)); i+=2) {
                int d0 = data[i];
                int d1 = data[i+1];
                unsigned energy = (d0*(int64_t) d0 + d1*(int64_t) d1) >> 32;
                data[i] = clz(energy);
                total += energy;
            }
            total = clz(total);
            data[3] = total;
            // Also normalise bin 0 - it will be overwritten
            int k = total - data[0] + 32;
            if (k > limit[0]) {
                limit[0] = k;
            }
        } else if (n == 1) { // Second half of energy check - normalise bins against total
            unsigned int total = data[3];
            for(int i = 1; i <= (1<< (LOG_FFT_BINS-1)); i++) {
                int k = total - data[2*i] + 32;
                if (k > limit[i]) {
                    limit[i] = k;
                }
            }
        }
    }
    unsigned index = bitrev(n << (32-LOG_FFT_BINS));
    data[2*index] = (x * (int64_t) raised_cosine6[n]) >> 24;
    data[2*index+1] = 0;
    n++;
    if (n == (1 << LOG_FFT_BINS)) { // Zeroth part of energy check: compute FFT
        headroom_t hr = 4;
        exponent_t exp = 0;
        fft_dit_forward ((complex_s32_t *) data, n, &hr, &exp);
        n = 0;
    }
    return n;
}
