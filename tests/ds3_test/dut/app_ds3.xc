// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// Downsample by factor of 3 example
// Uses Signed 32b Integer format

// Include files
#include <stdio.h>
#include <stdlib.h>
#include <xs1.h>
#include <stdint.h>
#include "src.h"

#define NUM_CHANNELS  2
#define NUM_OUTPUT_SAMPLES  256

#define NUM_IN_SAMPLES 800
// 0db 1KHz 48KHz sinewave - pure tone test
int32_t s1k_0db_48[NUM_IN_SAMPLES];
// -6db 5/6KHz 48KHz sinewaves - intermodulation test
int32_t im5k6k_m6dB_48[NUM_IN_SAMPLES];

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
    read_dat_file("../../../src_input/s1k_0dB_48.dat", s1k_0db_48, NUM_IN_SAMPLES);
    read_dat_file("../../../src_input/im5k6k_m6dB_48.dat", im5k6k_m6dB_48, NUM_IN_SAMPLES);


    unsafe {
        src_ff3_return_code_t return_code = SRC_FF3_NO_ERROR;

        // Input data for both channels
        const int32_t * unsafe input_data[NUM_CHANNELS] = {s1k_0db_48, im5k6k_m6dB_48};

        // Output samples
        int32_t output_data[NUM_CHANNELS];

        // DS3 instances variables
        // -----------------------
        // State and Control structures (one for each channel)
        int                 src_ds3_delay[NUM_CHANNELS][SRC_FF3_DS3_N_COEFS<<1];
        src_ds3_ctrl_t      src_ds3_ctrl[NUM_CHANNELS];

        //Init DS3
        for (int i=0; i<NUM_CHANNELS; i++) {

            printf("Init ds3 channel %d\n", i);
            // Process init
            // ------------
            // Set delay line base to ctrl structure
            src_ds3_ctrl[i].delay_base = src_ds3_delay[i];

            // Init instance
            if (src_ds3_init(&src_ds3_ctrl[i]) != SRC_FF3_NO_ERROR) {
                printf("Error on init\n");
                return_code = SRC_FF3_ERROR;
            }

            // Sync (i.e. clear data)
            // ----
            if (src_ds3_sync(&src_ds3_ctrl[i]) != SRC_FF3_NO_ERROR) {
                printf("Error on sync\n");
                return_code = SRC_FF3_ERROR;
            }

        }

        for (int s=0; s<NUM_OUTPUT_SAMPLES; s++) {
            for (int i=0; i<NUM_CHANNELS; i++) {
                // Set input and output data pointers for the DS3
                src_ds3_ctrl[i].in_data            = (int *)input_data[i];
                src_ds3_ctrl[i].out_data           = (int *)&output_data[i];

                // Do the sample rate conversion on three input samples
                if (src_ds3_proc(&src_ds3_ctrl[i]) != SRC_FF3_NO_ERROR) {
                    printf("Error on ds3 process\n");
                    return_code = SRC_FF3_ERROR;
                }
                //for(int j=0; j<3; j++) printf("in = %d\n", *(input_data[i] + j));
                input_data[i] += 3; // Move input pointer on by 3
                printf("%d\n", output_data[i]);
            }
        }
        return (int)return_code;
    }
}
