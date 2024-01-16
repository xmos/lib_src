#include <xcore/parallel.h>
#include <xcore/hwtimer.h>
#include <xs1.h>
#include <stdlib.h>
#include <stdio.h>
#include <print.h>
#include <platform.h>
#include <xscope.h>
#include "asynchronous_fifo.h"
#include "asrc_timestamp_interpolation.h"

#include <string.h>

#include "src.h"


int fs_code(int frequency) {
    if(frequency == 44100) {
        return  FS_CODE_44;
    } else if(frequency == 48000) {
        return  FS_CODE_48;
    } else if(frequency == 88200) {
        return  FS_CODE_88;
    } else if(frequency == 96000) {
        return  FS_CODE_96;
    } else if(frequency == 176400) {
        return  FS_CODE_176;
    } else if(frequency == 192000) {
        return  FS_CODE_192;
    } else {
        exit(1);
    }
}

int base_frequency(int frequency) {
    if(frequency == 44100) {
        return 44100;
    } else if(frequency == 48000) {
        return 48000;
    } else if(frequency == 88200) {
        return 44100;
    } else if(frequency == 96000) {
        return 48000;
    } else if(frequency == 176400) {
        return 44100;
    } else if(frequency == 192000) {
        return 48000;
    } else {
        exit(1);
    }
}

int base_frequency_multiplier(int frequency) {
    if(frequency == 44100) {
        return 1;
    } else if(frequency == 48000) {
        return 1;
    } else if(frequency == 88200) {
        return 2;
    } else if(frequency == 96000) {
        return 2;
    } else if(frequency == 176400) {
        return 4;
    } else if(frequency == 192000) {
        return 4;
    } else {
        exit(1);
    }
}

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


#define POSITIVE_DEVIATION(F)    ((F) + ((F)*12)/48000)
#define NEGATIVE_DEVIATION(F)    ((F) - ((F)* 8)/48000)

extern int async_resets;

DECLARE_JOB(producer,   (asynchronous_fifo_t *, int, int, int, int));
DECLARE_JOB(consumer,   (asynchronous_fifo_t *, int, int));
DECLARE_JOB(test_async, (int, int, int, float, float, int));

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


void producer(asynchronous_fifo_t *a, int input_frequency, int output_frequency, int xscope_used, int interpolation_ticks) {
    asrc_state_t sASRCState[SRC_CHANNELS_PER_INSTANCE];                                   // ASRC state machine state
    int iASRCStack[SRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES * 100]; // Buffer between filter stages
    asrc_ctrl_t sASRCCtrl[SRC_CHANNELS_PER_INSTANCE];                                     // Control structure
    asrc_adfir_coefs_t asrc_adfir_coefs;                                                  // Adaptive filter coefficients

    uint64_t fs_ratio = 0;
    int ideal_fs_ratio = 0;

    int freq = POSITIVE_DEVIATION(input_frequency)/4;
    int step = 100000000 / freq;
    int mod  = 100000000 % freq;
    int mod_acc = 0;
    int sine_cnt = 0;

    int inputFsCode = fs_code(input_frequency);
    int outputFsCode = fs_code(output_frequency);

    for(int ui = 0; ui < SRC_CHANNELS_PER_INSTANCE; ui++)
    {
            // Set state, stack and coefs into ctrl structure
            sASRCCtrl[ui].psState                   = &sASRCState[ui];
            sASRCCtrl[ui].piStack                   = iASRCStack[ui];
            sASRCCtrl[ui].piADCoefs                 = asrc_adfir_coefs.iASRCADFIRCoefs;
    }

    fs_ratio = asrc_init(inputFsCode, outputFsCode, sASRCCtrl, SRC_CHANNELS_PER_INSTANCE, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);
    ideal_fs_ratio = fs_ratio >> 32;

    int in_samples[4];
    int out_samples[5];
    int expected_error = (POSITIVE_DEVIATION(1LL<<32) - NEGATIVE_DEVIATION(1LL<<32));
    int expected_error_high = expected_error * 101/100;
    int expected_error_low  = expected_error *  99/100;
    hwtimer_t tmr = hwtimer_alloc();
    uint64_t now = hwtimer_get_time(tmr);
    int start = now;
    int good_error_count = 0;
    int stable_time = now;
    int32_t error;
//    printhexln(ideal_fs_ratio);
    for(int32_t i = 0; i < input_frequency * seconds; i+=4) {
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
        (void) hwtimer_get_time(tmr);
        int num_samples = asrc_process(in_samples, out_samples, fs_ratio, sASRCCtrl);
        int ts = asrc_timestamp_interpolation(now, &sASRCCtrl[0], interpolation_ticks);
        if (xscope_used) xscope_int(5, fs_ratio >> 32);
        for(int j = 0; j < num_samples; j++) {
            error = asynchronous_fifo_produce(a, (int32_t *)&out_samples[j], ts+OFFSET,
                                              j == num_samples - 1, xscope_used);
        }
        if (error > expected_error_low && error < expected_error_high) {
            good_error_count++;
        } else {
            stable_time = now;
            good_error_count = 0;
        }
        fs_ratio = (((int64_t)ideal_fs_ratio) << 32) + (error * (int64_t) ideal_fs_ratio);
    }
    printf("Error %08lx low %08x high %08x\n", error, expected_error_low, expected_error_high);
    printf("Stable counter %d from %d ms input %6d output %6d total resets %d\n",
           good_error_count, (stable_time - start)/100000, input_frequency, output_frequency, async_resets);
    hwtimer_free(tmr);
}

