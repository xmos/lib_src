// Copyright (c) 2016, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <xclib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "src.h"

static int pseudo_random(unsigned &x)
{
    crc32(x, -1, 0xEB31D82E);
    return (int)x;
}

int main()
{
    unsigned x = 1;

    int32_t data_debug[72];
    memset(data_debug, 0, sizeof(data_debug));

    int32_t data[3][24];
    memset(data, 0, sizeof(data));

    for (unsigned r=0;r<72*8;r++)
    {
        int64_t sum = 0;

        int32_t d = pseudo_random(x);
        sum = src_ds3_voice_add_sample(sum, data[0], src_ds3_voice_coefs[0], d);
        for (unsigned i=72-1;i>0;i--)
        {
            data_debug[i] = data_debug[i-1];
        }
        data_debug[0] = d;

        d = pseudo_random(x);
        sum = src_ds3_voice_add_sample(sum, data[1], src_ds3_voice_coefs[1], d);
        for (unsigned i=72-1;i>0;i--)
        {
            data_debug[i] = data_debug[i-1];
        }
        data_debug[0] = d;

        d = pseudo_random(x);
        sum = src_ds3_voice_add_final_sample(sum, data[2], src_ds3_voice_coefs[2], d);
        for (unsigned i=72-1;i>0;i--)
        {
            data_debug[i] = data_debug[i-1];
        }
        data_debug[0] = d;

        int64_t sum_debug = 0;
        for (unsigned i=0;i<72;i++)
        {
            sum_debug += (int64_t)src_ds3_voice_coefs_debug[i]*(int64_t)data_debug[i];
        }
        sum_debug >>= 31;

        sum_debug = (int32_t)(((int64_t)sum_debug * (int64_t)src_ds3_voice_fir_comp )>>src_ds3_voice_fir_comp_q);

        if ((int32_t)sum != (int32_t)sum_debug)
        {
            printf("Error\n");
            return 1;
        }

    }
    printf("Success\n");
    return 0;
}
