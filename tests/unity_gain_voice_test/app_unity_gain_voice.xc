// Copyright 2017-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include <xclib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  /* abs */
#include <math.h>    /* log10 */

#define debug_printf printf

#include "src.h"

#define MAX_DIFF_DB (-20)
#define NUM_OF_TAPS (SRC_FF3V_FIR_NUM_PHASES * SRC_FF3V_FIR_TAPS_PER_PHASE)
#define NUM_OF_SAMPLES (NUM_OF_TAPS + 1)
#define MIN_VALUE (1000000)
#define NUM_OF_TESTS (32)

void process_results(int32_t in_val, int32_t out_val, int is_upsampling) {
    // calculate the attenuation in dB
    float atten = fabs(in_val) / fabs(out_val);
    float atten_db = 20 * log10 (atten);
    debug_printf("\nDC Attenuation: %f dB\n", atten_db);
    atten_db = 0; // added this line to avoid warning about unused variable when DEBUG_PRINT is set to 0

    // calculate the difference in dB
    float diff =  fabs(in_val - out_val) / fabs(in_val);
    float diff_db = 20 * log10 (diff);
    debug_printf("Difference: %f dB\n", diff_db);
    char* sampling_str = "downsampling";
    if (is_upsampling) {
        sampling_str = "upsampling";
    }
    // evaluate the difference
    if (diff_db>MAX_DIFF_DB){
        printf("Error: the difference for %s is %f dB, max value is %d dB\n", sampling_str, diff_db, MAX_DIFF_DB);
        exit(1);
    }
}

int main()
{
    int32_t d = INT_MIN;

    for (uint32_t num_tests = 0; num_tests < NUM_OF_TESTS; num_tests++) {
        // increment the value the test
        d = d + INT_MAX/NUM_OF_TESTS;

        // skip lower valies
        if (abs(d)<MIN_VALUE) continue;
        
        int32_t data[SRC_FF3V_FIR_NUM_PHASES][SRC_FF3V_FIR_TAPS_PER_PHASE];
        memset(data, 0, sizeof(data));

        debug_printf("Upsampling:\nInput value: %d\nOutput samples: ", d);

        for (unsigned s=0;s<NUM_OF_SAMPLES;s++)
        {
            // feed the input value to the upsampling functions
            int32_t sample = src_us3_voice_input_sample(data[0], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1], d);
            
            if (s==NUM_OF_SAMPLES-1) debug_printf("%d ", sample);

            for(unsigned r = 1; r < SRC_FF3V_FIR_NUM_PHASES; r++) {
                int32_t sample = src_us3_voice_get_next_sample(data[0], src_ff3v_fir_coefs[SRC_FF3V_FIR_NUM_PHASES-1-r]);
                
                if (s==NUM_OF_SAMPLES-1) debug_printf("%d ", sample); 
            }
            
            if (s==NUM_OF_SAMPLES-1) {
                process_results(d, (int32_t) sample, 1);
            }    
        }
 
        debug_printf("Downsampling:\nInput value: %d\n", d);

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
                debug_printf("Output sample: %lld", sum);
                process_results(d, (int32_t) sum, 0);
            }
        }
    }
    printf("Success\n");
    return 0;
}
