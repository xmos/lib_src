// Copyright (c) 2017, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <xclib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>    /* roundf */
#include <stdlib.h>  /* abs */

#include "src.h"

#define NUM_OF_SAMPLES (100)
#define MIN_VALUE (1000000)
#define INTERVAL_VALUE (20)
#define DEBUG_PRINT (1)

int main()
{
    int32_t d = INT_MIN;

    for (uint32_t num_tests = 0; num_tests < 2*INTERVAL_VALUE; num_tests++) {
        // increment the value the test
        d = d + INT_MAX/INTERVAL_VALUE;

        // skip lower valies
        if (abs(d)<MIN_VALUE) continue;
        
        int32_t data[SRC_FF3V_FIR_NUM_PHASES][SRC_FF3V_FIR_TAPS_PER_PHASE];
        memset(data[0], 0, sizeof(data[0]));
        memset(data[1], 0, sizeof(data[1]));
        memset(data[2], 0, sizeof(data[2]));
        #if DEBUG_PRINT
            printf("Upsampling\nInput value: %d\nOutput samples: ", d);
        #endif
        for (unsigned s=0;s<NUM_OF_SAMPLES;s++)
        {
            // feed the input value to the upsampling functions
            int32_t sample = src_us3_voice_input_sample(data[0], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1], d);
            #if DEBUG_PRINT
                if (s==NUM_OF_SAMPLES-1) printf("%d ", sample);
            #endif

            for(unsigned r = 1; r < SRC_FF3V_FIR_NUM_PHASES; r++) {
                int32_t sample = src_us3_voice_get_next_sample(data[0], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1-r]);
                #if DEBUG_PRINT
                    if (s==NUM_OF_SAMPLES-1) printf("%d ", sample);
                #endif    
            }
            
            if (s==NUM_OF_SAMPLES-1) {
                // round up attenuation to the second decimal digit
                float atten =  roundf((float) (d) / sample * 100) / 100;
                #if DEBUG_PRINT
                    printf("\nAtten: %f\n", atten);
                #endif
                // return error if the attenuation is more than 10% off
                if (atten<0.9 || atten>1.1) {
                    printf("Error: wrong upsampling attenuation %f\n", atten);
                    return 1;
                }
            }    
        }
 
        #if DEBUG_PRINT
            printf("Downsampling\nInput value: %d\nOutput sample: ", d);
        #endif
        memset(data[0], 0, sizeof(data[0]));
        memset(data[1], 0, sizeof(data[1]));
        memset(data[2], 0, sizeof(data[2]));
        for (unsigned s=0;s<NUM_OF_SAMPLES;s++)
        {
            // feed the input value to the downsampling functions
            int64_t sum = 0;
            for(unsigned r = 0; r < SRC_FF3V_FIR_NUM_PHASES-1; r++) {
                //memset(data, 0, sizeof(data));
                sum = src_ds3_voice_add_sample(sum, data[r], src_ff3v_fir_coefs[r], d);   
            }
            sum = src_ds3_voice_add_final_sample(sum, data[SRC_FF3V_FIR_NUM_PHASES-1], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1], d);
            if (s==NUM_OF_SAMPLES-1) {
                // round up attenuation to the second digit
                float atten =  roundf((float) (d) / (int32_t) sum* 100) / 100;
                #if DEBUG_PRINT
                    printf("%d \nAtten: %f\n", (int32_t) sum, atten);
                #endif
                // return error if the attenuation is more than 10% off
                if (atten<0.9 || atten>1.1) {
                    printf("Error: wrong downsampling attenuation %f\n", atten);
                    return 1;
                }
            }
        }

    }
    printf("Success\n");
    return 0;
}
