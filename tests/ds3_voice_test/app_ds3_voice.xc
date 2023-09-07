// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include <xclib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "src.h"

#define NUM_OF_TAPS (SRC_FF3V_FIR_NUM_PHASES * SRC_FF3V_FIR_TAPS_PER_PHASE)

static int pseudo_random(unsigned &x)
{
    crc32(x, -1, 0xEB31D82E);
    return (int)x;
}

int main()
{
    unsigned x = 1;

    int32_t data_debug[NUM_OF_TAPS];
    memset(data_debug, 0, sizeof(data_debug));

    int32_t data[SRC_FF3V_FIR_NUM_PHASES][SRC_FF3V_FIR_TAPS_PER_PHASE];
    memset(data, 0, sizeof(data));

    for (unsigned r = 0; r < (NUM_OF_TAPS*8); r++)
    {
        int64_t sum = 0;

        int32_t d = pseudo_random(x);
        sum = src_ds3_voice_add_sample(sum, data[0], src_ff3v_fir_coefs[0], d);
        for (unsigned i = (NUM_OF_TAPS-1); i > 0; i--)
        {
            data_debug[i] = data_debug[i-1];
        }
        data_debug[0] = d;

        d = pseudo_random(x);
        sum = src_ds3_voice_add_sample(sum, data[1], src_ff3v_fir_coefs[1], d);
        for (unsigned i = (NUM_OF_TAPS-1); i > 0; i--)
        {
            data_debug[i] = data_debug[i-1];
        }
        data_debug[0] = d;

        d = pseudo_random(x);
        sum = src_ds3_voice_add_final_sample(sum, data[2], src_ff3v_fir_coefs[2], d);
        for (unsigned i = (NUM_OF_TAPS-1); i > 0; i--)
        {
            data_debug[i] = data_debug[i-1];
        }
        data_debug[0] = d;

        int64_t sum_debug = 0;
        for (unsigned i = 0; i < NUM_OF_TAPS; i++)
        {
            sum_debug += (int64_t)src_ff3v_fir_coefs_debug[i]*(int64_t)data_debug[i];
        }
        sum_debug >>= 31;

        sum_debug = (int32_t)(((int64_t)sum_debug * (int64_t)src_ff3v_fir_comp_ds)>>src_ff3v_fir_comp_q_ds);

        if ((int32_t)sum != (int32_t)sum_debug)
        {
            printf("Error\n");
            return 1;
        }

    }
    printf("Success\n");
    return 0;
}
