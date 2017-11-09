// Copyright (c) 2017, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <xclib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "src.h"

#define NUM_OF_TAPS (SRC_FF3V_FIR_NUM_PHASES * SRC_FF3V_FIR_TAPS_PER_PHASE)

#define INPUT_VAL INT_MAX//(5000000)
#define NUM_OF_SAMPLES 5000

int main()
{
    for (int32_t d = INT_MAX; d > 0; d = d - 100000000) {
        int32_t data[SRC_FF3V_FIR_NUM_PHASES][SRC_FF3V_FIR_TAPS_PER_PHASE];
        memset(data[0], 0, sizeof(data[0]));
        memset(data[1], 0, sizeof(data[1]));
        memset(data[2], 0, sizeof(data[2]));

        printf("Upsampling\nInput value: %d\nOutput samples: ", d);
        for (unsigned s=0;s<NUM_OF_SAMPLES;s++)
        {
        
            int32_t sample = src_us3_voice_input_sample(data[0], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1], d);
            if (s==NUM_OF_SAMPLES-1) printf("%d ", sample);
            for(unsigned r = 1; r < SRC_FF3V_FIR_NUM_PHASES; r++) {
                int32_t sample = src_us3_voice_get_next_sample(data[0], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1-r]);
                if (s==NUM_OF_SAMPLES-1) printf("%d ", sample);
            }
            
            if (s==NUM_OF_SAMPLES-1) {
                float atten =  roundf((float) (d)/sample * 100) / 100;
                printf("\nAtten: %f\n\n", atten);
                if (atten != 1) printf("\n\nError: wrong atten %f\n\n", atten);
            }    
        }


        printf("Downsampling\nInput value: %d\nOutput samples: ", d);
        memset(data[0], 0, sizeof(data[0]));
        memset(data[1], 0, sizeof(data[1]));
        memset(data[2], 0, sizeof(data[2]));
        for (unsigned s=0;s<NUM_OF_SAMPLES;s++)
        {
            int32_t sum = 0;
            for(unsigned r = 0; r < SRC_FF3V_FIR_NUM_PHASES-1; r++) {
                //memset(data, 0, sizeof(data));
                sum = src_ds3_voice_add_sample(sum, data[r], src_ff3v_fir_coefs[r], d);
                if (s==NUM_OF_SAMPLES-1) printf("%d ", sum);
            }
            sum = src_ds3_voice_add_final_sample(sum, data[2], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1], d);
            if (s==NUM_OF_SAMPLES-1) {
                float atten =  roundf((float) (d)/sum * 100) / 100;
                printf("%d \nAtten: %f\n\n", sum, atten);
                if (atten != 1) printf("\n\nError: wrong atten %f\n\n", atten);
            }
        }

    }
    return 0;
}
