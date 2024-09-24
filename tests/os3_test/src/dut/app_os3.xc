// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// Upsample by factor of 3 example
// Uses Signed 32b Integer format

// Include files
#include <stdio.h>
#include <xs1.h>
#include <stdint.h>
#include <stdlib.h>
#include "src.h"

#define NUM_CHANNELS  2
#define NUM_OUTPUT_SAMPLES  256

#define NUM_IN_SAMPLES 100
// 0db 1KHz 48KHz sinewave - pure tone test
int32_t s1k_0db_16[NUM_IN_SAMPLES];
// -6db 5/6KHz 48KHz sinewaves - intermodulation test
int32_t im5k6k_m6dB_16[NUM_IN_SAMPLES];

void read_dat_file(char *fname, int32_t data[], size_t n)
{
    FILE * movable InFileDat;
    if ((InFileDat = fopen((char *)fname, "rt")) == NULL)
    {
        printf("Error while opening input file, %s", fname);
        exit(1);
    }
    for(int i = 0; i < n; i++)
    {
        if(fscanf(InFileDat, "%i\n", &data[i]) == EOF)
        {
            printf("Error while reading input file, %s at line %d", fname, i);
            exit(1);
        }
        // printf("read: %d\n", data[i]);
    }
    fclose(move(InFileDat));
}

int main(void)
{
    read_dat_file("../../../src_input/s1k_0dB_16.dat", s1k_0db_16, NUM_IN_SAMPLES);
    read_dat_file("../../../src_input/im5k6k_m6dB_16.dat", im5k6k_m6dB_16, NUM_IN_SAMPLES);

    unsafe {
        src_ff3_return_code_t return_code = SRC_FF3_NO_ERROR;

        // Input data for both channels
        const int32_t * unsafe input_data[NUM_CHANNELS] = {s1k_0db_16, im5k6k_m6dB_16};

        // OS3 instances variables
        // -----------------------
        // State and Control structures (one per channel)
        int32_t           src_os3_delay[NUM_CHANNELS][(SRC_FF3_OS3_N_COEFS/SRC_FF3_OS3_N_PHASES)<<1];        // Delay line length is 1/3rd of number of coefs as over-sampler by 3 (and double for circular buffer simulation)
        src_os3_ctrl_t    src_os3_ctrl[NUM_CHANNELS];

        //Init OS3
        for (int i=0; i<NUM_CHANNELS; i++) {

            printf("Init os3 channel %d\n", i);
            // Process init
            // ------------
            // Set delay line base to ctrl structure
            src_os3_ctrl[i].delay_base        = (int*)src_os3_delay[i];

            // Init instance
            if (src_os3_init(&src_os3_ctrl[i]) != SRC_FF3_NO_ERROR) {
                printf("Error on init\n");
                return_code = SRC_FF3_ERROR;
            }

            // Sync (i.e. clear data)
            // ----
            if (src_os3_sync(&src_os3_ctrl[i]) != SRC_FF3_NO_ERROR) {
                printf("Error on sync\n");
                return_code = SRC_FF3_ERROR;
            }

        }

        for (int s=0; s<NUM_OUTPUT_SAMPLES; s++) {
            for (int i=0; i<NUM_CHANNELS; i++) {
                // Run through output samples, we read 1 input sample every 3 output samples
                // This is given when the phase member of the control structure reaches '0'

                // Check if a new input sample is needed
                // If it is needed read it into the control structure and call function to write to delay line
                // If we have reached end of file, signal it.
                if (src_os3_ctrl[i].phase == 0) {
                    src_os3_ctrl[i].in_data = *input_data[i];
                    //printf("in = %d\n", (int32_t) *input_data[i]);
                    input_data[i]++;

                    if (src_os3_input(&src_os3_ctrl[i]) != SRC_FF3_NO_ERROR) {
                        printf("Error on os3 input\n");
                        return_code = SRC_FF3_ERROR;
                    }
                }

                // Call sample rate conversion. Always output a sample on each loop
                if (src_os3_proc(&src_os3_ctrl[i]) != SRC_FF3_NO_ERROR) {
                    printf("Error on os3 process\n");
                    return_code = -4;
                }
                printf("%d\n", src_os3_ctrl[i].out_data);
            }
        }
        return (int)return_code;
    }
}
