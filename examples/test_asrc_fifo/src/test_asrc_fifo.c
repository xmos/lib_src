#include <xcore/parallel.h>
#include <xcore/hwtimer.h>
#include <stdio.h>
#include <print.h>
#include <platform.h>
#include <xscope.h>
#include "asynchronous_fifo.h"
#include "asrc_timestamp_interpolation.h"

#include <string.h>

#include <xcore/chanend.h>
#include <platform.h>
#include <xs1.h>
#include <print.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xscope.h>
#include "src.h"


#define INPUT_FREQUENCY     192000

#if INPUT_FREQUENCY == 44100
    #define BASE_FREQUENCY 44100
    #define INPUT_CODE     FS_CODE_44
#elif INPUT_FREQUENCY == 48000
    #define BASE_FREQUENCY 48000
    #define INPUT_CODE     FS_CODE_48
#elif INPUT_FREQUENCY == 96000
    #define BASE_FREQUENCY 48000
    #define INPUT_CODE     FS_CODE_96
#elif INPUT_FREQUENCY == 192000
    #define BASE_FREQUENCY 48000
    #define INPUT_CODE     FS_CODE_192
#else
#error "Unknown input frequency"
#endif


#define     SRC_N_CHANNELS                (1)   // Total number of audio channels to be processed by SRC (minimum 1)
#define     SRC_N_INSTANCES               (1)   // Number of instances (each usually run a logical core) used to process audio (minimum 1)
#define     SRC_CHANNELS_PER_INSTANCE     (SRC_N_CHANNELS/SRC_N_INSTANCES) // Calculated number of audio channels processed by each core
#define     SRC_N_IN_SAMPLES              (4)   // Number of samples per channel in each block passed into SRC each call
                                                // Must be a power of 2 and minimum value is 4 (due to two /2 decimation stages)
#define     SRC_N_OUT_IN_RATIO_MAX        (5)   // Max ratio between samples out:in per processing step (44.1->192 is worst case)
#define     SRC_DITHER_SETTING            (0)   // Enables or disables quantisation of output with dithering to 24b
#define     SRC_MAX_NUM_SAMPS_OUT         (SRC_N_OUT_IN_RATIO_MAX * SRC_N_IN_SAMPLES)
#define     SRC_OUT_BUFF_SIZE             (SRC_CHANNELS_PER_INSTANCE * SRC_MAX_NUM_SAMPS_OUT) // Size of output buffer for SRC for each instance

/* Stuff that must be defined for lib_src */
#define ASRC_N_IN_SAMPLES                 (SRC_N_IN_SAMPLES) /* Used by SRC_STACK_LENGTH_MULT in src_mrhf_asrc.h */

#define ASRC_N_CHANNELS                   (SRC_CHANNELS_PER_INSTANCE) /* Used by SRC_STACK_LENGTH_MULT in src_mrhf_asrc.h */


asrc_state_t sASRCState[SRC_CHANNELS_PER_INSTANCE];                                   // ASRC state machine state
int iASRCStack[SRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES * 100]; // Buffer between filter stages
asrc_ctrl_t sASRCCtrl[SRC_CHANNELS_PER_INSTANCE];                                     // Control structure
asrc_adfir_coefs_t asrc_adfir_coefs;                                                  // Adaptive filter coefficients

uint64_t fs_ratio = 0;
int ideal_fs_ratio = 0;

void my_init() {
    int inputFsCode = INPUT_CODE
    int outputFsCode = FS_CODE_48;

    for(int ui = 0; ui < SRC_CHANNELS_PER_INSTANCE; ui++)
    {
            // Set state, stack and coefs into ctrl structure
            sASRCCtrl[ui].psState                   = &sASRCState[ui];
            sASRCCtrl[ui].piStack                   = iASRCStack[ui];
            sASRCCtrl[ui].piADCoefs                 = asrc_adfir_coefs.iASRCADFIRCoefs;
    }

    fs_ratio = asrc_init(inputFsCode, outputFsCode, sASRCCtrl, SRC_CHANNELS_PER_INSTANCE, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);
    ideal_fs_ratio = fs_ratio >> 32;
}
    
