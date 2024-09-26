// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "src_poly.h"
#if (NUM_TAPS == 96)
#include "test_src_96t_coefs.h"
#define downsample src_ff3_96t_ds
#define upsample src_ff3_96t_us
#define do_fir fir_s32_32t
#define do_conv conv_s32_32t
#define coefs test_src_96t_coefs
#define NUM_PHASES TEST_SRC_96T_NUM_PHASES
#define NUM_TAPS_PER_PHASE TEST_SRC_96T_TAPS_PER_PHASE
#elif (NUM_TAPS == 72)
#include "test_src_72t_coefs.h"
#define downsample src_ff3_72t_ds
#define upsample src_ff3_72t_us
#define do_fir fir_s32_24t
#define do_conv conv_s32_24t
#define coefs test_src_72t_coefs
#define NUM_PHASES TEST_SRC_72T_NUM_PHASES
#define NUM_TAPS_PER_PHASE TEST_SRC_72T_TAPS_PER_PHASE
#else
#error Number of taps not supported
#endif


const char bin_in_48k[70] = "sig_48k.bin\0";
const char bin_out_16k[70] = "sig_c_16k.bin\0";
const char bin_out_48k[70] = "sig_c_48k.bin\0";

FILE * open_(const char * filename, const char * mode)
{
    FILE * fp = fopen(filename, mode);
    if(fp == NULL)
    {
        printf("Error while opening file\n");
        printf("%s\n", filename);
        exit(1);
    }
    return fp;
}

int main()
{
    int32_t ALIGNMENT(8) state_ds[NUM_PHASES][NUM_TAPS_PER_PHASE] = {{0}};
    int32_t ALIGNMENT(8) state_us[NUM_TAPS_PER_PHASE] = {0};
    FILE * in_48k_fp = open_(bin_in_48k, "rb");
    FILE * out_16k_fp = open_(bin_out_16k, "wb");
    FILE * out_48k_fp = open_(bin_out_48k, "wb");
    
    fseek(in_48k_fp, 0, SEEK_END);
    int in_len = ftell(in_48k_fp) / sizeof(int32_t);
    fseek(in_48k_fp, 0, SEEK_SET);
    int out_len = in_len / NUM_PHASES;

    for(int i = 0; i < out_len ; i ++)
    {
        int32_t in_48k[3] = {0};
        int32_t out_16k[1] = {0};
        int32_t out_48k[3] = {0};
        int64_t acc = 0;

        fread(in_48k, sizeof(int32_t), 3, in_48k_fp);

        // Not testing the high-level API for now
        //downsample(in_48k, out_16k, coefs, state_ds);
        acc += do_fir(state_ds[0], &coefs[0][0], in_48k[0]);
        acc += do_fir(state_ds[1], &coefs[1][0], in_48k[1]);
        acc += do_fir(state_ds[2], &coefs[2][0], in_48k[2]);
        out_16k[0] = (int32_t)acc;


        fwrite(out_16k, sizeof(int32_t), 1, out_16k_fp);

        // Not testing the high-level API for now
        //upsample(out_16k, out_48k, coefs, state_us);
        out_48k[0] = do_fir(state_us, &coefs[2][0], out_16k[0]) * 3;
        out_48k[1] = do_conv(state_us, &coefs[1][0]) * 3;
        out_48k[2] = do_conv(state_us, &coefs[0][0]) * 3;

        fwrite(out_48k, sizeof(int32_t), 3, out_48k_fp);
    }
    
    return 0;
}
