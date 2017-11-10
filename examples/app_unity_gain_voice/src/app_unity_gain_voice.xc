// Copyright (c) 2017, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <xclib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  /* abs */
#include <math.h>    /* log10 */

#include "src.h"

#define NUM_OF_SAMPLES (100)
#define MIN_VALUE (1000000)
#define INTERVAL_VALUE (20)

int main()
{
    int32_t d = INT_MIN;

    for (uint32_t num_tests = 0; num_tests < 2*INTERVAL_VALUE; num_tests++) {
        // increment the value the test
        d = d + INT_MAX/INTERVAL_VALUE;

        // skip lower valies
        if (abs(d)<MIN_VALUE) continue;
        
        int32_t data[SRC_FF3V_FIR_NUM_PHASES][SRC_FF3V_FIR_TAPS_PER_PHASE];
        memset(data, 0, sizeof(data));

        printf("Upsampling\nInput value: %d\nOutput samples: ", d);

        for (unsigned s=0;s<NUM_OF_SAMPLES;s++)
        {
            // feed the input value to the upsampling functions
            int32_t sample = src_us3_voice_input_sample(data[0], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1], d);
            
            if (s==NUM_OF_SAMPLES-1) printf("%d ", sample);

            for(unsigned r = 1; r < SRC_FF3V_FIR_NUM_PHASES; r++) {
                int32_t sample = src_us3_voice_get_next_sample(data[0], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1-r]);
                
                if (s==NUM_OF_SAMPLES-1) printf("%d ", sample); 
            }
            
            if (s==NUM_OF_SAMPLES-1) {
                // calculate the attenuation in dB
                float atten =  (float) (d) / sample;
                float atten_db = 20 * log10 (atten);
                printf("\nAtten: %f dB\n", atten_db);
            }    
        }
 
        printf("Downsampling\nInput value: %d\nOutput sample: ", d);

        memset(data, 0, sizeof(data));

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
                // calculate the attenuation in dB
                float atten =  (float) (d) / (int32_t) sum;
                float atten_db = 20 * log10 (atten);
                printf("%d \nAtten: %f dB\n", (int32_t) sum, atten_db);
            }
        }

    }
    printf("Success\n");
    return 0;
}
