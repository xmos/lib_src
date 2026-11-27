// Copyright 2023-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "src_poly.h"
#include "src_rat_fir_coefs.h"

#define BLOCK_FRAMES 160 // 10 ms at 16 kHz

const char bin_in_48k[70] = "sig_48k.bin\0";
const char bin_out_32k[70] = "sig_c_32k.bin\0";
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
    int32_t ALIGNMENT(8) state_ds[SRC_RAT_FIR_TAPS_PER_PHASE_DS] = {0};
    int32_t ALIGNMENT(8) state_us[SRC_RAT_FIR_TAPS_PER_PHASE_US] = {0};
    FILE * in_48k_fp = open_(bin_in_48k, "rb");
    FILE * out_32k_fp = open_(bin_out_32k, "wb");
    FILE * out_48k_fp = open_(bin_out_48k, "wb");

    fseek(in_48k_fp, 0, SEEK_END);
    int in_len = ftell(in_48k_fp) / sizeof(int32_t);
    fseek(in_48k_fp, 0, SEEK_SET);
    int loop_len = in_len / 3;

    // xsim host file I/O gets slower per call as the files grow, so move data
    // in blocks rather than one frame at a time.
    static int32_t in_48k[BLOCK_FRAMES * 3];
    static int32_t out_32k[BLOCK_FRAMES * 2];
    static int32_t out_48k[BLOCK_FRAMES * 3];

    for(int i = 0; i < loop_len; i += BLOCK_FRAMES)
    {
        int n = (loop_len - i < BLOCK_FRAMES) ? (loop_len - i) : BLOCK_FRAMES;
        fread(in_48k, sizeof(int32_t), n * 3, in_48k_fp);

        for(int j = 0; j < n; j++)
        {
            src_rat_2_3_96t_ds(&in_48k[j * 3], &out_32k[j * 2], src_rat_fir_ds_coefs, state_ds);
            src_rat_3_2_96t_us(&out_32k[j * 2], &out_48k[j * 3], src_rat_fir_us_coefs, state_us);
        }

        fwrite(out_32k, sizeof(int32_t), n * 2, out_32k_fp);
        fwrite(out_48k, sizeof(int32_t), n * 3, out_48k_fp);
    }

    fclose(in_48k_fp);
    fclose(out_32k_fp);
    fclose(out_48k_fp);

    return 0;
}