void consumer(asynchronous_fifo_t *a, int output_frequency, int xscope_used) {
    hwtimer_t tmr = hwtimer_alloc();
    uint64_t now = hwtimer_get_time(tmr);
    int freq = POSITIVE_DEVIATION(output_frequency);
    int step = 100000000 / freq;
    int mod = 100000000 % freq;
    int mod_acc = 0;
    int32_t output_data;

    for(int i = 0; i < output_frequency * seconds; i++) {
        now += step;
        mod_acc += mod;
        if (mod_acc >= freq) {
            mod_acc -= freq;
            now++;
        }
        hwtimer_set_trigger_time(tmr, now);
        (void) hwtimer_get_time(tmr);
        asynchronous_fifo_consume(a, &output_data, now + OFFSET);
        if (xscope_used) xscope_int(0, output_data);
        if (i == output_frequency/2) {
            freq = NEGATIVE_DEVIATION(output_frequency);
            step = 100000000 / freq;
            mod = 100000000 % freq;
        }
    }
    hwtimer_free(tmr);
}

#define FIFO_LENGTH   100

void test_async(int input_frequency, int output_frequency, int xscope_used,
                float speedup_p, float speedup_i, int interpolation_ticks) {
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_LENGTH, 1)];
    asynchronous_fifo_t *asynchronous_fifo_state = (asynchronous_fifo_t *)array;

    asynchronous_fifo_init(asynchronous_fifo_state, 1, FIFO_LENGTH,
                           100000000/output_frequency,
                           speedup_p, speedup_i);
    PAR_JOBS(
        PJOB(producer, (asynchronous_fifo_state, input_frequency, output_frequency, xscope_used, interpolation_ticks)),
        PJOB(consumer, (asynchronous_fifo_state, output_frequency, xscope_used))
        );
    asynchronous_fifo_exit(asynchronous_fifo_state);

}

int main(void) {
    hwtimer_free_xc_timer();
    PAR_JOBS(
//        PJOB(test_async, (44100, 44100, 1, 1.0, 2.0, 2268)), // OK, 4 sec or so, but test harness does not detect stability.
//        PJOB(test_async, (44100, 48000, 1, 1.0, 2.0, 2268)), // OK, 4 sec or so, but test harness does not detect stability. Slight overshoot
//        PJOB(test_async, (44100, 88200, 1, 2.0, 2.0, 2268)), // OK, 4 sec or so, but test harness does not detect stability.
//        PJOB(test_async, (44100, 96000, 1, 2.0, 2.0, 2268)), // OK  4 sec or so, but test harness does not detect stability.
//        PJOB(test_async, (48000, 44100, 1, 1.0, 2.0, 2083)), // OK, 4 sec or so, but test harness does not detect stability.
//        PJOB(test_async, (48000, 48000, 1, 1.0, 2.0, 2083)), // OK  4.16 s stable slight overshoot
//        PJOB(test_async, (48000, 88200, 1, 2.0, 2.0, 2083)), // OK, 4 sec or so, but test harness does not detect stability.
//        PJOB(test_async, (48000, 96000, 1, 2.0, 2.0, 2083)), // OK  4.16 s stable
//        PJOB(test_async, (88200, 44100, 1, 0.5, 1.0, 2268)), // OK, 4 sec or so, but test harness does not detect stability.
//        PJOB(test_async, (88200, 48000, 1, 0.5, 1.0, 2268)), // OK, 4 sec or so, but test harness does not detect stability. Slight overshoot, phase error not zero at the end?
//        PJOB(test_async, (88200, 88200, 1, 1.0, 1.0, 1134)), // OK, 4 sec or so, but test harness does not detect stability. Slight overshoot
//        PJOB(test_async, (88200, 96000, 1, 1.0, 1.0, 1134)), // OK  4 sec or so, but test harness does not detect stability. Slight overshoot
//        PJOB(test_async, (96000, 44100, 1, 0.5, 1.0, 2083)), // OK, 4 sec or so, but test harness does not detect stability. Non zero phase difference
//        PJOB(test_async, (96000, 48000, 1, 0.5, 1.0, 2083)), // OK  4.16 s stable slight overshoot
//        PJOB(test_async, (96000, 88200, 1, 1.0, 1.0, 1042)), // OK, 4 sec or so, but test harness does not detect stability. Phase error not zero.
//        PJOB(test_async, (96000, 96000, 1, 1.0, 1.0, 1042)), // OK  4.14 s stable
        );
    hwtimer_realloc_xc_timer();
}