int asrc_convert_quad_input(int out_samples[SRC_OUT_BUFF_SIZE], int *samples, int32_t timestamp, int32_t *timestamp_out) {
    int sampsOut = asrc_process(samples, out_samples, fs_ratio, sASRCCtrl);
    *timestamp_out = asrc_timestamp_interpolation(timestamp, &sASRCCtrl[0], BASE_FREQUENCY);
    return sampsOut;
}


DECLARE_JOB(producer, (asynchronous_fifo_t *));
DECLARE_JOB(consumer, (asynchronous_fifo_t *));

#define seconds 10
#define OFFSET 0 // 0x70000000

int32_t input_data[48] = {
    0,
    70075715,
    138952416,
    205451603,
    268435456,
    326826303,
    379625062,
    425928331,
    464943848,
    496004047,
    518577479,
    532277906,
    536870912,
    532277906,
    518577479,
    496004047,
    464943848,
    425928331,
    379625062,
    326826303,
    268435455,
    205451603,
    138952416,
    70075715,
    0,
    -70075715,
    -138952416,
    -205451603,
    -268435456,
    -326826303,
    -379625062,
    -425928331,
    -464943848,
    -496004047,
    -518577479,
    -532277906,
    -536870912,
    -532277906,
    -518577479,
    -496004047,
    -464943848,
    -425928331,
    -379625062,
    -326826303,
    -268435455,
    -205451603,
    -138952416,
    -70075715,
};


void producer(asynchronous_fifo_t *a) {
    hwtimer_t tmr = hwtimer_alloc();
    uint64_t now = hwtimer_get_time(tmr);
    int freq = (INPUT_FREQUENCY + 12)/4;
    int step = 100000000 / freq;
    int mod  = 100000000 % freq;
    int mod_acc = 0;
    int sine_cnt = 0;
    my_init();
    int in_samples[4];
    int out_samples[5];
    
    for(int32_t i = 0; i < INPUT_FREQUENCY * seconds; i+=4) {
        now += step;
        mod_acc += mod;
        if (mod_acc >= freq) {
            mod_acc -= freq;
            now++;
        }
        for(int j = 0; j < 4; j++) {
            in_samples[j] = input_data[sine_cnt];
            sine_cnt++;
            if (sine_cnt == 48) sine_cnt = 0;
        }
        hwtimer_set_trigger_time(tmr, now);
        int32_t ts;
        (void) hwtimer_get_time(tmr);
        int num_samples = asrc_convert_quad_input(out_samples, in_samples, now, &ts);
        xscope_int(5, fs_ratio >> 32);
        int32_t error;
        for(int j = 0; j < num_samples; j++) {
            error = asynchronous_fifo_produce(a, (int32_t *)&out_samples[j], ts+OFFSET,
                                                      j == num_samples - 1);
        }
        fs_ratio = (((int64_t)ideal_fs_ratio) << 32) + (error * (int64_t) ideal_fs_ratio);
    }
    hwtimer_free(tmr);
}

void consumer(asynchronous_fifo_t *a) {
    hwtimer_t tmr = hwtimer_alloc();
    uint64_t now = hwtimer_get_time(tmr);
    int freq = 48012;                  // Very close to 48012
    int step = 100000000 / freq;
    int mod = 100000000 % freq;
    int mod_acc = 0;
    int32_t output_data;
    
    for(int i = 0; i < 48000 * seconds; i++) {
        now += step;
        mod_acc += mod;
        if (mod_acc >= freq) {
            mod_acc -= freq;
            now++;
        }
        hwtimer_set_trigger_time(tmr, now);
        (void) hwtimer_get_time(tmr);
//        printchar('*');
        asynchronous_fifo_consume(a, &output_data, now + OFFSET);
        xscope_int(0, output_data);
        if (i == 24000) {
            freq = 47993;
            step = 100000000 / freq;
            mod = 100000000 % freq;
        }
    }
    hwtimer_free(tmr);
}

#define FIFO_LENGTH   100

int main(void) {
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_LENGTH, 1)];
    asynchronous_fifo_t *asynchronous_fifo_state = (asynchronous_fifo_t *)array;

    asynchronous_fifo_init(asynchronous_fifo_state, 1, FIFO_LENGTH,
                           100000000/48000, 4);
    PAR_JOBS(
        PJOB(producer, (asynchronous_fifo_state)),
        PJOB(consumer, (asynchronous_fifo_state))
        );
    asynchronous_fifo_exit(asynchronous_fifo_state);

}


