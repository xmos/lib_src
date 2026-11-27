// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

// Round trip through the deprecated fixed factor of 3 voice SRC:
// sig_48k.bin -> src_ds3_voice -> sig_c_16k.bin -> src_us3_voice -> sig_c_48k.bin

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "src.h"

#define NUM_PHASES SRC_FF3V_FIR_NUM_PHASES
#define TAPS_PER_PHASE SRC_FF3V_FIR_TAPS_PER_PHASE

#define BLOCK_FRAMES 160 // 10 ms at 16 kHz

static FILE * open_(const char * filename, const char * mode)
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
    // The assembly uses double word loads and stores on the delay lines
    static int32_t state_ds[NUM_PHASES][TAPS_PER_PHASE] __attribute__((aligned(8)));
    static int32_t state_us[TAPS_PER_PHASE] __attribute__((aligned(8)));
    memset(state_ds, 0, sizeof(state_ds));
    memset(state_us, 0, sizeof(state_us));

    FILE * in_48k_fp = open_("sig_48k.bin", "rb");
    FILE * out_16k_fp = open_("sig_c_16k.bin", "wb");
    FILE * out_48k_fp = open_("sig_c_48k.bin", "wb");

    fseek(in_48k_fp, 0, SEEK_END);
    int in_len = ftell(in_48k_fp) / sizeof(int32_t);
    fseek(in_48k_fp, 0, SEEK_SET);
    int out_len = in_len / NUM_PHASES;

    // xsim host file I/O gets slower per call as the files grow, so move data
    // in blocks rather than one frame at a time.
    static int32_t in_48k[BLOCK_FRAMES * NUM_PHASES];
    static int32_t out_16k[BLOCK_FRAMES];
    static int32_t out_48k[BLOCK_FRAMES * NUM_PHASES];

    for(int i = 0; i < out_len; i += BLOCK_FRAMES)
    {
        int n = (out_len - i < BLOCK_FRAMES) ? (out_len - i) : BLOCK_FRAMES;
        fread(in_48k, sizeof(int32_t), n * NUM_PHASES, in_48k_fp);

        for(int j = 0; j < n; j++)
        {
            int64_t sum = 0;
            for(int r = 0; r < NUM_PHASES - 1; r++)
            {
                sum = src_ds3_voice_add_sample(sum, state_ds[r], src_ff3v_fir_coefs[r],
                                               in_48k[j * NUM_PHASES + r]);
            }
            sum = src_ds3_voice_add_final_sample(sum, state_ds[NUM_PHASES - 1],
                                                 src_ff3v_fir_coefs[NUM_PHASES - 1],
                                                 in_48k[j * NUM_PHASES + NUM_PHASES - 1]);
            out_16k[j] = (int32_t)sum;

            out_48k[j * NUM_PHASES] = src_us3_voice_input_sample(state_us,
                                          src_ff3v_fir_coefs[NUM_PHASES - 1], out_16k[j]);
            for(int r = 1; r < NUM_PHASES; r++)
            {
                out_48k[j * NUM_PHASES + r] = src_us3_voice_get_next_sample(state_us,
                                                  src_ff3v_fir_coefs[NUM_PHASES - 1 - r]);
            }
        }

        fwrite(out_16k, sizeof(int32_t), n, out_16k_fp);
        fwrite(out_48k, sizeof(int32_t), n * NUM_PHASES, out_48k_fp);
    }

    fclose(in_48k_fp);
    fclose(out_16k_fp);
    fclose(out_48k_fp);

    return 0;
}
