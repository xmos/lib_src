#include <xcore/parallel.h>
#include <xcore/hwtimer.h>
#include <xs1.h>
#include <stdlib.h>
#include <stdio.h>
#include <print.h>
#include <platform.h>
#include <xscope.h>
#include "asynchronous_fifo.h"
#include "check_freq.h"
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
        printf("Illegal freq %d\n", frequency);
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


#define POSITIVE_DEVIATION(F, dev)    ((F) + ((F)*dev)/48000)
#define NEGATIVE_DEVIATION(F, dev)    ((F) + ((F)*dev)/48000)

extern int async_resets;

DECLARE_JOB(producer,   (asynchronous_fifo_t *, int, int, int, int *, int, int));
DECLARE_JOB(consumer,   (asynchronous_fifo_t *, int, int, int *, int, int));
DECLARE_JOB(test_async, (int, int, int, int *, int, int));

#define seconds 10
#define OFFSET 0 // 0x70000000

int32_t sine11[11] = {
    0,
    9070447,
    15261092,
    16606448,
    12679373,
    4726687,
    -4726687,
    -12679373,
    -16606448,
    -15261092,
    -9070447,
};

int32_t sine_wave[16] = {
    0,
    6420362,
    11863283,
    15500126,
    16777216,
    15500126,
    11863283,
    6420362,
    0,
    -6420362,
    -11863283,
    -15500126,
    -16777216,
    -15500126,
    -11863283,
    -6420362,
};

int32_t sine2_wave[19] = {
    0,
    5447553,
    10304779,
    14045322,
    16263837,
    16719913,
    15364126,
    12343398,
    7985072,
    2761438,
    -2761438,
    -7985072,
    -12343398,
    -15364126,
    -16719913,
    -16263837,
    -14045322,
    -10304779,
    -5447553,
};


static int interpolation_ticks_2D[6][6] = {
    {  2268, 2268, 2268, 2268, 2268, 2268},
    {  2083, 2083, 2083, 2083, 2083, 2083},
    {  2268, 2268, 1134, 1134, 1134, 1134},
    {  2083, 2083, 1042, 1042, 1042, 1042},
    {  2268, 2268, 1134, 1134,  567,  567},
    {  2083, 2083, 1042, 1042,  521,  521}
};

void producer(asynchronous_fifo_t *a, int input_frequency, int output_frequency, int xscope_used, int *errors, int pos_deviation, int neg_deviation) {
    int interpolation_ticks = interpolation_ticks_2D[fs_code(input_frequency)][fs_code(output_frequency)];
    asrc_state_t sASRCState[SRC_CHANNELS_PER_INSTANCE];                                   // ASRC state machine state
    int iASRCStack[SRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES * 100]; // Buffer between filter stages
    asrc_ctrl_t sASRCCtrl[SRC_CHANNELS_PER_INSTANCE];                                     // Control structure
    asrc_adfir_coefs_t asrc_adfir_coefs;                                                  // Adaptive filter coefficients

    uint64_t fs_ratio = 0;
    int ideal_fs_ratio = 0;

    int freq = POSITIVE_DEVIATION(input_frequency, pos_deviation)/4;
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
    ideal_fs_ratio = (fs_ratio + (1<<31)) >> 32;

    int in_samples[4];
    int out_samples[25];
    int freq1 = NEGATIVE_DEVIATION(output_frequency, neg_deviation);
    int freq2 = (POSITIVE_DEVIATION(input_frequency, pos_deviation)/4)*4;
    int expected_ratio = ideal_fs_ratio * (int64_t) freq2 / freq1 * output_frequency / input_frequency;
    int expected_ratio_high = expected_ratio * 1000005LL/1000000;
    int expected_ratio_low  = expected_ratio *  999995LL/1000000;
    hwtimer_t tmr = hwtimer_alloc();
    uint64_t now = hwtimer_get_time(tmr);
    int start = now;
    int good_error_count = 0;
    int stable_time = now;
    int32_t error;

    int t0, t1, t2, t3;
    for(int32_t i = 0; i < input_frequency * seconds; i+=4) {
        now += step;
        mod_acc += mod;
        if (mod_acc >= freq) {
            mod_acc -= freq;
            now++;
        }
        for(int j = 0; j < 4; j++) {
            in_samples[j] = sine_wave[sine_cnt];
            sine_cnt++;
            if (sine_cnt == 16) sine_cnt = 0;
        }
        asm volatile("gettime %0" : "=r" (t0));
        hwtimer_set_trigger_time(tmr, now);
        (void) hwtimer_get_time(tmr);
        asm volatile("gettime %0" : "=r" (t1));
        int num_samples = asrc_process(in_samples, out_samples, fs_ratio, sASRCCtrl);
        if (num_samples) {
            asm volatile("gettime %0" : "=r" (t2));
            int ts = asrc_timestamp_interpolation(now, &sASRCCtrl[0], interpolation_ticks);
            if (xscope_used) xscope_int(5, fs_ratio >> 32);
            error = asynchronous_fifo_produce(a, (int32_t *)out_samples, num_samples, ts+OFFSET,
                                              xscope_used);
            asm volatile("gettime %0" : "=r" (t3));
            if (i == 48008) {
//                printf("%d %d %d %d\n", t1-t0, t2-t1, t3-t2, t3-t0);
            }
            
            fs_ratio = (((int64_t)ideal_fs_ratio) << 32) + (error * (int64_t) ideal_fs_ratio);
        }
        int fs = fs_ratio >> 32;
        if (fs > expected_ratio_low && fs < expected_ratio_high) {
            good_error_count++;
        } else {
            stable_time = now;
            good_error_count = 0;
        }
    }
    hwtimer_free(tmr);
    int ms = (stable_time - start)/100000;
    if (ms > 3500 && ms < 5400) {
        *errors = 0;
    } else {
        printf("Failure: ratio %08x low %08x high %08x\n", (int)(fs_ratio >> 32), expected_ratio_low, expected_ratio_high);
        printf("         stable counter %d from %d ms input %6d output %6d total resets %d\n",
               good_error_count, ms, input_frequency, output_frequency, async_resets);
        *errors = 1;
    }
}

void consumer(asynchronous_fifo_t *a, int output_frequency, int xscope_used, int *limits, int pos_deviation, int neg_deviation) {
    hwtimer_t tmr = hwtimer_alloc();
    uint64_t now = hwtimer_get_time(tmr);
    int freq = POSITIVE_DEVIATION(output_frequency, pos_deviation);
    int step = 100000000 / freq;
    int mod = 100000000 % freq;
    int mod_acc = 0;
    int32_t output_data;
    int check_array[1<<(LOG_FFT_BINS+1)];
    int check_n = 0;

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
            freq = NEGATIVE_DEVIATION(output_frequency, neg_deviation);
            step = 100000000 / freq;
            mod = 100000000 % freq;
        }
        if (i < (output_frequency * seconds)-4000) {
            check_n = check_freq(check_array, output_data, check_n, limits, i > 4000);
        }
    }
    hwtimer_free(tmr);
}


int energies[6][6][(1<<(LOG_FFT_BINS-1))+1];
int acceptable_energy_limits[6][6][(1<<(LOG_FFT_BINS-1))+1] =
{
    {
        { 5, 5, 7,30,31,30, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        {19,20,25,31,31,28,21,18,15,14,12,11,10, 9, 8, 7, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 7,29,31,30, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        {24,30,31,29,20,17,14,13,11,10, 9, 8, 7, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        {31,31,29, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        {31,31,29,19,15,13,11,10, 8, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,},
    },{
        {17,18,21,28,31,31,25,20,17,15,13,12,11,10, 9, 8, 7, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 5, 5, 8,29,31,30, 8, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        {22,29,31,30,22,18,15,13,12,10, 9, 8, 7, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 8,29,31,30, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        {31,32,30,20,16,14,12,10, 9, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,},
        {31,31,30, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
    },{
        { 7, 7, 7, 7, 7, 7,10,30,32,30,10, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,},
        {13,13,14,16,18,21,28,31,31,25,20,17,15,13,12,10, 9, 8, 8, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 5, 5, 8,29,31,30, 8, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        {19,20,25,31,31,28,21,18,15,14,12,11,10, 9, 8, 7, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 8,29,31,30, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        {24,30,31,29,20,17,14,13,11,10, 9, 8, 7, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
    },{
        {11,11,12,13,14,16,19,24,31,31,28,21,18,15,13,12,10, 9, 8, 7, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 7, 7, 7, 7, 7, 7,10,30,32,30,10, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,},
        {17,18,21,28,31,31,25,20,17,15,13,12,11,10, 9, 8, 7, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 5, 5, 8,29,31,30, 8, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        {22,29,31,30,22,18,15,13,12,10, 9, 8, 7, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 8,29,31,30, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
    },{
        { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8,12,30,31,30,12, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,},
        { 6, 6, 7, 7, 8, 8, 9,10,11,13,14,16,19,24,31,31,28,21,18,15,13,12,10, 9, 8, 8, 7, 6, 6, 5, 5, 5, 5,},
        { 7, 7, 7, 7, 7, 7,10,30,32,30,10, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,},
        {13,13,14,16,18,21,28,31,31,25,20,17,15,13,12,10, 9, 8, 8, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 7, 7, 8,30,32,30, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,},
        {19,20,25,31,31,28,21,18,15,14,12,11,10, 9, 8, 7, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
    },{
        { 5, 5, 5, 6, 6, 7, 7, 8, 9,10,11,12,14,15,18,21,27,31,31,26,20,17,15,13,12,11,10, 9, 8, 8, 7, 7, 7,},
        { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8,12,30,31,30,12, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,},
        {11,11,12,13,14,16,19,24,31,31,28,21,18,15,13,12,10, 9, 8, 7, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 7, 7, 7, 7, 7, 7,10,30,32,30,10, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,},
        {17,18,21,28,31,31,25,20,17,15,13,12,11,10, 9, 8, 7, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,},
        { 7, 7, 8,30,32,30, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,},
    }
};

void clear_energies() {
    memset(energies, 0, sizeof(energies));
}

int verify_energy_limits() {
    int errors = 0;
    for(int i = 0; i < 6; i++) {
        for(int j = 0; j < 6; j++) {
            int faulty_bin = 0;
            for(int b = 0; b <= (1<<(LOG_FFT_BINS-1)); b++) {
                if (energies[i][j][b] > acceptable_energy_limits[i][j][b]) {
                    faulty_bin = 1;
                    errors++;
                }
            }
            if (faulty_bin) {
                printf("Energy limit exceeded, speed code %d, %d\n", i, j);
                for(int b = 0; b <= (1<<(LOG_FFT_BINS-1)); b++) {
                    printf("  Bin %d energy %2d acceptable %2d%s\n", b,
                           energies[i][j][b], acceptable_energy_limits[i][j][b],
                           energies[i][j][b] > acceptable_energy_limits[i][j][b] ? " Failed" : "");
                }
            }
            if (0) { //  Print acceptable energies table
                printf("  {");
                for(int b = 0; b <= (1<<(LOG_FFT_BINS-1)); b++) {
                    printf("%2d,", energies[i][j][b]);
                }
                printf("},\n");
            }
        }
    }
    return errors;
}

#define FIFO_LENGTH   100

void test_async(int input_frequency, int output_frequency, int xscope_used,
                int *errors, int pos_deviation, int neg_deviation) {
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_LENGTH, 1)];
    asynchronous_fifo_t *asynchronous_fifo_state = (asynchronous_fifo_t *)array;

    *errors = 1;
    asynchronous_fifo_init(asynchronous_fifo_state, 1, FIFO_LENGTH);
    asynchronous_fifo_init_PID_fs_codes(asynchronous_fifo_state,
                                        fs_code(input_frequency),
                                        fs_code(output_frequency));

    PAR_JOBS(
        PJOB(producer, (asynchronous_fifo_state, input_frequency,
                        output_frequency, xscope_used, errors,
                        pos_deviation, neg_deviation)),
        PJOB(consumer, (asynchronous_fifo_state, output_frequency, xscope_used,
                        energies[fs_code(input_frequency)][fs_code(output_frequency)],
                        pos_deviation, neg_deviation))
        );
    asynchronous_fifo_exit(asynchronous_fifo_state);

}

int test_44100_low(int pos_deviation, int neg_deviation) {
    int e0, e1, e2, e3;
    printf("Testing 44100 low pos %d neg %d\n", pos_deviation, neg_deviation);
    PAR_JOBS(
        PJOB(test_async, (44100, 44100, 0, &e0, pos_deviation, neg_deviation)), // OK, 4 sec or so.
        PJOB(test_async, (44100, 48000, 0, &e1, pos_deviation, neg_deviation)), // OK, 4 sec or so. Slight overshoot
        PJOB(test_async, (44100, 88200, 0, &e2, pos_deviation, neg_deviation)), // OK, 4 sec or so.
        PJOB(test_async, (44100, 96000, 0, &e3, pos_deviation, neg_deviation)) // OK  4 sec or so.
        );
    return e0 + e1 + e2 + e3;
}

int test_48000_low(int pos_deviation, int neg_deviation) {
    int e0, e1, e2, e3;
    printf("Testing 48000 low pos %d neg %d\n", pos_deviation, neg_deviation);
    PAR_JOBS(
        PJOB(test_async, (48000, 44100, 0, &e0, pos_deviation, neg_deviation)), // OK, 4 sec or so
        PJOB(test_async, (48000, 48000, 0, &e1, pos_deviation, neg_deviation)), // OK  4.16 s stable slight overshoot
        PJOB(test_async, (48000, 88200, 0, &e2, pos_deviation, neg_deviation)), // OK, 4 sec or so
        PJOB(test_async, (48000, 96000, 0, &e3, pos_deviation, neg_deviation)) // OK  4.16 s stable
        );
    return e0 + e1 + e2 + e3;
}

int test_4xx00_high(int pos_deviation, int neg_deviation) {
    int e0, e1, e2, e3;
    printf("Testing 44100/48000 high pos %d neg %d\n", pos_deviation, neg_deviation);
    PAR_JOBS(
        PJOB(test_async, (44100, 176400, 0, &e2, pos_deviation, neg_deviation)), // OK, 4 sec or so.
        PJOB(test_async, (44100, 192000, 0, &e3, pos_deviation, neg_deviation)), // OK  4 sec or so.
        PJOB(test_async, (48000, 176400, 0, &e0, pos_deviation, neg_deviation)), // OK, 4 sec or so
        PJOB(test_async, (48000, 192000, 0, &e1, pos_deviation, neg_deviation)) // OK  4.16 s stable
        );
    return e0 + e1 + e2 + e3;
}

int test_88200_low(int pos_deviation, int neg_deviation) {
    int e0, e1, e2, e3;
    printf("Testing 88200 low pos %d neg %d\n", pos_deviation, neg_deviation);
    PAR_JOBS(
        PJOB(test_async, (88200, 44100, 0, &e0, pos_deviation, neg_deviation)), // OK, 4 sec or so
        PJOB(test_async, (88200, 48000, 0, &e1, pos_deviation, neg_deviation)), // OK, 4 sec or so. Slight overshoot, phase error not zero at the end?
        PJOB(test_async, (88200, 88200, 0, &e2, pos_deviation, neg_deviation)), // OK, 4 sec or so. Slight overshoot
        PJOB(test_async, (88200, 96000, 0, &e3, pos_deviation, neg_deviation))  // OK  4 sec or so. Slight overshoot
        );
    return e0 + e1 + e2 + e3;
}

int test_96000_low(int pos_deviation, int neg_deviation) {
    int e0, e1, e2, e3;
    printf("Testing 96000 low pos %d neg %d\n", pos_deviation, neg_deviation);
    PAR_JOBS(
        PJOB(test_async, (96000, 44100, 0, &e0, pos_deviation, neg_deviation)), // OK, 4 sec or so. Non zero phase difference
        PJOB(test_async, (96000, 48000, 0, &e1, pos_deviation, neg_deviation)), // OK  4.16 s stable slight overshoot
        PJOB(test_async, (96000, 88200, 0, &e2, pos_deviation, neg_deviation)), // OK, 4 sec or so. Phase error not zero
        PJOB(test_async, (96000, 96000, 0, &e3, pos_deviation, neg_deviation)) // OK  4.14 s stable
        );
    return e0 + e1 + e2 + e3;
}

int test_9xx00_high(int pos_deviation, int neg_deviation) {
    int e0, e1, e2, e3;
    printf("Testing 44100/48000 high pos %d neg %d\n", pos_deviation, neg_deviation);
    PAR_JOBS(
        PJOB(test_async, (88200, 176400, 0, &e2, pos_deviation, neg_deviation)), // OK, 4 sec or so. Slight overshoot
        PJOB(test_async, (88200, 192000, 0, &e3, pos_deviation, neg_deviation)), // OK  4 sec or so. Slight overshoot
        PJOB(test_async, (96000, 176400, 0, &e0, pos_deviation, neg_deviation)), // OK, 4 sec or so. Phase error not zero
        PJOB(test_async, (96000, 192000, 0, &e1, pos_deviation, neg_deviation)) // OK  4.14 s stable
        );
    return e0 + e1 + e2 + e3;
}

int test_176400_low(int pos_deviation, int neg_deviation) {
    int e0, e1, e2, e3;
    printf("Testing 176400 low pos %d neg %d\n", pos_deviation, neg_deviation);
    PAR_JOBS(
        PJOB(test_async, (176400, 44100, 0, &e0, pos_deviation, neg_deviation)),
        PJOB(test_async, (176400, 48000, 0, &e1, pos_deviation, neg_deviation)),
        PJOB(test_async, (176400, 88200, 0, &e2, pos_deviation, neg_deviation)),
        PJOB(test_async, (176400, 96000, 0, &e3, pos_deviation, neg_deviation)) // phase != 0?
        );
    return e0 + e1 + e2 + e3;
}

int test_192000_low(int pos_deviation, int neg_deviation) {
    int e0, e1, e2, e3;
    printf("Testing 192000 low pos %d neg %d\n", pos_deviation, neg_deviation);
    PAR_JOBS(
        PJOB(test_async, (192000, 44100, 0, &e0, pos_deviation, neg_deviation)),
        PJOB(test_async, (192000, 48000, 0, &e1, pos_deviation, neg_deviation)),
        PJOB(test_async, (192000, 88200, 0, &e2, pos_deviation, neg_deviation)),
        PJOB(test_async, (192000, 96000, 0, &e3, pos_deviation, neg_deviation)) 
        );
    return e0 + e1 + e2 + e3;
}

int test_1xxx00_high(int pos_deviation, int neg_deviation) {
    int e0, e1, e2, e3;
    printf("Testing 176400/192000 high pos %d neg %d\n", pos_deviation, neg_deviation);
    PAR_JOBS(
        PJOB(test_async, (176400, 176400, 0, &e2, pos_deviation, neg_deviation)),
        PJOB(test_async, (176400, 192000, 0, &e3, pos_deviation, neg_deviation)),
        PJOB(test_async, (192000, 176400, 0, &e0, pos_deviation, neg_deviation)),
        PJOB(test_async, (192000, 192000, 0, &e1, pos_deviation, neg_deviation))
        );
    return e0 + e1 + e2 + e3;
}

int expected_wave_192_176[] = {
    0, 0, -1, 0, 1, -113, -1666, -4125,
    -3173, -6060, -2355, -2794, -2629, 7180, -8051, 20960,
    -17509, 29082, -20493, 16435, 1817, -34651, 74278, -152720,
    260250, -557782, 2276461, 9597719, 14249465, 16664195, 16016233, 12413262,
    6715529, -348775, -7202070, -12880910, -16154368, -16575819, -13987774, -8890073,
    -2197458, 4900273, 11106279, 15321643, 16775586, 15213034, 10913009, 4647555,
    -2454378, -9114697, -14135009, -16612012, -16100020, -12691156, -6998773, -47105,
    6913040, 12629323, 16073214, 16625055, 14185553, 9193648, 2547531, -4556961,
    -10841522, -15175370, -16778717, -15363074, -11183157, -4991055, 2099083, 8811536,
    13938529, 16557567, 16197405, 12922847, 7323084, 405680, -6584718, -12390330,
    -15966552, -16669916, -14373867, -9491531, -2901385, 4210804, 10565347, 15018866,
    16770046, 15503794, 11447948, 5332274,
};

int expected_wave_48_192[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, -1, -1, -1, -1,
    -1, -1, -1, 2, 13, 41, 97, 185,
    299, 414, 486, 463, 301, -15, -455, -945,
    -1391, -1713, -1886, -1957, -2028, -2213, -2575, -3080,
    -3598, -3955, -4023, -3795, -3411, -3103, -3082, -3408,
    -3938, -4368, -4382, -3834, -2855, -1827, -1199, -1247,
    -1885, -2662, -2972, -2382, -904, 938, 2344, 2656,
    1757, 210, -979, -883, 791, 3455, 5860, 6738,
    5533, 2809, 58, -1052, 377, 3856, 7634, 9566,
    8313, 4206, -751, -3864, -3231, 1067, 6802, 10661,
    10059, 4711, -2956, -8937, -9802, -4750, 3673, 10714,
    11955, 5914, -4814, -14661, -18032, -12547, -717, 11049,
    15800, 10024, -3997, -18979, -26400, -21323, -5590, 12642,
    23053, 18956, 1490, -20336, -34165, -31155, -11422, 15232,
    34128, 33783, 12854, -18179, -42013, -43941, -20618, 16954,
    48473, 55236, 31137, -12587, -52199, -64009, -38127, 14400,
    65828, 86066, 59886, -2594, -68910, -100495, -74335, 1734,
    89597, 138905, 114555, 19259, -102215, -182096, -164746, -40915,
    137795, 275350, 275214, 94074, -219449, -521511, -611045, -296565,
    534410, 1871927, 3584412, 5463430, 7293626, 8919129, 10280497, 11410273,
    12393566, 13314391, 14212496, 15068032, 15816964, 16385710, 16726069, 16833629,
    16742845, 16504109, 16156361, 15709499, 15144350, 14428298, 13536876, 12469614,
    11252491, 9927018, 8532858, 7093593, 5612728, 4080995, 2490056, 844740,
    -832567, -2507084, -4142885, -5713844, -7208595, -8627445, -9973650, -11244206,
    -12425158, -13493613, -14424897, -15200679, -15813719, -16267026, -16568377, -16723516,
    -16731783, -16586319, -16278364, -15803026, -15163313, -14370421, -13440401, -12389249,
    -11229129, -9967593, -8609909, -7162935, -5638298, -4053167, -2428356, -784923,
    858888, 2488835, 4093622, 5662431, 7182557, 8638465, 10012713, 11288151,
    12450197, 13488032, 14394262, 15163406, 15790189, 16268580, 16592054, 16754849,
    16753474, 16587679, 16260423, 15776973, 15143673, 14367050, 13453662, 12410656,
    11246627, 9972270, 8600436, 7145593, 5622960, 4047749, 2434810, 798751,
    -845659, -2483117, -4097719, -5673260, -7193786, -8644161, -10010423, -11279856,
    -12440833, -13482608, -14395222, -15169608, -15797866, -16273581, -16592052, -16750368,
    -16747317, -16583223, -16259803, -15780102, -15148533, -14370949, -13454706, -12408638,
    -11242947, -9969029, -8599272, -7146898, -5625826, -4050590, -2436257, -798329,
    847396, 2484992, 4098598, 5672608, 7191836, 8641670, 10008192, 11278292,
    12439780, 13481494, 14393407, 15166731, 15794008, 16269189, 16587692, 16746446,
    16743922, 16580145, 16256691, 15776677, 15144726, 14366922, 13450757, 12405055,
    11239884, 9966467, 8597067, 7144873, 5623870, 4048707, 2434551, 796950,
    -848326, -2485432, -4098603, -5672301, -7191373, -8641187, -10007783, -11277997,
    -12439599, -13481399, -14393367, -15166719, -15794008, -16269191, -16587694, -16746448,
    -16743924, -16580147, -16256694, -15776680, -15144728, -14366924, -13450759, -12405057,
    -11239886, -9966469, -8597069, -7144875, -5623872, -4048709, -2434553, -796952,
    848324, 2485430, 4098601, 5672299, 7191371, 8641185, 10007781, 11277995,
    12439597, 13481397, 14393365, 15166717, 15794006, 16269189, 16587692, 16746446,
    16743922, 16580145, 16256692, 15776678, 15144726, 14366922, 13450757, 12405055,
    11239884, 9966467, 8597067, 7144873, 5623870, 4048707, 2434551, 796950,
    -848326, -2485432, -4098603, -5672301, -7191373, -8641187, -10007783, -11277997,
    -12439599, -13481399, -14393367, -15166719, -15794008, -16269191, -16587694, -16746448,
};


int expected_wave_192_44[] = {
    0, 0, -1, -1, 0, -2, -2, -3,
    16, 746, 3010, -327, -4127, 3013, 1099, -315,
    -4943, 6579, -2719, 826, -4738, 7900, -4513, -682,
    761, 1204, 2791, -12176, 19053, -22130, 29257, -44290,
    60849, -73853, 88921, -113628, 145772, -179262, 218021, -275299,
    362385, -501559, 730165, -1324576, 9247601, 12607227, -13434121, -8306100,
    15320089, 4349036, -16727914, 426458, 16512343, -4936645, -15181087, 9156141,
    12645649, -12669857, -9117130, 15169571, 4944364, -16568342, -326669, 16630710,
    -4252908, -15476423, 8559487, 13088374, -12169464, -9730069, 14873326, 5604137,
    -16420289, -1060192, 16716018, -3570238, -15726783, 7925388, 13531770, -11669330,
    -10298785, 14521492, 6277267, -16259892, -1774333, 16751264, -2864686, -15957935,
    7283997, 13940737, -11144682, -10854384, 14150640, 6935570, -16071347, -2484851,
    16759487, -2156445, -16162294, 6632350, 14325559, -10599606, -11390159, 13753939,
    7581205, -15853445, -3190832, 16737095, -1444264, -16337131, 5968588, 14684215,
    -10035168, -11905127, 13332114,
};

int expected_wave_44_192[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, -1, -1, -1,
    -1, -1, -1, -1, 0, 4, 17, 47,
    101, 182, 286, 393, 473, 485, 390, 167,
    -179, -610, -1058, -1451, -1731, -1886, -1952, -2010,
    -2151, -2435, -2861, -3351, -3779, -4015, -3989, -3731,
    -3372, -3100, -3073, -3343, -3816, -4268, -4441, -4159,
    -3428, -2457, -1588, -1151, -1301, -1920, -2638, -2978,
    -2579, -1387, 277, 1825, 2652, 2430, 1301, -136,
    -1063, -817, 762, 3194, 5519, 6708, 6162, 4053,
    1325, -685, -860, 1125, 4557, 7924, 9572, 8504,
    4940, 328, -3236, -3975, -1334, 3625, 8588, 11007,
    9327, 3851, -3237, -8764, -10034, -6197, 1166, 8598,
    12301, 9965, 2048, -8213, -16139, -17785, -11942, -962,
    10088, 15688, 12539, 1372, -13071, -24031, -25883, -16994,
    -856, 15206, 23319, 18883, 3099, -17144, -32194, -34148,
    -20879, 2443, 25299, 36544, 29834, 7137, -21552, -42458,
    -44476, -24859, 8731, 41104, 56358, 45636, 12089, -29618,
    -59482, -61518, -31988, 17626, 64815, 86046, 68230, 16205,
    -47792, -93206, -95278, -47856, 31432, 106979, 140426, 109108,
    19259, -92960, -174677, -179446, -90874, 64142, 220145, 296495,
    232074, 18394, -282586, -544808, -608789, -327424, 388511, 1541124,
    3042898, 4741817, 6465172, 8066626, 9460569, 10633127, 11628267, 12516554,
    13359762, 14184828, 14975693, 15683816, 16250764, 16632262, 16813692, 16811630,
    16662406, 16403966, 16059516, 15629771, 15096286, 14433303, 13622085, 12661216,
    11568657, 10375411, 9114291, 7809214, 6469694, 5092516, 3669212, 2195451,
    677941, -864108, -2403219, -3911271, -5366849, -6759055, -8086387, -9351620,
    -10555333, -11691190, -12744987, -13697661, -14530636, -15230913, -15793655, -16221304,
    -16519911, -16694564, -16746091, -16670428, -16460754, -16111210, -15620391, -14993031,
    -14239245, -13371847, -12403081, -11342287, -10195449, -8966662, -7660671, -6285205,
    -4852042, -3376360, -1874790, -363095, 1145475, 2640515, 4113478, 5555973,
    6958222, 8308395, 9593102, 10798751, 11913141, 12926571, 13832057, 14624658,
    15300331, 15854889, 16283546, 16581228, 16743435, 16767259, 16652066, 16399593,
    16013460, 15498373, 14859384, 14101542, 13230010, 12250548, 11170089, 9997104,
    8741603, 7414761, 6028355, 4594226, 3123981, 1628985, 120586, -1389608,
    -2889542, -4366773, -5808728, -7203084, -8538134, -9803018, -10987767, -12083217,
    -13080858, -13972746, -14751520, -15410548, -15944126, -16347681, -16617895, -16752718,
    -16751303, -16613894, -16341729, -15936992, -15402829, -14743399, -13963921, -13070680,
    -12070994, -10973110, -9786081, -8519642, -7184104, -5790271, -4349386, -2873078,
    -1373304, 137742, 1647726, 3144302, 4615240, 6048538, 7432533, 8755992,
    10008192, 11178992, 12258907, 13239176, 14111837, 14869791, 15506870, 16017885,
    16398677, 16646146, 16758278, 16734162, 16573994, 16279077, 15851808, 15295661,
    14615158, 13815831, 12904180, 11887615, 10774401, 9573589, 8294942, 6948856,
    5546274, 4098601, 2617605, 1115327, -396018, -1904144, -3396789, -4861817,
    -6287317, -7661702, -8973795, -10212929, -11369031, -12432700, -13395288, -14248971,
    -14986807, -15602798, -16091936, -16450244, -16674808, -16763803, -16716505, -16533298,
    -16215673, -15766211, -15188568, -14487439, -13668525, -12738483, -11704875, -10576105,
    -9361349, -8070482, -6714002, -5302936, -3848756, -2363285, -858600, 653066,
    2159422, 3648221, 5107360, 6524975, 7889541, 9189964, 10415671, 11556696,
    12603763, 13548359, 14382804, 15100315, 15695058, 16162198, 16497936, 16699542,
    16765379, 16694909, 16488707, 16148449, 15676900, 15077897, 14356307, 13517998,
    12569786, 11519378, 10375317, 9146901, 7844120, 6477565, 5058345, 3598001,
    2108404, 601666, -909964, -2414195, -3898799, -5351705, -6761101, -8115528,
    -9403975, -10615965, -11741645, -12771864, -13698245, -14513257, -15210274, -15783629,
    -16228660, -16541749, -16720351,
};

int expected_wave_176_96[] = {
    0, 0, 0, 0, 1, -2, -308, -2475,
    -4310, -2295, -2158, 3608, 1786, 5160, 1310, -2526,
    1545, -12986, 11996, -20880, 29935, -29605, 41328, -39092,
    36380, -33940, 10771, 12125, -65728, 147815, -330104, 7124569,
    15952136, 15562798, 8133531, -3811217, -13563344, -16730400, -11465982, -517471,
    10679509, 16587689, 14179791, 4754238, -7088522, -15357873, -16002337, -8648576,
    3003221, 13164547, 16761619, 12002607, 1262528, -10109086, -16439615, -14577383,
    -5449964, 6394050, 15050763, 16204968, 9281319, -2268878, -12688087, -16782537,
    -12511223, -2003311, 9503207, 16272574, 14930383, 6145696, -5702498, -14708108,
    -16382019, -9889817, 1532265, 12190540, 16772075, 12993068, 2737275, -8882995,
    -16075267, -15254337, -6829422, 4999820, 14336759, 16527105, 10479020, -792645,
    -11669195, -16728880, -13449553, -3465886, 8245452, 15846601, 15548534, 7499834,
    -4287382, -13937428, -16639936, -11047772, 51493, 11125084, 16653045, 13879797,
    4187744, -7591814, -15587003, -15812381, -8155603
};

int expected_wave_192_88[] = {
    0, 0, 0, 1, 0, -361, -3176, -5381,
    -625, -1199, 6792, 2673, -373, 4462, -18816, 19518,
    -31278, 40355, -32511, 30770, -8510, -33867, 83886, -179601, 
    328160, -700420, 5116585, 15759296, 14866902, 3984726, -9627695, -16760609,
    -12178056, 570522, 13110113, 16493610, 8641334, -5210262, -15454199, -15079107,
    -4349123, 9384566, 16654345, 12486627, -269152, -12837921, -16582663, -8932605,
    4856576, 15307603, 15237043, 4693387, -9076240, -16607362, -12723503, -94216,
    12599833, 16633459, 9234174, -4512170, -15157094, -15383875, -5036634, 8772514,
    16551921, 12954464, 452829, -12360051, -16677320, -9531524, 4165715, 14999673,
    15523693, 5377591, -8464768, -16488913, -13179494, -811224, 12114638, 16713572,
    9824530, -3817348, -14835393, -15656411, -5716081, 8153166, 16418379, 13398517,
    1169258, -11863680, -16742187, -10113040,
};

int expected_wave_176_88[] = {
    0, 0, 0, 0, 3, -87, -1570, -5144,
    -3576, -1711, 1509, 6747, -213, 8484, -11765, 6941,
    -15964, 8022, -1463, -2162, 26382, -41807, 68070, -108036,
    140369, -199710, 279323, -492885, 2184572, 12873131, 16805447, 10494906,
    -1597643, -13101062, -16620595, -10664538, 1744615, 12980531, 16713697, 10597571,
    -1700694, -13005577, -16702856, -10598741, 1696188, 13012597, 16695557, 10604984,
    -1700779, -13009652, -16697001, -10606586, 1696162, 13005292, 16696102, 10606564,
    -1696158, -13005288, -16696099, -10606560, 1696158, 13005291, 16696102, 10606564,
    -1696158, -13005288, -16696099, -10606560, 1696158, 13005291, 16696102, 10606564,
    -1696158, -13005288, -16696099, -10606560, 1696158, 13005291, 16696102, 10606564,
    -1696158, -13005288, -16696099, -10606560, 1696158, 13005291, 16696102, 10606564,
    -1696158, -13005288, -16696099, -10606560, 1696158, 13005291, 16696102, 10606564,
    -1696158, -13005288, -16696099, -10606560,
};

int expected_wave_88_48[] = {
    0, 0, 0, 0, 0, -9, -272, -1210,
    -1426, -915, -117, 1159, 1069, 1736, -396, 400,
    -2685, 351, -2803, 2786, -1740, 4702, -2586, 3898,
    -4834, 2464, -4823, 2594, -1519, 2062, 2495, -2835,
    6599, -12076, 14766, -23884, 31154, -41354, 58115, -75616,
    107876, -161028, 243857, -492038, 6463944, 15661472, 15803384, 8796628,
    -3105881, -13079742, -16829799, -11954512, -1310312, 10140840, 16412849, 14602981,
    5434697, -6380689, -15063090, -16201091, -9289096, 2271268, 12687900, 16786415,
    12514505, 2004286, -9502982, -16276520, -14932508, -6149238, 5703560, 14709268,
    16386256, 9891972, -1530747, -12192407, -16774893, -12996503, -2738887, 8883551,
    16078002, 15257716, 6831009, -5000978, -14340084, -16530940, -10481452, 792834,
    11671908, 16732768, 13452676, 3466697, -8247363, -15850277, -15552142, -7501571,
    4288383, 13940665, 16643804, 11050338, -51502, -11127665, -16656907, -13883017,
    -4188712, 7593580, 15590626, 15816054, 8157497,
};

int expected_wave_96_48[] = {
    0, 0, 0, -1, 0, 62, 545, 9,
    -2957, -3466, -3439, 33, 1880, 4114, 2771, 298,
    -1744, -5003, -816, -3694, 7033, -2674, 11450, -10515,
    11444, -20782, 17778, -25301, 31855, -30931, 43398, -46506,
    52020, -66814, 70800, -88144, 110114, -136656, 202911, -403858,
    2073791, 12977543, 16695793, 10601304, -1681800, -13020521, -16677600, -10629024,
    1715725, 12982523, 16717930, 10587842, -1674848, -13022124, -16680305, -10622929,
    1707046, 12993079, 16706112, 10600392, -1687661, -13009432, -16692553, -10611388,
    1696389, 13002704, 16697601, 10607754, -1693903, -13004313, -16696731, -10608323,
    1693629, 13003518, 16693630, 10604771, -1695864, -13003061, -16693240, -10604744,
    1695869, 13003067, 16693245, 10604748, -1695865, -13003061, -16693240, -10604744,
    1695869, 13003067, 16693245, 10604748, -1695865, -13003061, -16693240, -10604744,
    1695869, 13003067, 16693245, 10604748,
};

int unit_test_spline() {
//    int DWORD_ALIGNED iH[3] = {0x12345678, 0x87654321, -12314516};
    int DWORD_ALIGNED iH[3] = { -12314516, 0x87654321,0x12345678};
    int           *piPhase0 = &iADFirCoefs[0][63];
    int DWORD_ALIGNED piADCoefs1[16];
    int out63[16] = {
        0xffffde68,
        0xfff9c91a,
        0x001fb061,
        0x00387050,
        0xfe1347a3,
        0x054d01a1,
        0xf7101d35,
        0xd71ca2dd,
        0xf6dd5890,
        0x05573f10,
        0xfe13e0a5,
        0x0036b73e,
        0x00205038,
        0xfff9bbd0,
        0xffffdd51,
        0x00000000        
    };
    ASRC_prepare_coefs();
    src_mrhf_spline_coeff_gen_inner_loop_asm_xs3(piPhase0, iH, piADCoefs1, FILTER_DEFS_ADFIR_PHASE_N_TAPS);
//    src_mrhf_spline_coeff_gen_inner_loop_asm(piPhase0, iH, piADCoefs2, FILTER_DEFS_ADFIR_PHASE_N_TAPS);

    int errors = 0;
    for(int i =0 ; i < 16; i++) {
        if (abs(piADCoefs1[i] - out63[i]) > 2) {
            printf("%08x should be 0x%08x,\n", piADCoefs1[i], out63[i]);
            errors++;
        }
    }

    printf("Spline done\n");
    return errors;
}



int unit_test_asrc(int test_case) {
    int input_frequency;
    int output_frequency;
    int *expected_wave;
    int steps = 100;
    if (test_case == 0) {
        input_frequency = 48000;
        output_frequency = 192000;
        expected_wave = expected_wave_48_192;   // 6 OS2 UP_N_TAPS FILTER_DEFS_ASRC_FIR_UP_ID
    } else if (test_case == 1) {
        input_frequency = 192000;
        output_frequency = 176400;
        expected_wave = expected_wave_192_176;  // 9 FILTER_DEFS_ASRC_FIR_UP192176_ID
    } else if (test_case == 2) {
        input_frequency = 192000;
        output_frequency = 44100;
        expected_wave = expected_wave_192_44;   // 10 FILTER_DEFS_ASRC_FIR_DS_ID,           1  FILTER_DEFS_ASRC_FIR_BL9644_ID
        steps = 500;
    } else if (test_case == 3) {
        input_frequency = 44100;
        output_frequency = 192000;
        expected_wave = expected_wave_44_192;   // 6 FILTER_DEFS_ASRC_FIR_UP_ID
    } else if (test_case == 4) {
        input_frequency = 176400;
        output_frequency = 96000;
        expected_wave = expected_wave_176_96;   // 5 FILTER_DEFS_ASRC_FIR_BL17696_ID
        steps = 200;
    } else if (test_case == 5) {
        input_frequency = 192000;
        output_frequency = 88200;
        expected_wave = expected_wave_192_88;   // 4 FILTER_DEFS_ASRC_FIR_BL19288_ID
        steps = 200;
    } else if (test_case == 6) {
        input_frequency = 176400;
        output_frequency = 88200;
        expected_wave = expected_wave_176_88;   // 3 FILTER_DEFS_ASRC_FIR_BLF_ID
        steps = 200;
    } else if (test_case == 7) {
        input_frequency = 88200;
        output_frequency = 48000;
        expected_wave = expected_wave_88_48;   // 2 FILTER_DEFS_ASRC_FIR_BL8848_ID
        steps = 200;
    } else if (test_case == 8) {
        input_frequency = 96000;
        output_frequency = 48000;
        expected_wave = expected_wave_96_48;   // 0 FILTER_DEFS_ASRC_FIR_BL_ID
        steps = 200;
    } else {
        printf("Bad test case %d\n", test_case);
        return 1;
    }

    asrc_state_t sASRCState[SRC_CHANNELS_PER_INSTANCE];                                   // ASRC state machine state
    int iASRCStack[SRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES * 100]; // Buffer between filter stages
    asrc_ctrl_t sASRCCtrl[SRC_CHANNELS_PER_INSTANCE];                                     // Control structure
    asrc_adfir_coefs_t asrc_adfir_coefs;                                                  // Adaptive filter coefficients

    uint64_t fs_ratio = 0;
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

    int in_samples[4];
    int out_samples[20];
    int exp_cnt = 0;
    int errors = 0;
    for(int32_t i = 0; i < steps; i+=4) {
        for(int j = 0; j < 4; j++) {
            in_samples[j] = sine_wave[sine_cnt];
            sine_cnt++;
            if (sine_cnt == 16) sine_cnt = 0;
        }
        int num_samples = asrc_process(in_samples, out_samples, fs_ratio, sASRCCtrl);
        for(int j = 0; j < num_samples; j++) {
            if (abs(out_samples[j] - expected_wave[exp_cnt]) > 8) {
                printf("saw %d expected %d\n", out_samples[j], expected_wave[exp_cnt]);
                errors++;
            }
            exp_cnt++;
        }
    }
    printf("%d -> %d: %d errors\n", input_frequency, output_frequency, errors);
    return errors;
}

#define LOCAL_SRC_CHANNELS_PER_INSTANCE 2
int expected_wave_stereo_96_48[] = {
0, 0, 0, 0, 0, 0, -1, -1,
-1, -1, 61, 52, 542, 474, 11, 102,
-2957, -2405, -3468, -3268, -3442, -4043, 25, -1882,
1879, -307, 4106, 2667, 2779, 3531, 293, 3298,
-1729, 2243, -5022, -1758, -800, -1028, -3728, -6499,
7055, 798, -2708, -6509, 11486, 8364, -10543, -5906,
11479, 15200, -20808, -11410, 17790, 17733, -25312, -22450,
31822, 20929, -30877, -32292, 43297, 31124, -46331, -38631,
51773, 47133, -66448, -48195, 70269, 63981, -87413, -70170,
109021, 87492, -135029, -117483, 200276, 159165, -398455, -334786,
2041107, 1711848, 12937615, 11339981, 16704557, 16556746, 10642841, 14757901,
-1622332, 6760332, -12984351, -4127189, -16682474, -13223912, -10677896, -16803199,
1653542, -13234640, 12940958, -4151112, 16724975, 6748197, 10639412, 14738056,
-1606305, 16575311, -12978125, 11365894, -16687133, 1417744, -10679159, -9176915,
1633698, -15858606, 12945382, -15892148, 16713694, -9191661, 10660510, 1356032,
-1608961, 11355651, -12958455, 16547915, -16700625, 14778460, -10675581, 6765761,
1612414, -4090501, 12948398, -13228705, 16706130, -16784578, 10676052, -13266431,
-1604727, -4152915, -12946621, 6710784, -16705806, 14745137, -10680666, 16562505,
1599181, 11395898, 12942489, 1424408, 16703205, -9149814, 10681177, -15866200,
-1596197, -15892494, -12938694, -9217891, -16703318, 1343321, -10685220, 11338138,
1590947, 16552368, 12935335, 14787494, 16703816, 6787654, 10689282, -4074117,
-1585701, -13218088, -12931979, -16788814, -16704316, -13280743, -10693346, -4173009,
1580451, 6694220, 12928618, 14738915, 16704811, 16569126, 10697406, 11413168,
-1575205, 1444987, -12925259, -9132456, -16705308, -15859310, -10701469, -15899282,
1569955, -9235516, 12921895, 1322290, 16705799, 11322570, 10705526, 16548826,
};

int expected_wave_stereo_176_176[] = {
0, 0, 0, 0, -1, -1, 0, 0,
1, 1, -14, -12, -667, -566, -3217, -2755,
-3711, -3316, -4612, -4343, -5465, -5452, -1683, -2745,
-6270, -7059, 4923, 1972, -6179, -7492, 10918, 6982,
-3466, -4601, 9268, 6876, 5902, 5144, -7013, -4486,
26868, 25169, -41349, -31059, 62687, 56891, -91948, -72704,
114132, 99765, -156050, -127934, 186621, 158070, -275388, -231709,
517502, 432823, 7015813, 5958157, 12389823, 10789692, 15660392, 14266946,
16845823, 16451024, 15163835, 16594781, 11461971, 15184896, 5774700, 11925805,
-596014, 7541394, -7019586, 2219513, -12279899, -3260589, -15730717, -8436085,
-16765112, -12677124, -15255954, -15550456, -11437111, -16749101, -5865387, -16123418,
582016, -13766937, 6954414, -9909849, 12260184, -4989071, 15708704, 476424,
16767183, 5888023, 15278195, 10663253, 11467956, 14285511, 5912985, 16360825,
-540964, 16666031, -6912706, 15168428, -12233920, 12030023, -15695939, 7590303,
-16772638, 2329521, -15300388, -3183250, -11502931, -8351682, -5957368, -12616693,
493540, -15516927, 6869444, -16738661, 12201394, -16149732, 15679093, -13813850,
16774031, -9983694, 15319809, -5073577, 11537428, 385361, 6001697, 5802613,
-446114, 10592182, -6826130, 14235970, -12168775, 16339821, -15662125, 16676156,
-16775294, 15208590, -15339111, 12095877, -11571836, 7674725, -6045983, 2423379,
398681, -3090108, 6782758, -8269333, 12136055, -12554044, 15645029, -15480755,
16776420, -16732878, 15358287, -16174965, 11606148, -13867369, 6090216, -10059709,
-351249, -5163866, -6739335, 290565, -12103241, 5713564, -15627811, 10518513,
-16777415, 14185650, -15377344, 16318292, -11640371, 16685748, -6134405, 15248265,
303810, 12161344, 6695855, 7758901, 12070327, 2517159, 15610464, -2996868,
16778272, -8186719, 15396274, -12490993, 11674497, -15444087, 6178540, -16726560,
-256373, -16199680, -6652325, -13920444, -12037320, -10135402, -15592997, -5253989,
-16778999, 195759, -15415085, 5624332, -11708534, 10444508, -6222630, 14134876,
208929, 16296242, 6608738, 16694807, 12004213, 15287453, 15575401, 12226422,
16779587, 7842829, 15433769, 2610859, 11742473, -2903532, 6266667, -8103844,
-161488, -12427543, -6565102, -15406926, -11971014, -16719707, -15557684, -16223877,
-16780046, -13973073, -15452333, -10210771, -11776322, -5343945, -6310657, 100948,
114042, 5534921, 6521410, 10370168, 11937715, 14083650, 15539839, 16273671,
16780366, 16703332, 15470770, 15326152, 11810073, 12291108, 6354592, 7926507,
-66599, 2704476, -6477669, -2810104, -11904325, -8020709, -15521874, -12363695,
-16780557, -15369272, -15489087, -16712320, -11843733, -16247555, -6398480, -14025255,
19153, -10285812, 6433875, -5433728, 11870837, 6135, 15503781, 5445333,
16780609, 10295497, 15507278, 14031973, 11877298, 16250579, 6442318, 16711324,
28296, 15364362, -6390027, 12355404, -11837254, 8009932, -15485567, 2798007,
-16780531, -2716584, -15525346, -7937317, -11910767, -12299451, -6486103, -15331126,
-75742, -16704398, 6346129, -16270714, 11803576, -14076991, 15467226, -10360528,
16780315, -5523342, 15543288, -88683, 11944139, 5355569, 6529833, 10220497,
123185, 13979848, -6302184, 16226968, -11769807, 16718779, -15448766, 15402079,
-16779969, 12419302, -15561108, 8093100, -11977419, 2891447, -6573515, -2622979,
-170631, -7853672, 6258184, -12234815, 11735939, -15292490, 15430178, -16695942,
16779484, -16293353, 15578801, -14128275, 12010600, -10434910, 6617141, -5612776,
218072, -183494, -6214138, 5265636, -11701982, 10145170, -15411470, 13927276,
-16778870, 16202838, -15596373, 16725701, -12043688, 15439305, -6660718, 12482804,
-265515, 8176009, 6170038, 2984795, 11667928, -2529289, 15392635, -7769776,
16778117, -12169788, 15613816, -15253365, 12076677, -16686952, 6704238, -16315471,
312952, -14179108, -6125893, -10508959, -11633784, -5702031, -15373682, -278301,
-16777234, 5175534, -15631139, 10069518, -12109572, 13874259, -6747708, 16178190,
-360390, 16732089, 6081696, 15476036, 11599543, 12545906, 15354601, 8258657,
16776213, 3078048, 15648333, -2435519, 12142367, -7685632, 6791120, -12104371,
407822, -15213753, -6037453, -16677429, -11565213, -16337067, -15335402, -14229487,
-16775062, -10582671, -15665405, -5791103, -12175069, -373098, -6834481, 5085266,
-455254, 9993545, 5993158, 13820798, 11530787, 16153024, 15316076, 16737941,
16773773, 15512273, 15682348, 12608608, 12207670, 8341040, 6877785, 3171202,
502679, -2341671, -5948819, -7601241, -11496273, -12038568, -15296631, -15173654,
-16772353, -16667372, -15699170, -16358140, -12240177, -14279411, -6921037, -10656046,
-550104, -5879991, 5904429, -467884, 11461662, 4994836, 15277060, 9917252,
16770796, 13766895, 15715863, 16127342, 12272582, 16743257, 6964230, 15548013,
597520, 12670906, -5859995, 8423157, -11426964, 3264255, -15257371, -2247748,
-16769108, -7516608, -15732433, -11972379, -12304893, -15133069, -7007371, -16656783,
-644935, -16378691, 5815511, -14328878, 11392171, -10729079, 15237556, -5968690,
16767283, -562654, 15748874, 4904246, 12337101, 9840642, 7050452, 13712552,
692342, 16101144, -5770984, 16748039, -11357291, 15583257, -15217624, 12732799,
-16765327, 8505005, -15765193, 3357204, -12369215, -2153753, -7093481, -7431734,
-739747, -11905807, 5726407, -15092001, 11322316, -16645660, 15197565, -16398717,
16763234, -14377888, 15781382, -10801770, 12401226, -6057199, 7136449, -657407,
787142, 4813499, -5681788, 9763717, -11287254, 13657770, -15177390, 16074431,
-16761010, 16752284, -15797449, 15618002, -12433142, 12794285, -7179364, 8586581,
-834534, 3450045, 5637120, -2059689, 11252098, -7346622, 15157088, -11838855,
16758649, -15050450, 15813386, -16634006, 12464955, -16418219, 7222218, -14426437,
881916, -10874115, -5592411, -6145514, -11216856, -752138, -15136670, 4722598,
-16756157, 9686480, -15829200, 13602551, -12496672, 16047204, -7265018, 16755994,
-929295, 15652248, 5547653, 12855362, 11181521, 8667882, 15116127, 3542776,
16753528, -1965559, 15844884, -7261276, 12528285, -11771524, 7307755, -15008418,
976663, -16621819, -5502854, -16437197, -11146100, -14474526, -15095467, -10946112,
-16750768, -6233632, -15860445, -846846, -12559802, 4631546, -7350439, 9608933,
-1024026, 13546898, 5458008, 16019464, 11110586, 16759169, 15074682, 15685993,
16747871, 12916027, 15875875, 8748906, 12591214, 3635394, 7393060, -1871366,
1071378, -7175697, -5413122, -11703817, -11074987, -14965906, -15053780, -16609101,
-16744843, -16455648, -15891182, -14522151, -12622530, -11017760, -7435625, -6321552,
-1118725, -941526, 5368189, 4540346, 11039296, 9531079, 15032755, 13490811,
16741678, 15991212, 15906358, 16761807, 12653741, 15719236, 7478127, 12976280,
1166059, 8829650, -5323217, 3727895, -11003520, -1777114, -15011613, -7089889,
-16738383, -11635735, -15921411, -14922916, -12684854, -16595852, -7520574, -16473574,
-1213387, -14569312, 5278198, -11089055, 10967653, -6409268, 14990347, -1036176,
16734951, 4449001, 15936333, 9452920, 12715863, 13434293, 7562956, 15962448,
1260702, 16763909, -5233141, 15751977, -10931701, 13036118, -14968965, 8910112,
-16731388, 3820277, -15951131, -1682805, -12746773, -7003855, -7605281, -11567281,
-1308011, -14879448, 5188038, -16582072, 10895659, -16490972, 14947460, -14616007,
16727688, -11159995, 15965798, -6496780, 12777578, -1130793, 7647542, 4357514,
1355305, 9374459, -5142898, 13377345, -10859533, 15933174, -14925839, 16765476,
-16723858, 15784214, -15980341, 13095539, -12808285, 8990289, -7689746, 3912537,
-1402593, -1588442, 5097713, -6917596, 10823316, -11498458, 14904094, -14835504,
16719890, -16567762, 15994753, -16507844, 12838885, -14662235, 7731884, -11230578,
1449865, -6584085, -5052491, -1225374, -10787017, 4265887, -14882235, 9295698,
-16715793, 13319970, -16009040, 15903391, -12869386, 16766506, -7773965, 15815947,
-1497130, 13154541, 5007224, 9070178, 10750628, 4004672, 14860253, -1494028,
16711558, -6831116, 16023196, -11429267, 12899781, -14791086, 7815979, -16552922,
1544379, -16524187, -4961922, -14707993, -10714157, -11300803, -14838156, -6671178,
-16707193, -1319916, -16037227, 4174124, -12930077, 9216640, -7857935, 13262169,
-1591619, 15873099, 4916576, 16767000, 10677596, 15847174, 14815936, 13213122,
16702691, 9149777, 16051127, 4096679, 12960265, -1399566, 7899824, -6744418,
1638843, -11359710, -4871194, -14746195, -10640954, -16537552, -14793602, -16540002,
-16698059, -14753282, -16064902, -11370666, -12990353, -6758059, -7941654, -1414416,
-1686058, 4082228, 4825770, 9137287, 10604222, 13203943, 14771145, 15842299,
16693290, 16766958, 16078544, 15877894, 13020334, 13271281, 7983416, 9229084,
1733255, 4188554, -4780311, -1305060, -10567410, -6657504, -14748574, -11289790,
-16688391, -14700833, -16092062, -16521654, -13050215, -16555288, -8025119, -14798099,
-1780442, -11440165, 4734810, -6844723, 10530510, -1508870, 14725882, 3990200,
16683355, 9057642, 16105448, 13145295, 13079987, 15810993, 8066753, 16766379,
1827611, 15908106, -4689274, 13329016, -10493529, 9308096, -14703076, 4280296,
-16678189, -1210512, -16118708, -6570377, -13109659, -11219509, -8108327, -14655000,
-1874769, -16505228, 4643698, -16570045, 10456460, -14842442, 14680148, -11509299,
16672887, -6931168, 16131836, -1603276, 13139222, 3898046, 8149833, 8977707,
1921909, 13086227, -4598088, 15779181, -10419312, 16765265, -14657107, 15937809,
-16667455, 13386324, -16144839, 9386810, -13168683, 4371901, -8191277, -1115925,
-1969037, -6483040, 4552438, -11148869, 10382076, -14608699, 14633945, -16488274,
16661885, -16584272, 16157709, -14886311, 13198036, -11578064, 8232651, -7017392,
2016145, -1697631, -4506755, 3805766, -10344762, 8897485, -14610669, 13026741,
-16656187, 15746865, -16170453, 16763614, -13227287, 15967003, -8273964, 13443204,
-2063242, 9465224, 4461032, 4463366, 10307361, -1021303, 14587273, -6395495,
16650351, -11077873, 16183064, -14561931, 13256429, -16470792, 8315206, -16597968,
2110317, -14929704, -4415278, -11646460, -10269881, -7103391, -14563765, -1791931,
-16644386, 3713365, -16195550, 8816979, -13285468, 12966838, -8356386, 15714045,
-2157380, 16761428, 4369484, 15995686, 10232315, 13499654, 14540136, 9543335,
16638284, 4554689, 16207903, -926647, 13314397, -6307747, 8397496, -11006523,
2204422, -14514697, -4323659, -16452784, -10194672, -16611134, -14516394, -14972619,
-16632054, -11714482, -16220130, -7189163, -13343224, -1886175, -8438542, 3620846,
-2251450, 8736191, 4277796, 12906520, 10156942, 15680722, 14492533, 16758705,
16625686, 16023858, 16232223, 13555673, 13371940, 9621141, 8479516, 4645866,
2298456, -831963, -4231902, -6219796, -10119136, -10934820, -14468560, -14466998,
-16619189, -16434250, -16244190, -16623769, -13400553, -15015056, -8520427, -11782131,
-2345447, -7274705, 4185971, -1980358, 10081245, 3528210, 14444467, 8655123,
16612555, 12845790, 16256024, 15646899, 13429055, 16755446, 8561266, 16051518,
2392416, 13611258, -4140010, 9698639, -10043277, 4736894, -14420262, -737251,
-16605793, -6131646, -16267731, -10862768, -13457454, -14418838, -8602040, -16415190,
-2439370, -16635872, 4094012, -15057013, 10005225, -11849402, 14395938, -7360015,
16598894, -2074477, 16279305, 3435462, 13485741, 8573779, 8642742, 12784649,
2486300, 15612574, -4047985, 16751652, -9967097, 16078664, -14371504, 13666408,
-16591866, 9775827, -16290752, 4827771, -13513924, -642516, -8683378, -6043301,
-2533215, -10790368, 4001922, -14370216, 9928885, -16395605, 14346950, -16647443,
16584701, -15098488, 16302066, -11916295, 13541996, -7445090, 8723941, -2168531,
2580105, 3342603, -3955831, 8492160, -9890598, 12723099, -14322286, 15577751,
-16577408, 16747322, -16313252, 16105296, -13569963, 13721121, -8764439, 9852703,
-2626978, 4918493, 3909705, -547761, 9852228, -5954762, 14297503, -10717624,
16569979, -14321134, 16324305, -16375496, 13597817, -16658481, 8804862, -15139480,
2673827, -11982807, -3863551, -7529926, -9813783, -2262514, -14272610, 3249638,
-16562421, 8410270, -16335231, 12661142, -13625567, 15542429, -8845219, 16742457,
-2720658, 16131413, 3817362, 13775396, 9775256, 9929263, 14247598, 5009058,
16554726, -452987, 16346022, -5866033, 13653204, -10644537, 8885501, -14271595,
2767463, -16354864, -3771147, -16668987, -9736654, -15179988, -14222477, -12048935,
-16546903, -7614521, -16356687, -2356426, -13680736, 3156569, -8925716, 8328111,
-2814251, 12598780, 3724897, 15506611, 9697971, 16737056, 14197238, 16157014,
16538944, 13829229, 16367217, 10005507, 13708154, 5099463, 8965856, -358200,
2861011, -5777116, -3678622, -10571109, -9659214, -14221599, -14171890, -16333708,
-16530857, -16678960, -16377620, -15220011, -13735467, -12114678, -9005928, -7698873,
-2907753, -2450262, 3632314, 3063399, 9620376, 8245685, 14146424, 12536015,
16522633, 15470296, 16387888, 16731120, 13762666, 16182098, 9045924, 13882621,
2954468, 10081429, -3585980, 5189705, -9581465, -263401, -14120850, -5688015,
-16514282, -10497343, -16398029, -14171149, -13789759, -16312030, -9085852, -16688400,
-3001163, -15259547, 3539614, -12180034, 9542473, -7782979, 14095158, -2544020,
16505794, 2970131, 16408035, 8162996, 13816737, 12472850, 9125703, 15433487,
3047830, 16724648, -3493223, 16206665, -9503410, 13935568, -14069358, 10157030,
-16497179, 5279781, -16417914, -168594, -13843609, -5598731, -9165485, -10423242,
-3094476, -14120245, 3446801, -16289830, 9464266, -16697306, 14043441, -15298595,
16488427, -12245000, 16427657, -7866835, 13870367, -2637696, 9205189, 2876768,
3141094, 8080046, -3400355, 12409286, -9425050, 15396184, -14017416, 16717642,
-16479548, 16230714, -16437274, 13988070, -13897017, 10232306, -9244825, 5369688,
-3187691, -73781, 3353878, -5509269, 9385756, -10348807, 13991275, -14068890,
16470533, -16267110, 16446754, -16705678, 13923553, -15337153, 9284382, -12309574,
3234258, -7950440, -3307378, -2731287, -9346391, 2783315, -13965027, 7996840,
-16461391, 12345326, -16456108, 15358391, -13949981, 16710103, -9323870, 16254244,
-3280806, 14040126, 3260844, 10307257, 9306943, 5459426, 13938660, 21037,
16452112, -5419629, 16465326, -10274041, 13976295, -14017084, 9363279, -16243869,
3327320, -16713516, -3214293, -15375222, -9267429, -12373756, -13912189, -8033793,
-16442706, -2824794, -16474416, 2689769, -14002500, 7913373, -9402616, 12280968,
-3373812, 15320103, 3167711, 16702027, 9227836, 16277253, 13885602, 14091731,
16433165, 10381874, 16483371, 5548985, 14028589, 115849, 9441875, -5329818,
3420274, -10198947, -3121108, -13964831, -9188173, -16220109, -13858907, -16720819,
-16423496, -15412799, -16492197, -12437541, -14054570, -8116886, -9481062, -2918208,
-3466712, 2596139, 3074475, 7829656, 9148433, 12216220, 13832098, 15281327,
16413693, 16693418, 16500888, 16299743, 14080434, 14142886, 9520169, 10456161,
3513119, 5638368, -3027822, 210660, -9108624, -5239836, -13805183, -10123527,
-16403761, -13912132, -16509451, -16195830, -14106190, -16727588, -9559204, -15449883,
-3559501, -12500929, 2981141, -8199721, 9068738, -3011529, 13778153, 2502426,
16393695, 7745689, 16517878, 12151080, 14131829, 15242062, 9598159, 16684275,
3605851, 16321711, -2934440, 14193588, -9028783, 10530114, -13751017, 5727571,
-16383502, 305464, -16526177, -5149686, -14157360, -10047783, -9637041, -13858987,
-3652176, -16171034, 2887711, -16733822, 8988752, -15486472, 13723767, -12563916,
16373174, -8282293, 16534340, -3104754, 14182773, 2408633, 9675842, 7661473,
3698468, 12085553, -2840963, 15202310, -8948653, 16674599, -13696411, 16343157,
-16362718, 14243837, -16542375, 10603730, -14208076, 5816591, -9714570, 400258,
-3744735, -5059371, 2794189, -9971718, 8908479, -13805399, 13668942, -16145720,
16352128, -16739520, 16550273, -15522567, 14233262, -12626502, 9753216, -8364601,
3790967, -3197880, -2747396, 2314763, -8868237, 7577013, -13641367, 12019638,
-16341412, 15162072, -16558043, 16664389, -14258339, 16364081, -9791788, 14293631,
-3837174, 10677007, 2700577, 5905424, 8827921, 495039, 13613679, -4968895,
16330560, -9895334, 16565677, -13751370, 14283297, -16119890, 9830278, -16744684,
3883345, -15558165, -2653741, -12688685, -8787537, -8446641, -13585887, -3290903,
-16319582, 2220819, -16573182, 7492310, -14308145, 11953340, -9868693, 15121349,
-3929490, 16653647, 2606880, 16384482, 8747080, 14342967, 13557982, 10749942,
16308470, 5994070, 16580551, 589805, 14332875, -4878260, 9907025, -9818633,
3975599, -13696901, -2560001, -16093544, -8706557, -16749312, -13529972, -15593266,
-16297231, -12750461, -16587791, -8528411, -14357494, -3383821, -9945282, 2126804,
-4021680, 7407368, 2513098, 11886659, 8665960, 15080142, 13501851, 16642372,
16285858, 16404358, 16594895, 14391845, 14381995, 10822534, 9983456, 6082523,
4067725, 684552, -2466179, -4787468, -8625298, -9741618, -13473625, -13641993,
-16274359, -16066684, -16601870, -16753404, -14406384, -15627868, -10021554, -12811830,
-4113742, -8609908, 2419237, -3476631, 8584563, 2032721, 13445288, 7322189,
16262726, 11819598, 16608708, 15038453, 14430655, 16630565, 10059567, 16423710,
4159722, 14440262, -2372279, 10894779, -8543763, 6170782, -13416847, 779277,
-16250966, -4696524, -16615417, -9664292, -14454813, -13586650, -10097505, -16039310,
-4205672, -16756960, 2325298, -15661971, 8502891, -12872790, 13388295, -8691130,
16239073, -3569330, 16621990, 1938573, 14478853, 7236776, 10135357, 11752159,
4251585, 14996283, -2278302, 16618226, -8461955, 16442537, -13359640, 14488218,
-16227054, 10966677, -16628434, 6258844, -14502780, 873977, -10173133, -4605429,
-4297468, -9586657, 2231284, -13530872, 8420947, -16011423, 13330874, -16759981,
16214901, -15695572, 16634741, -12933337, 14526588, -8772074, 10210823, -3661914,
4343313, 1844363, -2184253, 7151131, -8379876, 11684344, -13302006, 14953634,
-16202623, 16605356, -16640918, 16460837, -14550284, 14535710, -10248435, 11038223,
-4389127, 6346705, 2137200, 968649, 8338734, -4514188, 13273027, -9508715,
16190211, -13474662, 16646959, -15983024, 14573859, -16762466, 10285962, -15728672,
4434901, -12993471, -2090133, -8852738, -8297530, -3754382, -13243946, 1750094,
-16177673, 7065258, -16652871, 11616155, -14597322, 14910507, -10323410, 16591954,
-4480645, 16478612, 2043047, 14582738, 8256255, 11109417, 13214756, 6434363,
16165003, 1063290, 16658646, -4422801, 14620664, -9430469, 10360772, -13418021,
4526348, -15954114, -1995947, -16764415, -8214918, -15761268, -13185463, -13053189,
-16152207, -8933118, -16664291, -3846729, -14643893, 1655769, -10398055, 6979159,
-4572019, 11547596, 1948828, 14866902, 8173511, 16578022, 13156061, 16495860,
16139278, 14629299, 16669799, 11180255, 14667001, 6521816, 10435251, 1157897,
4617650, -4331274, -1901698, -9351922, -8132043, -13360950, -13126558, -15924694,
-16126224, -16765828, -16675178, -15793361, -14689995, -13112490, -10472367, -9013213,
-4663248, -3938954, 1854548, 1561391, 8090507, 6892836, 13096946, 11478667,
16113036, 14822823, 16680420, 16563560, 14712869, 16512580, 10509396, 14675393,
4708804, 11250736, -1807387, 6609060, -8048909, 1252467, -13067234, -4239608,
-16099725, -9273076, -16685532, -13303452, -14735629, -15894765, -10546345, -16766704,
-4754327, -15824948, 1760208, -13171372, 8007243, -9093019, 13037413, -4031052,
16086280, 1466963, 16690507, 6806294, 14758266, 11409370, 10583205, 14778269,
4799808, 16548569, -1713018, 16528772, -7965517, 14721017, -13007492, 11320857,
-16072711, 6696093, -16695352, 1346997, -14780790, -4147806, -10619985, -9193933,
-4845254, -13245529, 1665811, -15864327, 7923723, -16767044, 12977462, -15856030,
16059009, -13229833, 16700060, -9172535, 14803192, -4123022, 10656676, 1372488,
4890658, 6719533, -1618595, 11339709, -7881870, 14733243, -12947333, 16533048,
-16045183, 16544435, -16704639, 14766170, -14825480, 11390616, -10693285, 6782912,
-4936027, 1441484, 1571362, -4055872, 7839950, -9114496, 12917097, -13187183,
16031225, -15833382, 16709080, -16766849, 14847645, -15886605, 10729806, -13287870,
4981352, -9251757, -1524120, -4214859, -7797972, 1277970, -12886761, 6632558,
-16017143, 11269686, -16713391, 14687746, -14869695, 16516998, -10766244, 16559570,
-5026641, 14810851, 1476862, 11460011, 7755926, 6869513, 12856318, 1535925,
16002928, -3963808, 16717565, -9034768, 14891622, -13128415, 10802592, -15801931,
5071887, -16766117, -1429597, -15916671, -7713823, -13345483, -12825777, -9330684,
-15988590, -4306562, -16721609, 1183410, -14913435, 6545370, -10838858, 11199302,
-5117095, 14641779, 1382315, 16500421, 7671655, 16574175, 12795128, 14855059,
15974119, 11529039, 16725515, 6955896, 14935124, 1630316, 10875034, -3871617,
5162259, -8954750, -1335027, -13069226, -7629429, -15769975, -12764382, -16764849,
-15959525, -15946229, -16729292, -13402669, -14956697, -9409312, -10911126, -4398127,
-5207385, 1088813, 1287724, 6457974, 7587138, 11128560, 12733530, 14595343,
15944800, 16483315, 16732931, 16588250, 14978147, 14898792, 10947127, 11597699,
5252466, 7042055, -1240415, 1724656, -7544790, -3779303, -12702579, -8874447,
-15929951, -13009620, -16736440, -15737514, -14999482, -16763045, -10983045, -15975276,
-5297509, -13459426, 1193092, -9487639, 7502378, -4489552, 12671523, 994181,
15914971, 6370370, 16739812, 11057462, 15020692, 14548441, 11018871, 16465683,
5342506, 16601794, -1145763, 14942048, -7459910, 11665988, -12640370, 7127990,
-15899867, 1818940, -16743053, -3686867, -15041787, -8793859, -11054613, -12949598,
-5387463, -15704550, 1098421, -16760704, 7417378, -16003813, 12609112, -13515753,
15884632, -9565663, 16746157, -4580833, 15062757, 899517, 11090262, 6282563,
5432374, 10986010, -1051075, 14501074, -7374791, 16447524, -12577757, 16614808,
-15869275, 14984826, -16749131, 11733904, -15083611, 7213696, -11125827, 1913166,
-5477245, -3594314, 1003716, -8712991, 7332141, -12889162, 12546298, -15671083,
15853786, -16757828, 16751967, -16031838, 15104340, -13571647, 11161299, -9643381,
5522069, -4671968, -956353, 804824, -7289437, 6194556, -12514742, 10914207,
-15838175, 14453243, -16754673, 16428839, -15124952, 16627290, -11196685, 15027126,
-5566852, 11801444, 908978, 7299172, 7246670, 2007332, 12483082, -3501646,
15822433, -8631844, 16757241, -12828314, 15145440, -15637116, 11231978, -16754416,
5611587, -16059350, -861600, -13627108, -7203849, -9720791, -12451326, -4762953,
-15806569, 710105, -16759679, 6106350, -15165810, 10842055, -11267186, 14404950,
-5656281, 16409629, 814211, 16639241, 7160967, 15068944, 12419467, 11868607,
15790574, 7384415, 16761979, 2101432, 15186055, -3408866, 11302299, -8550420,
5700926, -12767055, -766820, -15602649, -7118032, -16750469, -12387513, -16086349,
-15774457, -13682133, -16764149, -9797890, -15206183, -4853786, -11337326, 615364,
-5745529, 6017948, 719418, 10769557, 7075035, 14356196, 12355455, 16389894,
15758210, 16650659, 16766181, 15110281, 15226185, 11935391, 11372258, 7469421,
5790083, 2195466, -672015, -3315976, -7031986, -8468724, -12323303, -12705388,
-15741841, -15567682, -16768083, -16745985, -15246070, -16112834, -11407103, -13736721,
-5834594, -9874676, 624602, -4944463, 6988877, 520603, 12291048, 5929355,
15725342, 10696714, 16769847, 14306984, 15265828, 16369635, 11441854, 16661545,
5879054, 15151135, -577189, 12001793, -6945716, 7554189, -12258699, 2289430,
-15708721, -3222981, -16771481, -8386756, -15285469, -12643315, -11476516, -15532218,
-5923471, -16740966, 529766, -16138803, 6902495, -13790869, 12226248, -9951145,
15691971, -5034983, 16772977, 425826, 15304983, 5840571, 11511083, 10623529,
5967837, 14257313, -482344, 16348852, -6859223, 16671898, -12193703, 15191504,
-15675099, 12067811, -16774343, 7638715, -15324379, 2383320, -11545562, -3129883,
-6012160, -8304520, 434914, -12580838, 6815893, -15496257, 12161057, -16735412,
15658098, -16164256, 16775571, -13844576, 15343649, -10027297, 11579945, -5125341,
6056430, 331034, -387484, 5751601, -6772511, 10550004, -12128317, 14207187,
-15640976, 16327547, -16776668, 16681719, -15362799, 15231388, -11614239, 12133443,
-6100656, 7722996, 340047, 2477134, 6729072, -3036684, 12095477, -8222019,
15623725, -12517958, 16777628, -15459801, 15381823, -16729322, 11648436, -16189192,
6144829, -13897840, -292611, -10103128, -6685583, -5215536, -12062544, 236233,
-15606353, 5662447, -16778457, 10476142, -15400728, 14156607, -11682544, 16305719,
-6188956, 16691005, 245169, 15270784, 6642037, 12198688, 12029510, 7807031,
15588852, 2570869, 16779148, -2943389, 15419506, -8139255, 11716554, -12454678,
6233031, -15422850, -197730, -16722698, -6598441, -16213611, -11996384, -13950660,
-15571230, -10178635, -16779709, -5305563, -15438165, 141424, -11750475, 5573113,
-6277059, 10401945, 150284, 14105573, 6554789, 16283370, 11963159, 16699758,
15553481, 15309691, 16780132, 12263542, 15456696, 7890816, 11784298, 2664521,
6321033, -2850000, -102842, -8056231, -6511088, -12391000, -11929842, -15385406,
-15535610, -16715539, -16780425, -16237510, -15475107, -14003034, -11818031, -10253817,
-6364961, -5395421, 55394, 46610, 6467332, 5483600, 11896425, 10327415,
15517612, 14054090, 16780580, 16260501, 15493391, 16707977, 11851664, 15348110,
6408833, 12328003, -7952, 7974348, -6423527, 2758089, -11862916, -2756518,
-15499492, -7972947, -16780604, -12326924, -15511557, -15347470, -11885210, -16707845,
-6452662, -16260892, -39499, -14054961, 6379666, -10328673, 11829310, -5485108,
15481246, -48206, 16780490, 5393910, 15529592, 10252554, 11918654, 14002155,
6496431, 16237111, 86942, 16715662, -6335759, 15386038, -11795613, 12392072,
-15462880, 8057627, -16780247, 2851569, -15547508, -2662949, -11952007, -7889411,
-6540153, -12262456, -134389, -15309043, 6291798, -16699617, 11761817, -16283752,
15444386, -14106437, 16779865, -10403197, 15565296, -5574618, 11985261, -143020,
6583819, 5304049, 181831, 10177367, -6247790, 13949774, -11727932, 16213202,
-15425773, 16722812, -16779353, 15423473, -15582963, 12455743, -12018423, 8140647,
-6627436, 2944956, -229275, -2569295, 6203729, -7805622, 11693949, -12197596,
15407033, -15270127, 16778702, -16690855, 15600501, -16306093, 12051484, -14157463,
6670997, -10477388, 276714, -5663950, -6159621, -237829, -11659876, 5214019,
-15388173, 10101854, -16777922, 13896946, -15617919, 16188775, -12084454, 16729427,
-6714508, 15460416, -324154, 12519017, 6115461, 8223406, 11625706, 3038250,
15369186, -2475559, 16777003, -7721583, 15635208, -12132345, 12117323, -15230723,
6757961, -16681560, 371588, -16327911, -6071255, -14208036, -11591447, -10551245,
-15350080, -5753101, -16775955, -332630, -15652376, 5123821, -12150099, 10026017,
-6801364, 13843675, -419023, 16163830, 6026997, 16735508, 11557091, 15496864,
15330848, 12581890, 16774768, 8305903, 15669415, 3131447, 12182774, -2381744,
6844709, -7637298, 466451, -12066706, -5982695, -15190831, -11522647, -16671731,
-15311497, -16349208, -16773451, -14258154, -15686333, -10624764, -12215355, -5842068,
-6888003, -427421, -513878, 5033460, 5938341, 9949860, 11488107, 13789960,
15292020, 16138369, 16771996, 16741053, 15703121, 15532816, 12247835, 12644360,
6931239, 8388134, 561298, 3224543, -5893943, -2287853, -11453479, -7552767,
-15272424, -12000682, -16770411, -15150454, -15719788, -16661369, -12280221, -16369982,
-6974422, -14307817, -608717, -10697943, 5849495, -5930848, 11418756, -522199,
15252702, 4942938, 16768688, 9873385, 15736325, 13735804, 12312505, 16112391,
7017546, 16746063, 656128, 15568272, -5805003, 12706427, -11383945, 8470097,
-15232863, 3317537, -16766835, -2193888, -15752740, -7467996, -12344694, -11934273,
-7060618, -15109592, -703537, -16650473, 5760462, -16390232, 11349039, -14357022,
15212897, -10770780, 16764844, -6019438, 15769026, -616959, 12376781, 4852258,
7103630, 9796594, 750936, 13681210, -5715878, 16085898, -11314047, 16750537,
-15192814, 15603230, -16762722, 12768086, -15785189, 8551790, -12408772, 3410424,
-7146588, -2099853, -798334, -7382986, 5671244, -11867483, 11278960, -15068247,
15172606, -16639046, 16760463, -16409958, 15801222, -14405768, 12440661, -10843273,
7189486, -6107836, 845721, -711700, -5626569, 4761422, -11243787, 9719490,
-15152280, 13626177, -16758074, 16058890, -15817133, 16754476, -12472454, 15637689,
-7232330, 12829338, -893105, 8633208, 5581845, 3503203, 11208520, -2005751,
15131829, -7297739, 16755547, -11800314, 15832913, -15026420, 12504143, -16627086,
7275112, -16429160, 940478, -14454053, -5537081, -10915419, -11173167, -6196039,
-15111261, -806418, -16752890, 4670435, -15848571, 9642075, -12535737, 13570709,
-7317840, 16031369, -987848, 16757879, 5492268, 15671648, 11137721, 12890180,
15090568, 8714351, 16750095, 3595869, 15864098, -1911585, 12567226, -7212259,
7360506, -11732767, 1035206, -14984113, -5447415, -16614595, -11102190, -16447836,
-15069759, -14501876, -16747170, -10987216, -15879502, -6284043, -12598619, -901110,
-7403117, 4579297, -1082559, 9564352, 5402515, 13514807, 11066566, 16003335,
15048825, 16760746, 16744107, 15705106, 15894775, 12950609, 12629907, 8795215,
7445664, 3688421, 1129900, -1817357, -5357576, -7126549, -11030858, -11664845,
-15027775, -14941327, -16740914, -16601573, -15909925, -16465986, -12661098, -14549236,
-7488156, -11058662, -1177236, -6371847, 5312589, -995774, 10995058, 4488014,
15006601, 9486323, 16737583, 13458473, 15924944, 15974790, 12692183, 16763077,
7530584, 15738062, 1224558, 13010624, -5267565, 8875797, -10959173, 3780854,
-14985311, -1723072, -16734123, -7040610, -15939840, -11596550, -12723172, -14898062,
-7572956, -16588019, -1271875, -16483609, 5222494, -14596130, 10923198, -11129754,
14963897, -6459447, 16730525, -1090406, 15954604, 4396587, 12754054, 9407991,
7615264, 13401709, 1319177, 15945734, -5177385, 16764873, -10887138, 15770514,
-14942367, 13070223, -16726797, 8956096, -15969245, 3873166, -12784839, -1628732,
-7657514, -6954447, -1366473, -11527884, 5132231, -14854322, 10850988, -16573935,
14920714, -16500706, 16722931, -14642557, 15983754, -11200490, 12815517, -6546840,
7699699, -1185002, 1413754, 4305020, -5087041, 9329357, -10814755, 13344516,
-14898945, 15916168, -16718936, 16766132, -15998139, 15802462, -12846097, 13129404,
-7741827, 9036109, -1461028, 3965355, 5041805, -1534339, 10778432, -6868061,
14877054, -11458849, 16714803, -14810106, 16012393, -16559322, 12876571, -16517275,
7783889, -14688516, 1508286, -11270868, -4996533, -6634024, -10742026, -1279561,
-14855048, 4213314, -16710541, 9250426, -16026522, 13286896, -12906945, 15886093,
-7825893, 16766854, -1555535, 15833905, 4951217, 13188165, 10705531, 9115832,
14832918, 4057417, 16706140, -1439898, 16040519, -6781456, 12937212, -11389448,
7867830, -14765417, 1602769, -16544178, -4905866, -16533315, -10668953, -14734006,
-14810675, -11340886, -16701611, -6720995, -16054392, -1374079, -12967380, 4121474,
-7909708, 9171198, -1649994, 13228851, 4860471, 15855510, 10632287, 16767041,
14788308, 15864841, 16696943, 13246505, 16068133, 9195264, 12997440, 4149349,
7951519, -1345410, 1697201, -6694633, -4815041, -11319683, -10595540, -14720256,
-14765828, -16528506, -16692147, -16548827, -16081749, -14779024, -13027400, -11410540,
-7993270, -6807752, -1744399, -1468553, 4769569, 4029503, 10558704, 9091678,
14743226, 13170384, 16687212, 15824420, 16095233, 16766692, 13057252, 15895270,
8034954, 13304421, 1791579, 9274402, -4724063, 4241148, -10521787, -1250879,
-14720509, -6607597, -16682149, -11249556, -16108592, -14674624, -13087004, -16512305,
-8076577, -16563810, -1838748, -14823570, 4678515, -11479830, 10484783, -6894291,
14697671, -1562980, 16676948, 3937402, 16121818, 9011866, 13116647, 13111495,
8118132, 15792824, 1885899, 16765806, -4632934, 15925191, -10447698, 13361911,
-14674720, 9353243, -16671618, 4332812, -16134919, -1156309, -13146189, -6520350,
-8159625, -11179069, -1933039, -14628522, 4587311, -16495576, 10410526, -16578263,
14651647, -14867641, 16666150, -11548753, 16147888, -6980610, 13175622, -1657357,
8201050, 3845176, 1980160, 8931767, -4541656, 13052187, -10373275, 15760722,
-14628461, 16764384, -16660553, 15954603, -16160731, 13418974, -13204954, 9431786,
-8242413, 4424337, -2027268, -1061701, 4495961, -6432893, 10335937, -11108225,
14605154, -14581953, 16654819, -16478320, 16173441, -16592186, 13234176, -14911238,
8283706, -11617307, 2074357, -7066706, -4450234, -1751681, -10298520, 3752826,
-14581735, 8851382, -16648956, 12992461, -16186026, 15728117, -13263296, 16762427,
-8324937, 15983504, -2121432, 13475608, 4404467, 9510026, 10261017, 4515721,
14558195, -967059, 16642956, -6345231, 16198477, -11037025, 13292306, -14534918,
8366097, -16460536, 2168487, -16605578, -4358669, -14954357, -10223436, -11685489,
-14534542, -7152575, -16636827, -1845949, -16210803, 3660357, -13321215, 8770714,
-8407195, 12932321, -2215529, 15695009, 4312832, 16759933, 10185769, 16011894,
14510769, 13531812, 16630561, 9587963, 16222995, 4606960, 13350012, -872387,
8448221, -6257367, 2262549, -10965473, -4266964, -14487417, -10148025, -16442226,
-14486884, -16618440, -16624165, -14996998, -16235062, -11753298, -13378707, -7238216,
-8489183, -1940158, -2309554, 3567770, 4221059, 8689765, 10110195, 12871766,
14462880, 15661399, 16617633, 16756903, 16246994, 16039773, 13407291, 13587582,
8530074, 9665593, 2356537, 4698052, -4175124, -777687, -10072289, -6169302,
-14438764, -10893570, -16610972, -14439454, -16258801, -16423391, -13435772, -16630770,
-8570901, -15039160, -2403506, -11820730, 4129151, -7323625, 10034298, -2034305,
14414528, 3475070, 16604174, 8608538, 16270474, 12810800, 13464141, 15627288,
8611655, 16753337, 2450451, 16067138, -4083149, 13642918, -9996231, 9742914,
-14390181, 4788994, -16597248, -682961, -16282021, -6081039, -13492407, -10821318,
-8652344, -14391029, -2497381, -16404030, 4037111, -16642568, 9958081, -15080841,
14365715, -11887785, 16590185, -7408800, 16293434, -2128387, 13520560, 3382258,
8692960, 8527036, 2544286, 12749424, -3991044, 15592678, -9919854, 16749236,
-14341138, 16093989, -16582993, 13697818, -16304720, 9819923, -13548610, 4879783,
-8733511, -588214, -2591176, -5992583, 3944942, -10748721, 9881544, -14342143,
14316443, -16384145, 16575665, -16653834, 16315872, -15122039, 13576548, -11954459,
8773988, -7493738, 2638040, -2222401, -3898812, 3289338, -9843160, 8445262,
-14291637, 12687641, -16568208, 15557569, -16326898, 16744599, -13604380, 16120326,
-8814399, 13752280, -2684888, 9896618, 3852646, 4970415, 9804692, -493448,
14266713, -5903934, 16560614, -10675779, 16337789, -14292799, 13632101, -16363736,
8854735, -16664568, 2731710, -15162754, -3806454, -12020752, -9766150, -7578437,
-14241679, -2316344, -16552893, 3196313, -16348553, 8363217, -13659716, 12625452,
-8895004, 15521962, -2778514, 16739427, 3760228, 16146148, 9727526, 13806302,
14216527, 9972997, 16545035, 5060889, 16359183, -398666, 13687218, -5815097,
8935198, -10602497, 2825292, -14242998, -3713975, -16342803, -9688829, -16674768,
-14191265, -15202985, -16537048, -12086660, -16369686, -7662893, -13714614, -2410213,
-8975325, 3103185, -2872051, 8280905, 3667689, 12562859, 9650049, 15485859,
14165886, 16733719, 16528926, 16171453, 16380054, 13859882, 13741897, 10049057,
9015376, 5151201, 2918784, -303872, -3621378, -5726074, -9611197, -10528875,
-14140398, -14192742, -16520675, -16321348, -16390296, -16684435, -13769074, -15242728,
-9055359, -12152181, -2965497, -7747104, 3575033, -2504005, 9572264, 3009959,
14114792, 8198328, 16512288, 12499864, 16400402, 15449261, 13796137, 16727476,
9095266, 16196241, 3012182, 13913019, -3528664, 10124796, -9533259, 5241348,
-14089078, -209068, -16503773, -5636868, -16410381, -10454917, -13823093, -14142031,
-9135104, -16299371, -3058847, -16693569, 3482263, -15281985, 9494173, -12217313,
14063247, -7831068, 16495123, -2597716, 16420225, 2916636, 13849936, 8115489,
9174865, 12436470, 3105484, 15412169, -3435838, 16720699, -9455015, 16220511,
-14037308, 13965711, -16486344, 10200210, -16429941, 5331327, -13876671, -114257,
-9214556, -5547482, -3152100, -10380624, 3389382, -14090869, 9415778, -16276873,
14011252, -16702169, 16477430, -15320752, 16439523, -12282055, 13903291, -7914780,
9254170, -2691344, 3198687, 2823221, -3342903, 8032391, -9376469, 12372678,
-13985089, 15374584, -16468388, 16713386, -16448976, 16244262, -13929804, 14017957,
-9293713, 10275298, -3245251, 5421135, 3296394, -19445, 9337084, -5457921,
13958811, -10306001, 16459210, -14039256, 16458295, -16253855, 13956204, -16710236,
9333182, -15359032, 3291790, -12346407, -3249856, -7998243, -9297622, -2784889,
-13932422, 2729713, -16449904, 7949035, -16467486, 12308489, -13982494, 15336507,
-9372575, 16705539, -3338301, 16267494, 3203296, 14069755, 9258086, 10350060,
13905920, 5510773, 16440464, 75373, 16476541, -5368179, 14008668, -10231045,
9411891, -13987193, 3384783, -16230316, -3156711, -16717767, -9218480, -15396818,
-13879310, -12410360, -16430895, -8081445, -16485468, -2878341, -14034735, 2636120,
-9451136, 7865426, -3431243, 12243909, 3110098, 15297940, 9178795, 16697158,
13852586, 16290206, 16421192, 14121103, 16494260, 10424489, 14060685, 5600232,
9490301, 170186, 3477670, -5278270, -3063464, -10155764, -9139042, -13934684,
-13825755, -16206258, -16411361, -16724764, -16502924, -15434112, -14086527, -12473917,
-9529394, -8164391, -3524074, -2971702, 3016801, 2542442, 9099211, 7781565,
13798809, 12178936, 16401395, 15258884, 16511451, 16688244, 14112252, 16312397,
9568407, 14171999, 3570446, 10498584, -2970118, 5689512, -9059312, 264993,
-13771757, -5188191, -16391302, -10080158, -16519851, -13881729, -14137869, -16181683,
-9607348, -16731226, -3616793, -15470913, 2923407, -12537076, 9019336, -8247075,
13744591, -3064968, 16381074, 2448683, 16528115, 7697455, 14163369, 12113574,
9646208, 15219340, 3663108, 16678795, -2876677, 16334066, -8979292, 14222441,
-13717319, 10572344, -16370718, 5778611, -16536250, 359792, -14188759, -5097947,
-9684995, -10004229, -3709397, -13828330, 2829920, -16156590, 8939172, -16737153,
13689933, -15507219, 16360228, -12599834, 16544250, -8329496, 14214032, -3158137,
9723700, 2354846, 3755652, 7613099, -2783144, 12047824, -8898985, 15179309,
-13662442, 16668813, -16349611, 16355213, -16552121, 14272430, -14239195, 10645766,
-9762332, 5867524, -3801881, 454579, 2736342, -5007539, 8858723, -9927981,
13634838, -13774489, 16338860, -16130980, 16559856, -16742545, 14264241, -15543029,
9800881, -12662189, 3848077, -8411651, -2689522, -3251204, -8818393, 2260933,
-13607129, 7528499, -16327982, 11981689, -16567462, 15138793, -14289176, 16658298,
-9839357, 16375837, -3894245, 14321961, 2642677, 10718848, 8777990, 5956250,
13579307, 549352, 16316969, -4916971, 16574932, -9851415, 14313993, -13720207,
9877749, -16104854, 3940378, -16747401, -2595814, -15578342, -8737520, -12724139,
-13551380, -8493536, -16305830, -3344167, -16582274, 2166948, -14338700, 7443659,
-9916067, 11915171, -3986484, 15097792, 2548927, 16647251, 8696976, 16395937,
13523341, 14371035, 16294556, 10791587, 16589479, 6044786, 14363288, 644107,
9954301, -4826246, 4032553, -9774534, -2502023, -13665487, -8656367, -16078214,
-13495198, -16751722, -16283156, -15613158, -16596555, -12785682, -14387765, -8575150,
-9992460, -3437024, -4078595, 2072893, 2455096, 7358581, 8615685, 11848272,
13466943, 15056309, 16271622, 16635670, 16603495, 16415513, 14412124, 14419649,
10030535, 10863980, 4124600, 6133128, -2408153, 738842, -8574937, -4735367,
-13438584, -9697341, -16259962, -13610329, -16610306, -16051059, -14436371, -16755508,
-10068533, -15647473, -4170576, -12846816, 2361186, -8656490, 8534117, -3529770,
13410114, 1978773, 16248168, 7273267, 16616980, 11780995, 14460498, 15014345,
10106448, 16623559, 4216514, 16434564, -2314205, 14467802, -8493233, 10936027,
-13381541, 6221274, -16236248, 833553, -16623525, -4644336, -14484514, -9619837,
-10144285, -13554737, -4262423, -16023391, 2267201, -16758757, 8452277, -15681289,
13352856, -12907540, 16224195, -8737553, 16629933, -3622404, 14508411, 1884589,
10182037, 7187721, 4308294, 11713340, -2220183, 14971900, -8411258, 16610915,
-13324069, 16453089, -16212015, 14515493, -16636213, 11007723, -14532195, 6309222,
-10219712, 928237, -4354135, -4553157, 2173143, -9542026, 8370167, -13498711,
13295172, -15995210, 16199702, -16761471, 16642355, -15714603, 14555859, -12967851,
10257302, -8818337, 4399936, -3714922, -2126090, 1790344, -8329013, 7101945,
-13266172, 11645311, -16187264, 14928976, -16648369, 16597740, -14579411, 16471088,
-10294813, 14562719, -4445706, 11079068, 2079016, 6396967, 8287789, 1022892,
13237062, -4461832, 16174692, -9463910, 16654245, -13442253, 14602842, -15966518,
10332238, -16763648, 4491437, -15747414, -2031929, -13027747, -8246502, -8898838,
-13207850, -3807321, -16161995, 1696043, -16659992, 7015942, -14626160, 11576909,
-10369584, 14885575, -4537136, 16584035, 1984822, 16488561, 8205146, 14609479,
13178529, 11150058, 16149164, 6484508, 16665602, 1117514, 14649358, -4370364,
10406843, -9385491, 4582795, -13385366, -1937703, -15937316, -8163728, -16765290,
-13149106, -15779722, -16136209, -13087226, -16671083, -8979055, -14672442, -3899598,
-10444024, 1601687, -4628421, 6929714, 1890565, 11508137, 8122240, 14841698,
13119575, 16569799, 16123120, 16505506, 16676426, 14655773, 14695405, 11220692,
10481116, 6571841, 4674006, 1212100, -1843415, -4278757, -8080692, -9306772,
-13089942, -13328050, -16109907, -15907604, -16681640, -16766395, -14718255, -15811526,
-10518129, -13146287, -4719558, -9058985, 1796247, -3991751, 8039075, 1507280,
13060201, 6843265, 16096560, 11438998, 16686717, 14797347, 14740983, 16555034,
10555054, 16521924, 4765068, 14701597, -1749069, 11290967, -7997398, 6658965,
-13030359, 1306647, -16083089, -4187013, -16691665, -9227755, -14763597, -13270309,
-10591898, -15877383, -4810543, -16766965, 1701872, -15842823, 7955653, -13204928,
13000409, -9138625, 16069486, -4083776, 16696474, 1412825, 14786089, 6756598,
10628654, 11369492, 4855977, 14752522, -1654666, 16539739, -7913848, 16537813,
-12970359, 14746952, -16055758, 11360880, -16701155, 6745875, -14808467, 1401153,
-10665329, -4095135, -4901375, -9148444, 1607443, -13212143, 7871977, -15846655,
12940202, -16766998, 16041898, -15873614, 16705698, -13263146, 14830722, -9217973,
10701915, -4175670, 4946731, 1318325, -1560210, 6669714, -7830046, 11299623,
-12909945, 14707226, -16027913, 16523915, -16710111, 16553174, -14852863, 14791835,
-10738418, 11430431, -4992050, 6832570, 1512962, 1495614, 7788048, -4003126,
12879581, -9068839, 16013796, -13153554, 16714387, -15815420, 14874881, -16766495,
10774833, -15903898, 5037326, -13320940, -1465705, -9297026, -7745993, -4267431,
-12849118, 1223783, -15999556, 6582616, -16718533, 11229393, -14896785, 14661459,
-10811165, 16507562, -5082566, 16568005, 1418432, 14836245, 7703871, 11499616,
12818548, 6919046, 15985183, 1590027, 16722542, -3910989, 14918565, -8988945,
10847406, -13094545, 5127761, -15783679, -1371153, -16765456, -7661692, -15933673,
-12787880, -13378308, -15970687, -9375782, -16726421, -4359055, -14940230, 1129201,
-10883565, 6495309, -5172919, 11158803, 1323858, 14615224, 7619447, 16490682,
12757105, 16582306, 15956059, 14880180, 16730162, 11568433, 14961771, 7005301,
10919633, 1684389, 5218032, -3818727, -1276556, -8908763, -7577146, -13035117,
-12726233, -15751433, -15941307, -16763880, -16733773, -15962938, -14983197, -13435249,
-10955617, -9454238, -5263107, -4450540, 1229241, 1034583, 7534780, 6407793,
12695254, 11087857, 15926424, 14568521, 16737247, 16473275, 15004499, 16596077,
10991510, 14923640, 5308136, 11636881, -1181919, 7091332, -7492358, 1778697,
-12664178, -3726343, -15911417, -8828297, -16740591, -12975272, -15025685, -15718684,
-11027320, -16761769, -5353126, -15991693, 1134584, -13491760, 7449872, -9532392,
12632998, -4541883, 15896280, 939932, 16743797, 6320073, 15046747, 11016556,
11063036, 14521352, 5398070, 16455340, -1087245, 16609317, -7407330, 14966623,
-12601720, 11704956, -15881019, 7177136, -16746873, 1872949, -15067693, -3633839,
-11098669, -8747548, -5442974, -12915013, 1039892, -15685432, 7364725, -16759122,
12570337, -16019937, 15865628, -13547839, 16749811, -9610241, 15088514, -4633080,
11134209, 845252, 5487831, 6232151, -992535, 10944903, -7322065, 14473719,
-12538858, 16436880, -15850113, 16622026, -16752619, 15009127, -15109219, 11772657,
-11169664, 7262711, -5532648, 1967140, 945166, -3541220, 7279343, -8666519,
12507274, -12854340, 15834468, -15651678, 16755290, -16755938, 15129799, -16047668,
11205025, -13603485, 5577417, -9687783, -897794, -4724130, -7236566, 750544,
-12475595, 6144029, -15818700, 10872899, -16757830, 14425623, -15150262, 16417894,
-11240301, 16634204, -5622146, 15051151, 850410, 11839982, 7193728, 7348053,
12443811, 2061269, 15802802, -3448487, 16760233, -8585213, 15170599, -12793257,
11275483, -15617424, 5666825, -16752219, -803023, -16074886, -7150836, -13658696,
-12411933, -9765015, -15786781, -4815028, -16762505, 655812, -15190820, 6055711,
-11310579, 10800549, -5711463, 14377066, 755627, 16398383, 7107883, 16645850,
12379951, 15092694, 15770631, 11906928, 16764640, 7433161, 15210915, 2155332,
11345580, -3355644, 5756051, -8503633, -708228, -12731764, -7064877, -15582670,
-12347874, -16747965, -15754358, -16101591, -16766644, -13713471, -15230893, -9841934,
-11380495, -4905772, -5800598, 561059, 660819, 5967199, 7021811, 10727852,
12315694, 14328049, 15737955, 16378347, 16768510, 16656963, 15250744, 15133754,
11415315, 11973493, 5845094, 7518031, -613409, 2249325, -6978693, -3262694,
-12283420, -8421781, -15721430, -12669864, -16770247, -15547419, -15270478, -16743174,
-11450048, -16127780, -5889547, -13767807, 565991, -9918539, 6935514, -4996360,
12251044, 466288, 15704776, 5878496, 16771845, 10654813, 15290086, 14278573,
11484685, 16357788, 5933949, 16667544, -518571, 15174330, -6892285, 12039675,
-12218574, 7602660, -15688000, 2343247, -16773313, -3169639, -15309575, -8339659,
-11519234, -12607559, -5978308, -15511670, 471144, -16737849, 6848996, -16153453,
12186002, -13821702, 15671095, -9994827, 16774644, -5086787, 15328938, 371502,
11553687, 5789605, 6022615, 10581433, -423717, 14228642, -6805656, 16336706,
-12153337, 16677592, -15654068, 15214421, -16775844, 12105472, -15348183, 7687046,
-11588052, 2437094, -6066878, -3076483, 376282, -8257271, 6762258, -12544851,
12120570, -15475425, 15636913, -16731988, 16776905, -16178610, 15367301, -13875156,
11622320, -10070795, 6111088, -5177052, -328849, 276705, -6718810, 5700530,
-12087711, 10507715, -15619636, 14178255, -16777837, 16315101, -15386300, 16687106,
-11656499, 15254025, -6155254, 12170882, 281408, 7771187, 6675305, 2530863,
12054751, -2983229, 15602230, -8174619, 16778631, -12481741, 15405172, -15438685,
11690581, -16725591, 6199366, -16203250, -233970, -13928166, -6631750, -10146441,
-12021699, -5267151, -15584704, 181898, -16779294, 5611272, -15423924, 10433660,
-11724573, 14127415, -6243432, 16292975, 186526, 16696087, 6588138, 15293142,
11988547, 12235903, 15567049, 7855079, 16779820, 2624551, 15442550, -2889879,
11758468, -8091706, 6287445, -12418233, -139084, -15401451, -6544477, -16718661,
-11955302, -16227371, -15549274, -13980730, -16780215, -10221762, -15461056, -5357082,
-11792273, 87086, -6331411, 5521834, 91637, 10359272, 6500760, 14076123,
11921959, 16270327, 15531370, 16704534, 16780472, 15331769, 15479434, 12300533,
11825979, 7938719, 6375323, 2718155, -44194, -2796437, -6456996, -8008534,
-11888524, -12354328, -15513347, -15363726, -16780599, -16711196, -15497692, -16250974,
-11859594, -14032847, -6419188, -10296757, -3256, -5446843, 6413172, -7731,
11854987, 5432218, 15495194, 10284551, 16780588, 14024380, 15515825, 16247159,
11893113, 16712447, 6462999, 15369907, 50699, 12364770, -6369305, 8022107,
-11821363, 2811673, -15476923, -2702904, -16780447, -7925104, -15533835, -12290026,
-11926539, -15325508, -6506761, -16703195, -98147, -16274057, 6325383, -14084517,
11787640, -10371423, 15458523, -5536428, 16780167, -102544, 15551717, 5342432,
11959866, 10209504, 6550467, 13972190, 145589, 16223472, -6281413, 16719825,
-11753826, 15407552, -15440004, 12428611, -16779757, 8105237, -15569480, 2905100,
-11993101, -2609287, -6594124, -7841422, -193035, -12225333, 6237390, -15286800,
11719915, -16694661, 15421358, -16296619, 16779210, -14135735, 15587114, -10445757,
12026236, -5625836, 6637725, -197355, 240475, 5252473, -6193320, 10134130,
-11685914, 13919553, -15402592, 16199267, -16778532, 16726669, -15604627, 15444706,
-12059279, 12492054, -6681276, 8188108, -287917, 2998435, 6149197, -2515585,
11651815, -7757490, 15383699, -12160248, 16777716, -15247604, 15622011, -16685593,
12092222, -16318661, 6724771, -14186501, 335353, -10519756, -6105029, -5715065,
-11617627, -292161, -15364688, 5162347, -16776769, 10058431, -15639275, 13866471,
-12125072, 16174543, -6768215, 16732977, -382790, 15481365, 6060808, 12555098,
11583342, 8270718, 15345549, 3091673, 16775685, -2421803, 15656409, -7673309,
12157821, -12094775, 6811602, -15207921, 430220, -16675992, -6016543, -16340180,
-11548969, -14236814, -15326292, -10593420, -16774471, -5804110, -15673423, -386956,
-12190477, 5072055, -6854938, 9982411, -477651, 13812945, 5972225, 16149301,
11514499, 16738751, 15306908, 15517529, 16773118, 12617741, 15690307, 8353062,
12223031, 3184813, 6898215, -2327944, 525073, -7588883, -5927864, -12028915,
-11479941, -15167751, -15287405, -16665857, -16771636, -16361177, -15707069, -14286672,
-12255492, -10666744, -6941441, -5892970, -572496, -481740, 5883451, 4981602,
11445288, 9906071, 15267777, 13758977, 16770015, 16123544, 15723702, 16743990,
12287851, 15553197, 6984608, 12679980, 619910, 8435140, -5838996, 3277851,
-11410546, -2234010, -15248030, -7504214, -16768264, -11962670, -15740214, -15127095,
-12320115, -16655190, -7027723, -16381651, -667323, -14336072, 5794489, -10739728,
11375710, -5981642, 15228158, -576508, 16766375, 4890988, 15756595, 9829415,
12352278, 13704570, 7070777, 16097271, 714727, 16748693, -5749940, 15588368,
-11340787, 12741814, -15208168, 8516948, -16764356, 3370784, -15772855, -2140005,
-12384345, -7419306, -7113779, -11896043, -762129, -15085957, 5705342, -16643990,
11305769, -16401601, 15188052, -14385014, 16762200, -10812368, 15788984, -6070123,
12416309, -671258, 7156721, 4800219, 809521, 9752444, -5660701, 13649724,
-11270665, 16070483, -15167819, 16752860, -16759913, 15623040, -15804992, 12803240,
-12448178, 8598484, -7199609, 3463609, -856911, -2045931, 5616012, -7334160,
11235466, -11829035, 15147461, -15044335, 16757488, -16632257, 15820868, -16421027,
12479944, -14433497, 7242435, -10884663, 904289, -6158409, -5571281, -765986,
-11200182, 4709296, -15126985, 9675162, -16754933, 13594442, -15836623, 16043181,
-12511613, 16756492, -7285208, 15657212, -951665, 12864257, 5526502, 8679745,
11164804, 3556324, 15106385, -1951792, 16752240, -7248779, 15852247, -11761649,
12543179, -15002233, 7327918, -16619993, 999029, -16439927, -5481683, -14481517,
-11129341, -10956609, -15085667, -6246499, -16749418, -860690, -15867748, 4618222,
-12574649, 9597570, -7370574, 13538725, -1046389, 16015366, 5436816, 16759588,
11093785, 15690884, 15064826, 12924862, 16746457, 8760728, 15883118, 3648925,
12606014, -1857590, 7413166, -7163167, 1093736, -11693886, -5391910, -14959651,
-11058145, -16607197, -15043867, -16458302, -16743366, -14529075, -15898365, -11028205,
-12637282, -6334388, -7455704, -955366, -1141079, 4527001, 5346956, 9519671,
11022412, 13482575, 15022785, 15987039, 16740138, 16762148, 15913481, 15724054,
12668445, 12985054, 7498178, 8841431, 1188408, 3741409, -5301964, -1763330,
-10986595, -7077325, -15001586, -11625750, -16736780, -14916590, -15928474, -16593870,
-12699511, -16476150, -7540596, -14576168, -1235733, -11099448, 5256925, -6422075,
10950686, -1050011, 14980264, 4435635, 16733284, 9441468, 15943336, 13425994,
12730471, 15958200, 7582949, 16764172, 1283043, 15756722, -5211849, 13044831,
-10914693, 8921851, -14958825, 3833773, -16729658, -1669012, -15958074, -6991258,
-12761334, -11557242, -7625246, -14873053, -1330347, -16580013, 5166726, -16493472,
10878609, -14622794, 14937264, -11170336, 16725895, -6509557, 15972681, -1144623,
12792090, 4344127, 7667479, 9362963, 1377636, 13368984, -5121567, 15928852,
-10842442, 16765660, -14915586, 15788885, -16722002, 13104191, -15987164, 9001986,
-12822748, 3926015, -7709653, -1574642, -1424918, -6904966, 5076362, -11488365,
10806185, -14829040, 14893786, -16565625, 16717971, -16510266, 16001515, -14668954,
12853300, -11240868, 7751763, -6596831, 1472185, -1239199, -5031121, 4252480,
-10769845, 9284158, -14871870, 13311546, -16713810, 15898994, -16015742, 16766611,
-12883753, 15820544, -7793814, 13163131, -1519444, 9081834, 4985835, 4018131,
10733415, -1480221, 14849832, -6818454, 16709512, -11419120, 16029837, -14784553,
12914099, -16550708, 7835799, -16526532, 1566687, -14714644, -4940514, -11311039,
-10696903, -6683893, -14827679, -1333735, -16705084, 4160697, -16043808, 9205057,
-12944346, 13253683, -7877725, 15868628, -1613922, 16767027, 4895149, 15851696,
10660302, 13221651, 14805403, 9161390, 16700519, 4110119, 16057647, -1385752,
12974485, -6731724, 7919584, -11349510, 1661139, -14739593, -4849749, -16535261,
-10623620, -16542270, -14783013, -14759863, -16695824, -11380849, -16071361, -6770742,
-13004525, -1428228, -7961384, 4068781, -1708348, 9125661, 4804307, 13195395,
10586848, 15837754, 14760501, 16766907, 16690992, 15882342, 16084943, 13279748,
13034456, 9240654, 8003116, 4201975, 1755538, -1291240, -4758830, -6644779,
-10549996, -11279538, -14737874, -14694161, -16686030, -16519286, -16098400, -16557479,
-13064288, -14804611, -8044789, -11450295, -1802719, -6857375, 4713311, -1522675,
10513056, 3976735, 14715126, 9045973, 16680931, 13136686, 16111725, 15806374,
13094011, 16766250, 8086393, 15912480, 1849881, 13337421, -4667758, 9319622,
-10476036, 4293697, -14692264, -1196686, -16675703, -6557621, -16124924, -11209204,
-13123633, -14648260, -8127936, -16502782, -1897033, -16572158, 4622164, -14848885,
10438927, -11519375, 14669281, -6943788, 16670337, -1617074, 16137991, 3884562,
13153146, 8965997, 8169410, 13077557, 1944165, 15774488, -4576537, 16765057,
-10401740, 15942109, -14646185, 13394666, -16664842, 9398293, -16150933, 4385282,
-13182558, -1102093, -8210824, -6470254, -1991285, -11138512, 4530869, -14601891,
10364465, -16485751, 14622967, -16586307, 16659210, -14892685, 16163742, -11588087,
13211861, -7029979, 8252167, -1711421, 2038386, 3792265, -4485169, 8885733,
-10327111, 13018009, -14599637, 15742098, -16653449, 16763328, -16176425, 15971228,
-13241062, 13451484, -8293448, 9476662, -2085475, 4476727, 4439429, -1007466,
10289671, -6382679, 14576186, -11067464, 16647550, -14555054, 16188976, -16468192,
13270154, -16599926, 8334660, -14936008, 2132543, -11656428, -4393658, -7115945,
-10252153, -1805713, -14552622, 3699846, -16641522, 8805185, -16201400, 12958045,
-13299143, 15709205, -8375808, 16761063, -2179597, 15999837, 4347848, 13507871,
10214548, 9554729, 14528938, 4568028, 16635358, -912806, 16213692, -6294901,
13328022, -10996062, 8416886, -14507752, 2226631, -16450107, -4302007, -16613014,
-10176866, -14978853, -14505142, -11724396, -16629064, -7201684, -16225857, -1899948,
-13356798, 3607310, -8457900, 8724356, -2273650, 12897667, 4256127, 15675809,
10139099, 16758262, 14481226, 16027934, 16622633, 13563827, 16237889, 9632490,
13385464, 4659183, 8498843, -818117, 2320648, -6206921, -4210218, -10924308,
-10101254, -14459986, -14457198, -16431496, -16616074, -16625571, -16249795, -15021220,
-13414027, -11791989, -8539722, -7287192, -2367630, -1994122, 4164270, 3514657,
10063325, 8643248, 14433050, 12836877, 16609378, 15641912, 16261568, 16754925,
13442478, 16055518, 8580528, 13619348, 2414590, 9709943, -4118294, 4750190,
-10025319, -723402, -14408791, -6118743, -16602553, -10852205, -16273214, -14411758,
-13470826, -16412359, -8621270, -16637596, -2461535, -15063106, 4072280, -11859206,
9987229, -7372468, 14384413, -2088232, 16595591, 3421893, 16284726, 8561863,
13499062, 12775676, 8661939, 15607514, 2508456, 16751053, -4026238, 16082589,
-9949064, 13674435, -14359924, 9787086, -16588500, 4841044, -16296112, -628664,
-13527195, -6030369, -8702543, -10779755, -2555361, -14363069, 3980160, -16392698,
9910814, -16649090, 14335316, -15104511, 16581273, -11926043, 16307363, -7457507,
13555215, -2182275, 8743073, 3329018, 2602241, 8480205, -3934054, 12714066,
-9872490, 15572618, -14310598, 16746644, -16573917, 16109146, -16318489, 13729084,
-13583131, 9863916, -8783537, 4931744, -2649105, -533906, 3887913, -5941802,
9834083, -10706960, 14285761, -14313920, 16566425, -16372512, 16329480, -16660051,
13610934, -15145432, 8823927, -11992498, 2695943, -7542309, -3841744, -2276249,
-9795600, 3236038, -14260814, 8398275, -16558805, 12652050, -16340344, 15537224,
-13638632, 16741700, -8864251, 16135188, -2742764, 13783294, 3795541, 9940430,
9757036, 5022286, 14235749, -439130, 16551048, -5853045, 16351074, -10633823,
13666218, -14264314, 8904499, -16351803, 2789559, -16670479, -3749311, -15185870,
-9718398, -12058571, -14210574, -7626869, -16543162, -2370150, -16361677, 3142954,
-13693698, 8316077, -8944681, 12589629, -2836336, 15501332, 3703048, 16736221,
9679678, 16160713, 14185282, 13837063, 16535141, 10016626, 16372145, 5112667,
13721064, -344341, 8984787, -5764101, 2883086, -10560346, -3656759, -14214252,
-9640884, -16330571, -14159880, -16680374, -16526991, -15225821, -16382486, -12124257,
-13748325, -7711185, -9024825, -2463974, -2929817, 3049769, 3610437, 8233613,
9602010, 12526806, 14134361, 15464946, 16518705, 16730207, 16392692, 16185722,
13775472, 13890389, 9064787, 10092502, 2976520, 5202885, -3564089, -249541,
-9563063, -5674973, -14108733, -10486531, -16510291, -14163735, -16402771, -16308817,
-13802513, -16689735, -9104680, -15265286, -3023204, -12189556, 3517710, -7795254,
9524035, -2557721, 14082988, 2956487, 16501741, 8150885, 16412715, 12463582,
13829440, 15428064, 9144497, 16723657, 3069859, 16210213, -3471306, 13943272,
-9484935, 10168056, -14057134, 5292936, -16493063, -154732, -16422532, -5585663,
-13856260, -10412380, -9184244, -14112765, -3116494, -16286541, 3424870, -16698563,
9445756, -15304262, 14031165, -12254465, 16484250, -7879074, 16432214, -2651385,
13882965, 2863111, 9223914, 8067897, 3163100, 12399959, -3378412, 15390689,
-9406505, 16716573, -14005087, 16234186, -16475308, 13995708, -16441768, 10243283,
-13909563, 5382818, -9263515, -59920, -3209685, -5496175, 3331922, -10337897,
9367175, -14061344, 13978893, -16263745, 16466231, -16706857, 16451187, -15342749,
13936045, -12318981, 9303035, -7962641, 3256238, -2744962, -3285411, 2769645,
-9327774, 7984653, -13952591, 12335941, -16457026, 15352822, -16460479, 16708955,
-13962424, 16257641, -9342492, 14047699, -3302776, 10318186, 3238864, 5472529,
9288292, 34897, 13926173, -5406510, 16447686, -10263083, 16469634, -14009473,
13988682, -16240428, 9381863, -16714617, 3349277, -15380746, -3192300, -12383106,
-9248743, -8045957, -13899648, -2838457, -16438218, 2676085, -16478662, 7901149,
-14014834, 12271526, -9421165, 15314464, -3395757, 16700802, 3145706, 16280574,
9209115, 14099237, 13873009, 10392755, 16428615, 5562063, 16487554, 129711,
14040870, -5316673, 9460388, -10187941, 3442206, -13957154, -3099091, -16216592,
-9169417, -16721842, -13846262, -15418251, -16418884, -12446833, -16496319, -8129013,
-14066798, -2931858, -9499540, 2582443, -3488631, 7817396, 3052447, 12206721,
9129642, 15275617, 13819401, 16692115, 16409018, 16302987, 16504947, 14150326,
14092610, 10466994, 9538611, 5651420, 3535024, 224521, -3005782, -5226666,
-9089799, -10112473, -13792433, -13904390, -16399025, -16192237, -16513448, -16728533,
-14118312, -15455263, -9577610, -12510162, -3581393, -8211809, 2959090, -3025165,
9049878, 2488719, 13765351, 7733392, 16388897, 12141525, 16521813, 15236281,
14143898, 16682894, 9616529, 16324879, 3627730, 14200963, -2912377, 10540897,
-9009889, 5740597, -13738163, 319323, -16378642, -5136492, -16530049, -10036682,
-14169375, -13851180, -9655374, -16167365, -3674041, -16734688, 2865638, -15491780,
8969824, -12573092, 13710861, -8294343, 16368252, -3118375, 16538150, 2394914,
14194735, 7649141, 9694139, 12075940, 3720319, 15196458, -2818879, 16673140,
-8929692, 16346249, -13683454, 14251145, -16357735, 10614464, -16546122, 5829590,
-14219985, 414116, -9732830, -5046154, -3766572, -9960570, 2772094, -13797528,
8889484, -16141976, 13655933, -16740309, 16347083, -15527803, 16553958, -12635619,
14245117, -8376612, 9771439, -3211486, 3812790, 2301033, -2725291, 7564645,
-8849208, 12009969, -13628307, 15156148, -16336305, 16662852, -16561665, 16367096,
-14270140, 14300872, -9809974, 10687691, -3858982, 5918396, 2678462, 508895,
8808859, -4955654, 13600568, -9884140, 16325392, -13743434, 16569237, -16116070,
14295044, -16745394, 9848427, -15563328, 3905139, -12697742, -2631616, -8458612,
-8768442, -3304494, -13572724, 2207079, -16314352, 7479907, -16576679, 11943615,
-14319839, 15115355, -9886805, 16652032, -3951268, 16387420, 2584744, 14350141,
8727952, 10760576, 13544768, 6007013, 16303178, 603658, 16583985, -4864996,
14344514, -9807393, 9925100, -13688901, 3997363, -16089649, -2537856, -16749943,
-8687396, -15598356, -13516707, -12759459, -16291878, -8540343, -16591163, -3397397,
-14369079, 2113054, -9963320, 7394930, -4043429, 11876878, 2490944, 15074077,
8646766, 16640679, 13488535, 16407220, 16280443, 14398952, 16598204, 10833118,
14393525, 6095439, 10001455, 698402, 4089459, -4774182, -2444015, -9730333,
-8606071, -13633929, -13460258, -16062714, -16268883, -16753957, -16605116, -15632885,
-14417861, -12820768, -10039515, -8621800, -4135460, -3490191, 2397063, 2018961,
8565304, 7309716, 13431870, 11809762, 16257188, 15032318, 16611892, 16628794,
14442076, 16426495, 10077491, 14447302, 4181424, 10905313, -2350096, 6183669,
-8524472, 793124, -13403379, -4683215, -16245367, -9652961, -16618539, -13578523,
-14466181, -16035265, -10115390, -16757436, -4227359, -15666914, 2303107, -12881667,
8483568, -8702981, 13374776, -3582873, 16233413, 1924803, 16625049, 7224269,
14490165, 11742268, 10153205, 14990078, 4273256, 16616378, -2256102, 16445245,
-8442600, 14495190, -13346070, 10977159, -16221332, 6271701, -16631430, 887820,
-14514038, -4592099, -10190942, -9575281, -4319122, -13522681, 2209076, -16007303,
8401560, -16760378, 13317254, -15700442, 16209118, -12942154, 16637674, -8783885,
14537791, -3675440, 10228593, 1830584, 4364950, 7138591, -2162036, 11674398,
-8360458, 14947359, -13288336, 16603430, -16196779, 16463469, -16643789, 14542615,
-14561432, 11048654, -10266167, 6359533, -4410748, 982487, 2114975, -4500836,
8319284, -9497295, 13259307, -13466408, 16184305, -15978830, 16649767, -16762784,
14584952, -15733469, 10303655, -13002228, 4456506, -8864507, -2067901, -3767891,
-8278048, 1736307, -13230176, 7052684, -16171707, 11606155, -16655615, 14904162,
-14608360, 16589951, -10341065, 16481166, -4502232, 14589574, 2020806, 11119796,
8236742, 6447162, 13200935, 1077124, 16158975, -4409429, 16661327, -9419005,
14631646, -13409704, 10378387, -15949845, 4547919, -16764655, -1973700, -15765992,
-8195374, -13061885, -13171593, -8944846, -16146118, -3860220, -16666910, 1641974,
-14654820, 6966552, -10415631, 11537541, -4593573, 14860488, 1926573, 16575941,
8153936, 16498337, 13142142, 14636067, 16133128, 11190582, 16672355, 6534584,
14677873, 1171725, 10452788, -4317881, 4639186, -9340414, -1879435, -13352571,
-8112438, -15920350, -13112589, -16765989, -16120013, -15798011, -16677671, -13121125,
-14700812, -9024898, -10489864, -3952426, -4684766, 1547589, 1832278, 6880197,
8070870, 11468558, 13082928, 14816339, 16106765, 16561402, 16682850, 16514980,
14723630, 14682092, 10526854, 11261010, 4730305, 6621798, -1785111, 1266290,
-8029242, -4226195, -13053166, -9261524, -16093393, -13295011, -16687899, -15890347,
-14746334, -16766787, -10563763, -15829524, -4775810, -13179945, 1737925, -9104663,
7987546, -4044506, 13023295, 1453154, 16079888, 6793623, 16692811, 11399209,
14768917, 14771716, 10600583, 16546333, 4821273, 16531095, -1690729, 14727648,
-7945790, 11331078, -12993325, 6708799, -16066258, 1360814, -16697593, -4134374,
-14791385, -9182338, -10637323, -13237026, -4866701, -15859835, 1643516, -16767049,
7903966, -15860532, 12963247, -13238344, 16052495, -9184136, 16702238, -4136456,
14813731, 1358673, 10673973, 6706831, 4912086, 11329495, -1596293, 14726621,
-7862083, 16530735, -12933069, 16546681, -16038609, 14772732, -16706753, 11400784,
-14835962, 6795586, -10710543, 1455294, -4957436, -4042420, 1549054, -9102858,
7820134, -13178617, 12902783, -15828816, 16024590, -16766775, 16711131, -15891032,
14858071, -13296320, 10747023, -9263315, 5002743, -4228275, -1501806, 1264148,
-7778126, 6619824, -12872399, 11259418, -16010447, 14681054, -16715380, 16514608,
-14880065, 16561738, -10783420, 14817344, -5048013, 11470126, 1454543, 6882156,
7736051, 1549728, 12841908, -3950338, 15996172, -9023087, 16719490, -13119787,
14901936, -15797291, 10819728, -16765965, 5093239, -15921024, -1407272, -13353870,
-7693919, -9342198, -12811318, -4319958, -15981773, 1169583, -16723471, 6532606,
-14923692, 11188982, -10855953, 14635019, -5138429, 16497953, 1359985, 16576265,
7651721, 14861483, 12780621, 11539100, 15967243, 6968506, 16727314, 1644112,
14945325, -3858129, 10892087, -8943028, 5183573, -13060538, -1312692, -15765260,
-7609466, -16764618, -12749826, -15950507, -15952589, -13410993, -16731028, -9420782,
-14966842, -4411502, -10928139, 1074980, -5228680, 6445179, 1265384, 11118187,
7567147, 14588515, 12718926, 16480770, 15937803, 16590263, 16734604, 14905146,
14988236, 11607706, 10964099, 7054633, 5273741, 1738444, -1218070, -3765796,
-7524770, -8862683, -12687928, -13000871, -15922894, -15732726, -16738050, -16762736,
-15009514, -15979480, -10999975, -13467688, -5318764, -9499065, 1170742, -4502906,
7482330, 980343, 12656824, 6357546, 15907853, 11047038, 16741358, 14541545,
15030668, 16463061, 11035759, 16603730, 5363740, 14948332, -1123409, 11675940,
-7439834, 7140534, -12625623, 1832720, -15892690, -3673344, -16744536, -8782054,
-15051705, -12940788, -11071459, -15699688, -5408678, -16760317, 1076063, -16007942,
7397274, -13523952, 12594317, -9577045, 15877395, -4594166, 16747577, 885674,
15072618, 6269709, 11107067, 10975534, 5453568, 14494110, -1028713, 16444825,
-7354659, 16616666, -12562915, 14991040, -15861977, 11743801, -16750488, 7226207,
-15093415, 1926937, -11142590, -3580773, -5498419, -8701145, 981350, -12880292,
7311982, -15666149, 12531408, -16757363, 15846429, -16035892, 16753260, -13579783,
15114087, -9654718, 11178019, -4685279, 5543221, 790978, -933983, 6181672,
-7269250, 10903680, -12499805, 14446212, -15830758, 16426063, -16755903, 16629070,
-15134643, 15033270, -11213364, 11811287, -5587984, 7311649, 886605, 2021093,
7226456, -3488088, 12468098, -8619957, 15814956, -12819383, 16758408, -15632108,
15155073, -16753873, 11248614, -16063330, 5632697, -13635180, -839224, -9732082,
-7183608, -4776242, -12436295, 696256, -15799032, 6093438, -16760783, 10831478,
-15175386, 14397851, -11283779, 16406776, -5677370, 16640943, 791832, 15075018,
7140698, 11878395, 12404389, 7396857, 15782978, 2115184, 16763020, -3395292,
15195574, -8538493, 11318850, -12758065, 5721993, -15597568, -744437, -16749846,
-7097736, -16090253, -12372388, -13690141, -15766801, -9809136, -16765126, -4867052,
-15215644, 601511, -11353834, 6005008, -5766575, 10758928, 697033, 14349030,
7054712, 16386965, 12340283, 16652284, 15750495, 15116284, 16767095, 11945123,
15235589, 7481829, 11388723, 2209208, 5811106, -3302387, -649627, -8456757,
-7011637, -12696339, -12308085, -15562529, -15734066, -16745285, -16768934, -16116663,
-15255416, -13744664, -11423526, -9885875, -5855596, -4957707, 602212, 506748,
6968501, 5916386, 12275783, 10686035, 15717508, 14299750, 16770635, 16366629,
15275117, 16663092, 11458232, 15157067, 5900034, 12011469, -554796, 7566561,
-6925314, 2303161, -12243388, -3209377, -15700828, -8374750, -16772205, -12634206,
-15294700, -15526992, -11492852, -16740188, -5944429, -16142556, 507372, -13798748,
6882067, -9962299, 12210891, -5048203, 15684018, 411968, 16773638, 5827576,
15314157, 10612800, 11527375, 14250013, 5988772, 16345770, -459948, 16673367,
-6838769, 15197365, -12178300, 12077431, -15667087, 7651052, -16774940, 2397040,
-15333495, -3116263, -11561811, -8292475, -6033072, -12571670, 412516, -15490958,
6795413, -16734555, 12145607, -16167934, 15650027, -13852391, 16776105, -10038403,
15352706, -5138538, 11596150, 317175, 6077319, 5738579, -365084, 10539226,
-6752006, 14199821, -12112822, 16324388, -15632846, 16683110, -16777139, 15237178,
-15371799, 12143006, -11630400, 7735298, -6121522, 2490843, 317646, -3023051,
6708541, -8209935, 12079936, -12508732, 15615536, -15454430, 16778035, -16728387,
15390765, -16192794, 11664553, -13905590, 6165672, -10114187, -270209, -5228708,
-6665027, 222372, -12046957, 5649398, -15598105, 10465315, -16778801, 14149174,
-15409612, 16302484, -11698617, 16692318, -6209776, 15276503, 222767, 12208194,
6621456, 7819296, 12013878, 2584566, 15580545, -2929741, 16779429, -8127133,
15428332, -12445393, 11732583, -15417407, 6253827, -16721685, -175326, -16217137,
-6577835, -13958345, -11980707, -10189647, -15562865, -5318711, -16779927, 127562,
-15446932, 5560037, -11766460, 10391069, -6297832, 14098075, 127880, 16280059,
6534158, 16700993, 11947437, 15315339, 15545056, 12272990, 16780286, 7903044,
15465405, 2678206, 11800238, -2836339, 6341783, -8044071, -80437, -12381657,
-6490433, -15379891, -11914074, -16714447, -15527127, -16240961, -16780516, -14010653,
-15483758, -10264782, -11833926, -5408544, -6385686, 32749, 32990, 5470499,
6446653, 10316493, 11880614, 14046526, 15509071, 16257115, 16780607, 16709134,
15501984, 15353686, 11867517, 12337396, 6429538, 7986542, 14458, 2771764,
-6402820, -2742842, -11847058, -7960750, -15490892, -12317524, -16780568, -15341882,
-15520090, -16706675, -11901016, -16264266, -6473338, -14062514, -61904, -10339589,
6358938, -5498205, 11813408, -62068, 15472588, 5380783, 16780391, 10241582,
15538067, 13994525, 11934415, 16233648, 6517083, 16716740, 109347, 15391542,
-6315007, 12401405, -11779666, 8069780, -15454163, 2865228, -16780084, -2649262,
-15555924, -7877177, -11967723, -12252998, -6560781, -15303384, -156793, -16698370,
6271022, -16287051, 11745826, -14113925, 15435611, -10414064, 16779639, -5587689,
15573653, -156881, 12000932, 5290897, 6604422, 10166347, 204235, 13942078,
-6226990, 16209663, -11711897, 16723812, -15416940, 15428905, -16779063, 12465019,
-15591261, 8152762, -12034048, 2958602, -6648014, -2555596, -251678, -7793351,
6182905, -12188081, 11677869, -15264397, 15398141, -16689530, 16778350, -16309315,
15608741, -14164885, 12067064, -10488207, 6691549, -5676995, 299116, -251689,
-6138775, 5200842, -11643753, 10090787, -15379223, 13889186, -16777506, 16185161,
-15626100, 16730350, -12099988, 15465775, -6735035, 12528234, -346555, 8235484,
6094591, 3051882, 11609539, -2461849, 15360178, -7709276, 16776524, -12122773,
15643330, -15224921, 12132812, -16680156, 6778463, -16331057, 393987, -14215391,
-6050363, -10562014, -11575236, -5766119, -15341015, -346489, -16775412, 5110621,
-15660439, 10014903, -12165542, 13835849, -6821840, 16160140, -441421, 16736352,
6006082, 15502151, 11540837, 12591048, 15321724, 8317942, 16774162, 3145064,
15677419, -2368022, 12198171, -7624955, 6865160, -12057078, 488847, -15184959,
-5961757, -16670249, -11506349, -16352278, -15302316, -14265443, -16772782, -10635484,
-15694277, -5855059, -12230706, -441278, -6908428, 5020236, -536272, 9938700,
5917380, 13782070, 11471766, 16134603, 15282780, 16741819, 16771264, 15538031,
15711006, 12653460, 12263140, 8400134, 6951637, 3238146, 583690, -2274120,
-5872961, -7540389, -11437095, -11990997, -15263127, -15144510, -16769615, -16659809,
-15727614, -16372975, -12295479, -14315039, -6994794, -10708613, -631107, -5943811,
5828490, -536054, 11402328, 4929690, 15243348, 9862178, 16767829, 13727850,
15744091, 16108550, 12327717, 16746750, 7037892, 15573414, 678515, 12715467,
-5783976, 8482058, -11367474, 3331124, -15223451, -2180144, -16765912, -7455582,
-15760447, -11924532, -12359859, -15103578, -7080937, -16648836, -725921, -16393149,
5739413, -14364177, 11332525, -10781400, 15203428, -6032374, 16763858, -630811,
15776673, 4838987, 12391899, 9785341, 7123922, 13673191, 773318, 16081981,
-5694807, 16751147, -11297490, 15608299, -15183287, 12777067, -16761673, 8563710,
-15792777, 3423995, -12423844, -2086099, -7166854, -7370537, -820713, -11857687,
5650152, -15062162, 11262361, -16637331, 15163022, -16412798, 16759351, -14412856,
15808750, -10853843, 12455686, -6120744, 7209724, -725549, 868097, 4748129,
-5605456, 9708192, -11227145, 13618095, -15142638, 16054899, -16756898, 16755007,
-15824601, 15642685, -12487432, 12838259, -7252541, 8645088, -915478, 3516758,
5560711, -1991988, 11191836, -7285256, 15122130, -11790462, 16754308, -15020265,
15840322, -16625293, 12519074, -16431923, 7295296, -14461074, 962847, -10925938,
-5515926, -6208917, -11156441, -820264, -15101505, 4657120, -16751587, 9630731,
-15855920, 13562564, -12550620, 16027303, -7337996, 16758332, -1010213, 15676571,
5471092, 12899040, 11120953, 8726190, 15080756, 3609407, 16748729, -1897813,
15871387, -7199743, 12582062, -11722860, 7380634, -14977888, 1057567, -16612724,
-5426219, -16450522, -11085380, -14508829, -15059890, -10997684, -16745741, -6296893,
-15886731, -914952, -12613407, 4565961, -7423217, 9552963, -1104917, 13506598,
5381298, 15999194, 11049714, 16761121, 15038899, 15709956, 16742615, 12959409,
15901944, 8807013, 12644647, 3701942, 7465736, -1803577, 1152253, -7113999,
-5336339, -11654883, -11013964, -14935031, -15017792, -16599624, -16739359, -16468595,
-15917034, -14556121, -12675791, -11069078, -7508200, -6384667, -1199585, -1009611,
5291332, 4474656, 10978122, 9474890, 14996561, 13450201, 16735965, 15970574,
15931993, 16763373, 12706828, 15742838, 7550600, 13019363, 1246902, 8887555,
-5246287, 3794358, -10942197, -1709283, -14975214, -7028027, -16732442, -11586534,
-15946829, -14891697, -12737769, -16585993, -7592943, -16486142, -1294214, -14602947,
5201197, -11140118, 10906180, -6472236, 14953744, -1104238, 16728780, 4383209,
15961533, 9396513, 12768603, 13393374, 7635222, 15941443, 1341512, 16765090,
-5156069, 15775216, -10870079, 13078901, -14932158, 8967812, -16724989, 3886652,
-15976113, -1614935, -12799340, -6941831, -7677444, -11517814, -1388802, -14847887,
5110895, -16571832, 10833888, -16503161, 14910448, -14649306, 16721060, -11210802,
15990562, -6559599, 12829970, -1198829, 7719600, 4291621, 1436078, 9317836,
-5065685, 13336118, -10797614, 15911803, -14888624, 16766271, -16717002, 15807091,
-16004887, 13138021, -12860501, 9047782, -7761698, 3978822, -1483346, -1520535,
5020430, -6855413, 10761250, -11448726, 14866676, -14803602, 16712806, -16557141,
16019080, -16519653, 12890926, -14695196, 7803731, -11281127, 1530599, -6646752,
-4975140, -1293382, -10724804, 4199896, -14844613, 9238861, -16708480, 13278436,
-16033149, 15881653, -12921252, 16766915, -7845705, 15838459, -1577843, 13196721,
4929805, 9127463, 10688268, 4070865, 14822428, -1426086, 16704017, -6768776,
16047085, -11379271, 12951470, -14758844, 7887612, -16541920, 1625070, -16535616,
-4884435, -14740617, -10651650, -11351092, -14800128, -6733693, -16699424, -1387894,
-16060898, 4108036, -12981589, 9159591, -7929461, 13220330, -1672289, 15850996,
4839022, 16767024, 10614944, 15869322, 14777706, 13254998, 16694694, 9206852,
16074578, 4162778, 13011600, -1331592, 7971242, -6681922, 1719490, -11309453,
-4793574, -14713614, -10578156, -16526170, -14755170, -16551051, -16689834, -14785566,
-16088133, -11420693, -13041511, -6820418, -8012963, -1482361, -1766681, 4016046,
4748084, 9080028, 10541280, 13161801, 14732512, 15819832, 16684837, 16766596,
16101556, 15899676, 13071314, 13312852, 8054616, 9285947, 1813854, 4254558,
-4702560, -1237056, -10504324, -6594855, -14709740, -11239273, -16679710, -14667913,
-16114854, -16509892, -13101016, -16565956, -8096209, -14830043, -1861017, -11489930,
4656994, -6906925, 10467280, -1576781, 14686847, 3923927, 16674447, 9000174,
16128020, 13102850, 13130609, 15788162, 8137733, 16765632, 1908161, 15929523,
-4611396, 13370280, -10430156, 9364745, -14663840, 4346201, -16669053, -1142479,
-16141060, -6507576, -13160102, -11168734, -8179196, -14621743, -1955293, -16493086,
4565756, -16580332, 10392945, -14874045, 14640712, -11558799, 16663523, -6993211,
16153968, -1671151, 13189485, 3831682, 8220590, 8920033, 2002406, 13043481,
-4520083, 15755987, -10355654, 16764132, -14617471, 15958860, -16657863, 13427281,
-16166750, 9443243, -13218767, 4437706, -8261922, -1047867, -2049507, -6420090,
4474371, -11097838, 10318277, -14575106, 14594109, -16475752, 16652066, -16594177,
16179399, -14917572, 13247939, -11627298, 8303183, -7079274, 2096588, -1765467,
-4428627, 3739315, -10280822, 8839606, -14570634, 12983695, -16646140, 15723308,
-16191922, 16762096, -13277010, 15987687, -8344383, 13483852, -2143656, 9521439,
4382843, 4529069, 10243280, -953220, 14547039, -6332398, 16640077, -11026586,
16204313, -14528002, 13305970, -16457892, 8385512, -16607492, 2190702, -14960621,
-4337029, -11695426, -10205660, -7165110, -14523331, -1859726, -16633885, 3646828,
-16216577, 8758896, -13334828, 12923493, -8426577, 15690127, -2237736, 16759523,
4291175, 16016002, 10167955, 13539992, 14499504, 9599331, 16627556, 4620287,
16228708, -858544, 13363575, -6244504, 8467572, -10954983, 2284747, -14480434,
-4245292, -16439505, -10130172, -16620276, -14475564, -15003192, -16621098, -11763180,
-16240714, -7250717, -13392219, -1953927, -8508502, 3554225, -2331744, 8677907,
4199370, 12862878, 10092304, 15656444, 14451505, 16756415, 16614503, 16043805,
16252585, 13595699, 13420753, 9676916, 8549361, 4711357, 2378718, -763839,
-4153419, -6156410, -10054360, -10883028, -14427334, -14432403, -16607780, -16420593,
-16264331, -16632528, -13449183, -15045284, -8590155, -11830557, -2425678, -7336092,
4107431, -2048064, 10016331, 3461507, 14403044, 8596640, 16600919, 12801852,
16275942, 15622260, 13477501, 16752771, 8630877, 16071096, 2472614, 13650972,
-4061414, 9754192, -9978227, 4802277, -14378643, -669111, -16593930, -6068119,
-16287428, -10810726, -13505716, -14383910, -8671534, -16401156, -2519534, -16644248,
4015360, -15086894, 9940038, -11897556, 14354123, -7421233, 16586804, -2142136,
16298779, 3368680, 13533819, 8515098, 8712117, 12740417, 2566430, 15587576,
-3969278, 16748592, -9901774, 16097872, -14329492, 13705808, -16579550, 9831156,
-16310004, 4893043, -13561817, -574360, -8752635, -5979634, -2613310, -10738078,
3923161, -14334958, 9863427, -16381194, 14304742, -16655437, 16572159, -15128022,
16321094, -11964175, 13589703, -7506136, 8793079, -2236140, 2660164, 3275744,
-3877016, 8433284, -9825005, 12678574, -14279883, 15552394, -16564639, 16743876,
-16332058, 16124134, -13617485, 13760205, -8833456, 9907805, -2707002, 4983653,
3830836, -479592, 9786501, -5890958, 14254905, -10665087, 16556983, -14285547,
16342888, -16360708, 13645154, -16666092, 8873759, -15168666, 2753814, -12030411,
-3784630, -7590800, -9747922, -2330072, -14229817, 3182704, -16549199, 8351200,
-16353590, 12616326, -13672717, 15516715, -8913995, 16738626, -2800607, 16149880,
3738389, 13814163, 9709261, 9984137, 14204611, 5074103, 16541279, -384808,
16364158, -5802094, 13700168, -10591754, 8954155, -14235679, 2847375, -16339699,
-3692123, -16676215, -9670527, -15208825, -14179296, -12096263, -16533230, -7675220,
-16374600, -2423930, -13727512, 3089562, -8994248, 8268849, -2894123, 12553674,
3645823, 15480540, 9631711, 16732840, 14153863, 16175109, 16525045, 13867678,
16384906, 10060150, 13754743, 5164391, 9034265, -290012, 2940845, -5713044,
-3599498, -10518083, -9592823, -14185356, -14128321, -16318168, -16516732, -16685804,
-16395085, -15248498, -13781868, -12161727, -9074214, -7759395, -2987547, -2517710,
3553140, 2996321, 9553853, 8186234, 14102663, 12490621, 16508283, 15443869,
16405130, 16726519, 13808879, 16199821, 9114086, 13920751, 3034221, 10135842,
-3506757, 5254513, -9514812, -195207, -14076895, -5623811, -16499706, -10444076,
-16415047, -14134579, -13835784, -16296114, -9153890, -16694860, -3080874, -15287683,
3460343, -12226803, 9475690, -7843322, 14051011, -2611410, 16490993, 2902985,
16424829, 8103357, 13862573, 12427168, 9193616, 15406705, 3127499, 16719663,
-3413905, 16224015, -9436497, 13973378, -14025019, 10211209, -16482152, 5344468,
-16434483, -100396, -13889256, -5534399, -9233273, -10369735, -3174103, -14083351,
3367436, -16273540, 9397224, -16703382, 13998910, -15326379, 16473175, -12291487,
16444002, -7926998, 13915824, -2705025, 9272851, 2809556, 3220678, 8020221,
-3320944, 12363319, -9357880, 15369048, -13972695, 16712272, -16464072, 16247691,
-16453394, 14025557, -13942284, 10286248, -9312359, 5434250, -3267230, -5582,
3274420, -5444810, 9318456, -10295061, 13946361, -14031671, 16454831, -16250445,
16462651, -16711370, 13968632, -15364587, 9351792, -12355781, 3313755, -8010422,
-3227874, -2798556, -9278963, 2716035, -13919922, 7936827, -16445464, 12299072,
-16471779, 15330899, -13994869, 16704347, -9391151, 16270847, -3360255, 14077290,
3181300, 10360962, 9239391, 5523863, 13893367, 89235, 16435961, -5355045,
16480772, -10220059, 14020990, -13979544, 9430432, -16226831, 3406725, -16718823,
-3134704, -15402301, -9199750, -12419676, -13866705, -8093588, -16426331, -2891996,
-16489637, 2622430, -14047004, 7853181, -9469641, 12234434, -3453171, 15292261,
3088078, 16695888, 9160031, 16293483, 13839929, 14128572, 16416565, 10435342,
16498366, 5613296, 14072901, 184047, 9508770, -5265111, 3499586, -10144730,
-3041432, -13926969, -9120243, -16202698, -13813045, -16725742, -16406672, -15439523,
-16506968, -12483176, -14098690, -8176495, -9547828, -2985343, -3545976, 2528740,
2994758, 7769284, 9080378, 12169404, 13786047, 15253133, 16396644, 16686896,
16515433, 16315597, 14124362, 14179402, 9586805, 10509389, 3592335, 5702550,
-2948064, 278853, -9040444, -5175008, -13758943, -10069077, -16386489, -13873950,
-16523771, -16178046, -14149925, -16732126, -9625709, -15476252, -3638668, -12546276,
2901342, -8259142, 9000434, -3078596, 13731725, 2434969, 16376199, 7685138,
16531972, 12103986, 14175372, 15213518, 9664533, 16677369, 3684969, 16337190,
-2854601, 14229778, -8960356, 10583100, -13704402, 5791621, -16365782, 373650,
-16540045, -5084739, -14200708, -9993102, -9703283, -13820486, -3731244, -16152878,
2807833, -16737975, 8920203, -15512485, 13676964, -12608975, 16355230, -8341524,
16547982, -3171749, 14225928, 2341120, 9741952, 7600746, 3777485, 12038180,
-2761047, 15173416, -8879982, 16667309, -13649421, 16358261, -16344552, 14279700,
-16555791, 10656472, -14251037, 5880508, -9780546, 468436, -3823701, -4994308,
2714235, -9916807, 8839686, -13766580, 13621766, -16127192, 16333738, -16743288,
16563463, -15548223, 14276029, -12671271, 9819059, -8423639, 3869881, -3264802,
-2667405, 2247197, -8799324, 7516111, -13594005, 11971989, -16322798, 15132829,
-16571007, 16656716, -14300910, 16378808, -9857497, 14329164, -3916035, 10729504,
2620549, 5969206, 8758887, 563206, 13566131, -4903717, 16311724, -9840195,
16578415, -13712234, 14325674, -16100991, 9895853, -16748067, 3962153, -15583464,
-2573677, -12733162, -8718384, -8505485, -13538154, -3357750, -16300523, 2153201,
-16585694, 7431236, -14350326, 11905415, -9934133, 15091758, -4008244, 16645591,
2526780, 16398832, 8677807, 14378171, 13510063, 10802192, 16289188, 6057714,
16592836, 657958, 14374860, -4812969, 9972330, -9763268, 4054298, -13657450,
-2479867, -16074276, -8637165, -16752309, -13481869, -15618206, -16277727, -12794645,
-16599850, -8587059, -14399283, -3450590, -10010451, 2059137, -4100325, 7346123,
2432930, 11838460, 8596450, 15050204, 13453564, 16633933, 16266132, 16418331,
16606727, 14426718, 14423587, 10874535, 10048488, 6146027, 4146314, 752690,
-2385977, -4722067, -8555671, -9686029, -13425154, -13602229, -16254410, -16047046,
-16613475, -16756016, -14447780, -15652448, -10086448, -12855720, -4192274, -8668359,
2339002, -3543320, 8514818, 1965007, 13396633, 7260775, 16242555, 11771127,
16620086, 15008169, 14471853, 16621743, 10124325, 16437305, 4238197, 14474803,
-2292012, 10946530, -8473902, 6234145, -13368009, 847397, -16230574, -4631015,
-16626569, -9608480, -14495814, -13546573, -10162124, -16019303, -4284090, -16759187,
2244999, -15686191, 8432914, -12916383, 13339275, -8749381, 16218459, -3635937,
16632914, 1870814, 14519656, 7175195, 10199838, 11703417, 4329944, 14965654,
-2197973, 16609022, -8391863, 16455754, -13310437, 14522426, -16206218, 11018176,
-16639131, 6322062, -14543385, 942077, -10237474, -4539814, -4375768, -9530624,
2150925, -13490484, 8350741, -15991048, 13281490, -16761823, 16193844, -15719431,
16645211, -12976633, 14566994, -8830124, 10275024, -3728438, 4421553, 1776561,
-2103863, 7089386, -8309555, 11635334, -13252440, 14922661, -16181344, 16595770,
-16651161, 16473676, -14590491, 14569584, -10312497, 11089469, -4467307, 6409778,
2056781, 1036727, 8268300, -4448468, 13223280, -9452464, 16168711, -13433963,
16656974, -15962281, 14613867, -16763922, 10349883, -15752169, 4513021, -13036468,
-2009687, -8910584, -8226982, -3820820, -13194018, 1682252, -16155953, 7003349,
-16662659, 11566878, -14637130, 14879190, -10387190, 16581987, -4558703, 16491071,
1962572, 14616277, 8185594, 11160407, 13164647, 6497289, 16143061, 1131344,
16668206, -4356980, 14660272, -9374001, 10424410, -13377013, 4604344, -15933004,
-1915446, -16765485, -8144146, -15784403, -13135175, -13095887, -16130045, -8990759,
-16673624, -3913079, -14683301, 1587888, -10461551, 6917089, -4649953, 11498052,
1868301, 14835244, 8102628, 16567673, 13105594, 16507940, 16116895, 14662501,
16678904, 11230988, 14706209, 6584592, 10498604, 1225925, 4695520, -4265352,
-1821144, -9295238, -8061049, -13319635, -13075911, -15903217, -16103621, -16766512,
-16684055, -15816133, -14729003, -13154886, -10535577, -9070647, -4741054, -4005213,
1773969, 1493474, 8019402, 6830608, 13046121, 11428859, 16090214, 14790823,
16689069, 16552830, 14751675, 16524280, 10572462, 14708257, 4786546, 11301210,
-1726784, 6671684, -7977694, 1320466, -13016230, -4173589, -16076683, -9216178,
-16693953, -13261831, -14774234, -15872922, -10609267, -16767003, -4832004, -15847357,
1679581, -13213465, 7935919, -9150244, 12986231, -4097219, 16063018, 1399012,
16698700, 6743909, 14796670, 11359300, 10645983, 14745929, 4877419, 16537458,
-1632369, 16540092, -7894084, 14753543, -12956132, 11371071, -16049230, 6758563,
-16703318, 1414966, -14818992, -4081691, -10682617, -9136824, -4922799, -13203603,
1585139, -15842120, 7852183, -16766958, 12925926, -15878074, 16035309, -13271622,
16707798, -9229549, 14841192, -4189094, 10719162, 1304506, 4968136, 6656993,
-1537901, 11289378, -7810222, 14700564, -12895620, 16521557, -16021264, 16555375,
-16712148, 14798357, -14863276, 11440569, -10755626, 6845226, -5013437, 1509420,
1490647, -3989664, 7768195, -9057177, 12865207, -13144953, 16007087, -15810811,
16716361, -16766376, 14885238, -15908283, 10791999, -13329354, 5058694, -9308559,
-1443384, -4280835, -7726110, 1209957, -12834696, 6569865, -15992786, 11219095,
-16720444, 14654729, -14907085, 16505127, -10828290, 16570128, -5103914, 14842698,
1396106, 11509700, 7683960, 6931670, 12804077, 1603825, 15978353, -3897508,
16724389, -8977240, 14928809, -13085883, 10864491, -15778996, 5149090, -16765259,
-1348821, -15937983, -7641751, -13386660, -12773361, -9387271, -15963796, -4372439,
-16728205, 1115370, -14950418, 6482527, -10900609, 11148453, -5194228, 14608425,
1301521, 16488170, 7599478, 16584352, 12742538, 14886564, 15949108, 11578463,
16731883, 7017893, 14971903, 1698180, 10936636, -3805228, 5239321, -8897017,
-1254215, -13026394, -7557148, -15746676, -12711618, -16763605, -15934296, -15967174,
-16735431, -13443537, -14993272, -9465683, -10972579, -4463903, -5284377, 1020747,
1206895, 6394981, 7514754, 11077455, 12680591, 14561654, 15919352, 16470686,
16738842, 16598046, 15014518, 14929954, 11008430, 11646857, 5329385, 7103891,
-1159569, 1792480, -7472303, -3712826, -12649468, -8816509, -15904286, -12966488,
-16742122, -15713854, -15035647, -16761415, -11044198, -15995854, -5374356, -13499985,
1112230, -9543793, 7429789, -4555225, 12618239, 926092, 15889088, 6307231,
16745265, 11006102, 15056652, 14514417, 11079873, 16452674, 5419279, 16611208,
-1064886, 14972867, -7387219, 11714877, -12586913, 7189661, -15873768, 1886723,
-16748278, -3620306, -15077541, -8735719, -11115464, -12906168, -5464163, -15680528,
1017529, -16758690, 7344586, -16024023, 12555483, -13556001, 15858316, -9621597,
16751153, -4646401, 15098306, 831407, 11150961, 6219280, 5508999, 10934398,
-970168, 14466716, -7301899, 16434137, -12523957, 16623840, -15842742, 15015301,
-16753898, 11782523, -15118953, 7275202, -11186374, 1980906, -5553796, -3527669,
922796, -8654650, 7259149, -12845435, 12492326, -15646701, 15827037, -16755428,
16756505, -16051679, 15139476, -13611584, 11221693, -9699093, 5598544, -4737428,
-875420, 736695, -7216345, 6131129, -12460599, 10862344, -15811209, 14418553,
-16758982, 16415074, -15159882, 16635940, -11256927, 15057254, -5643251, 11849793,
828033, 7360511, 7173480, 2075025, 12428769, -3434920, 15795251, -8573304,
16761321, -12784292, 15180162, -15612374, 11292066, -16751631, 5687909, -16078823,
-780643, -13666732, -7130561, -9776280, -12396843, -4828304, -15779171, 641960,
-16763531, 6042782, -15200325, 10789942, -11327120, 14369928, -5732525, 16395486,
733244, 16647508, 7087581, 15098727, 12364815, 11916683, 15762961, 7445584,
16765602, 2169078, 15220363, -3342061, 11362078, -8491683, 5777092, -12722739,
-685842, -15577548, -7044548, -16747298, -12332691, -16105451, -15746629, -13721442,
-16767543, -9853154, -15240283, -4919026, -11396950, 547205, -5821617, 5954242,
638431, 10717195, 7001456, 14320844, 12300465, 16375374, 15730166, 16658543,
16769346, 15139716, 15260077, 11983193, 11431727, 7530419, 5866091, 2263061,
-591019, -3249096, -6958311, -8409792, -12268145, -12660780, -15713582, -15542223,
-16771019, -16742429, -15279753, -16131565, -11466416, -13775714, -5910522, -9929712,
543598, -5009590, 6915106, 452431, 12235722, 5865512, 15696869, 10644106,
16772554, 14271302, 15299303, 16354739, 11501010, 16669046, 5954902, 15180221,
-496176, 12049319, -6871850, 7615013, -12203206, 2356972, -15680034, -3156026,
-16773959, -8327631, -15318735, -12598416, -11535516, -15506402, -5999238, -16737025,
448747, -16157163, 6828536, -13829545, 12170588, -10005953, 15663069, -5099994,
16775226, 357644, 15338040, 5776594, 11569925, 10570676, 6043522, 14221303,
-401318, 16333580, -6785170, 16679016, -12137877, 15220241, -15645984, 12115059,
-16776363, 7699363, -15357227, 2450808, -11604246, -3062855, -6087762, -8245204,
353882, -12535649, 6741747, -15470084, 12105065, -16731086, 15628769, -16182245,
16777361, -13882934, 15376287, -10081875, 11638470, -5190235, 6131949, 262845,
-306448, 5687492, -6698274, 10496909, -12072160, 14170850, -15611433, 16311899,
-16778230, 16688452, -15395228, 15259774, -11672606, 12180413, -6176091, 7783467,
259006, 2544565, 6654743, -2969587, 12039154, -8162513, 15593969, -12472481,
16778960, -15433272, 15414041, -16724612, 11706643, -16206809, 6220180, -13935878,
-211567, -10157473, -6611163, -5280310, -12006056, 168037, -15576384, 5598207,
-16779560, 10422806, -15432736, 14119944, -11740592, 16289696, -6264223, 16697355,
164122, 15298819, 6567526, 12245376, 11972859, 7867322, 15558670, 2638241,
16780022, -2876224, 15451303, -8079562, 11774442, -12408915, 6308212, -15395967,
-116680, -16717602, -6523840, -16230854, -11939569, -13988378, -15540836, -10232747,
-16780354, -5370215, -15469751, 73225, -11808202, 5508744, -6352155, 10348369,
69233, 14068586, 6480099, 16266973, 11906181, 16705724, 15522874, 15337375,
16780548, 12309948, 15488071, 7950926, 11841864, 2731832, 6396041, -2782770,
-21791, -7996354, -6436311, -12344953, -11872702, -15358170, -15504792, -16710060,
-16780612, -16254382, -15506272, -14040431, -11875438, -10307696, -6439886, -5459953,
-25662, -21593, 6392462, 5419102, 11839120, 10273600, 15486580, 14016778,
16780537, 16243729, 15524344, 16713558, 11908909, 15375441, 6483670, 12374128,
73104, 8034277, -6348571, 2825338, -11805451, -2689223, -15468250, -7912885,
-16780332, -12280592, -15542296, -15319879, -11942290, -16701981, -6527407, -16277388,
-120551, -14092033, 6304625, -10382311, 11771683, -5549510, 15449793, -116406,
16779989, 5329291, 15560120, 10198505, 11975571, 13964522, 6571088, 16219966,
167993, 16720859, -6260632, 15413015, -11737825, 12437911, -15431215, 8117369,
-16779516, 2918752, -15577823, -2595593, -12008761, -7829167, -6614721, -12215842,
-215438, -15281101, 6216584, -16693369, 11703869, -16299874, 15412511, -14143185,
16778905, -10456596, 15595398, -5638892, 12041851, -211216, 6658297, 5239307,
262877, 10123083, -6172492, 13911820, -11669824, 16195684, -15393687, 16727624,
-16778164, 15450096, -15612852, 12501296, -12074848, 8200202, -6701823, 3012072,
-310318, -2501879, 6128345, -7745198, 11635681, -12150700, 15374736, -15241833,
16777285, -16684223, 15630178, -16321839, 12107745, -14193885, 6745292, -10530547,
357753, -5728094, -6084154, -306020, -11601449, 5149157, -15355666, 10047337,
-16776275, 13858673, -15647383, 16170885, -12140550, 16733855, -6788711, 15486683,
-405189, 12564282, 6039910, 8282773, 11567121, 3105296, 15336469, -2408086,
16775127, -7660981, 15664458, -12085170, 12173253, -15202078, 6832072, -16674544,
452617, -16343283, -5995622, -14244132, -11532704, -10604161, -15317154, -5817113,
-16773850, -400814, -15681412, 5058841, -12205863, 9971270, -6875382, 13805082,
-500046, 16145568, 5951282, 16739550, 11498190, 15522775, 15297713, 12626865,
16772434, 8365079, 15698237, 3198421, 12238371, -2314216, 6918633, -7576520,
547467, -12019254, -5906898, -15161837, -11463589, -16664331, -15278153, -16364203,
-16770888, -14293922, -15714940, -10677436, -12270786, -5905946, -6961833, -495596,
-594888, 4968364, 5862463, 9894884, 11428892, 13751050, 15258466, 16119735,
16769204, 16744711, 15731514, 15558370, 12303098, 12689045, 7004974, 8447117,
642299, 3291444, -5817985, -2220271, -11394108, -7491816, -15238662, -11952953,
-16767390, -15121111, -15747966, -16653585, -12335316, -16384601, -7048062, -14343256,
-689710, -10750369, 5773457, -5994590, 11359229, -590362, 15218732, 4877728,
16765438, 9818181, 15764288, 13696579, 12367432, 16093386, 7091090, 16749335,
737111, 15593468, -5728887, 12750820, -11324263, 8528886, -15198685, 3384362,
-16763356, -2126256, -15780488, -7406872, -12399452, -11886270, -7134065, -15079902,
-784511, -16642307, 5684266, -16404474, 11289202, -14392131, 15178512, -10822959,
16761135, -6083042, 15796558, -685108, 12431370, 4786935, 7176979, 9741165,
831900, 13641669, -5639605, 16066523, -11254055, 16753425, -15158221, 15628067,
-16758785, 12812186, -15812506, 8610382, -12463192, 3477171, -7219840, -2032172,
-879286, -7321692, 5594894, -11819206, 11218815, -15038210, 15137806, -16630497,
16756297, -16423823, 15828323, -14440546, 12494910, -10895203, 7262639, -6171300,
926661, -779833, -5550142, 4695990, -11183488, 9663837, -15117273, 13586323,
-16753679, 16039146, -15844018, 16756978, -12526533, 15662166, -7305385, 12873143,
-974033, 8691602, 5505342, 3569869, 11148068, -1938024, 15096616, -7236277,
16750923, -11751765, 15859582, -14996037, 12558051, -16618155, 7348067, -16442646,
1021393, -14488499, -5460502, -10967098, -11112563, -6259361, -15075841, -874533,
-16748037, 4604895, -15875023, 9586200, -12589473, 13530543, -7390695, 16011256,
-1068749, 16759996, 5415615, 15695765, 11076965, 12933688, 15054943, 8772544,
16745013, 3662453, 15890333, -1843813, 12620791, -7150631, 7433260, -11683948,
1116093, -14953385, -5370688, -16605282, -11041283, -16460944, -15033928, -14535989,
-16741860, -11038643, -15905520, -6347221, -12652011, -969205, -7475769, 4513652,
-1163431, 9508257, 5325714, 13474330, 11005508, 15982854, 15012788, 16762477,
16738568, 15728862, 15920576, 12993819, 12683127, 8853206, 7518215, 3754920,
1210756, -1749544, -5280702, -7064756, -10969649, -11615758, -14991533, -14910255,
-16735147, -16591877, -15935509, -16478716, -12714145, -14583014, -7560605, -11109834,
-1258076, -6434878, 5235643, -1063846, 10933699, 4422265, 14970154, 9430009,
16731588, 13417686, 15950311, 15953942, 12745057, 16764423, 7602930, 15761455,
1305381, 13053536, -5190547, 8933585, -10897665, 3847266, -14948659, -1655219,
-16727899, -6978656, -15964989, -11547196, -12775871, -14866648, -7645198, -16577942,
-1352680, -16495960, 5145405, -14629572, 10861540, -11180670, 14927041, -6522330,
16724072, -1158453, 15979535, 4330737, 12806579, 9351460, 7687401, 13360613,
1399964, 15924518, -5100226, 16765832, -10825333, 15793545, -14905307, 13112834,
-16720116, 9013678, -15993957, 3939490, -12837189, -1560840, -7729547, -6892332,
-1447241, -11478264, 5055002, -14822565, 10789034, -16563477, 14883451, -16512677,
16716022, -14675663, 16008248, -11251149, 12867693, -6609572, 7771627, -1253022,
1494503, 4239070, -5009742, 9272612, -10752654, 13303113, -14861479, 15894586,
-16711798, 16766706, -16022415, 15825130, -12898097, 13171713, -7813649, 9093483,
-1541756, 4031588, 4964438, -1466412, 10716183, -6805788, 14839384, -11408966,
16707437, -14778009, 16036449, -16548482, 12928394, -16528866, 7855604, -14721284,
1588993, -11321268, -4919098, -6696604, -10679631, -1347552, -14817175, 4147267,
-16702946, 9193467, -16050360, 13245187, -12958592, 15864146, -7897501, 16767043,
-1636222, 15856208, 4873714, 13230171, 10642990, 9172997, 14794844, 4123557,
16698318, -1371937, 16064138, -6719026, 12988682, -11339303, 7939330, -14732980,
1683433, -16532959, -4828296, -16544526, -10606267, -14766435, -14772398, -11391025,
-16693560, -6783421, -16077792, -1442039, -13018673, 4055332, -7981100, 9114029,
-1730635, 13186838, 4782835, 15833198, 10569456, 16766844, 14749830, 15886780,
16688665, 13288206, 16091313, 9252218, 13048555, 4215393, 8022802, -1277418,
1777819, -6632050, -4737340, -11269277, -10532564, -14687480, -14727148, -16516906,
-16683640, -16559658, -16104709, -14811113, -13078337, -11460418, -8064444, -6870022,
-1824993, -1536479, 4691803, 3963268, 10495584, 9034299, 14704344, 13128068,
16678478, 15801744, 16117973, 16766109, 13108011, 15916844, 8106018, 13345817,
1872148, 9331142, -4646233, 4307096, -10458524, -1182859, -14681427, -6544861,
-16673187, -11198891, -16131112, -14641510, -13137584, -16500325, -8147530, -16574260,
-1919292, -14855318, 4600621, -11529444, 10421376, -6956403, 14658388, -1630871,
16667758, 3871076, 16144118, 8954280, 13167048, 13068877, 8188974, 15769784,
1966417, 16764838, -4554977, 15946398, -10384149, 13403000, -14635236, 9409769,
-16662200, 4398660, -16156999, -1088261, -13196410, -6457463, -8230356, -11128147,
-2013530, -14595072, 4509292, -16483217, 10346835, -16588331, 14611964, -14899048,
16656505, -11598101, 16169747, -7042561, 13225663, -1725210, 8271668, 3778761,
2060624, 8873975, -4463575, 13009269, -10309442, 15737320, -14588578, 16763030,
-16650681, 15975443, -16182369, 13459755, -13254814, 9488094, -8312919, 4490084,
-2107704, -993629, 4417819, -6369859, 10271963, -11057047, 14565072, -14548168,
16644719, -16465581, 16194858, -16601873, 13283856, -14942302, 8354098, -11666388,
2154764, -7128494, -4372031, -1819494, -10234406, 3686325, -14541453, 8793386,
-16638629, 12949244, -16207222, 15704353, -13312795, 16760687, -8395215, 16003977,
-2201811, 13516079, 4326204, 9566117, 10196763, 4581364, 14517714, -898965,
16632401, -6282051, 16219452, -10985593, 13341623, -14500798, 8436261, -16447419,
2248836, -16614883, -4280346, -14985077, -10159043, -11734302, -14493863, -7214199,
-16626045, -1913720, -16231556, 3593771, -13370349, 8712515, -8477244, 12888805,
-2295847, 15670884, 4234451, 16757808, 10121237, 16031999, 14469893, 13571971,
16619552, 9643833, 16243527, 4672498, 13398964, -804272, 8518154, -6194042,
2342835, -10913788, -4188525, -14452964, -10083354, -16428731, -14445810, -16627363,
-16612929, -15027374, -16255372, -11801840, -13427476, -7299674, -8559001, -2007885,
-2389809, 3501102, 4142562, 8631367, 10045387, 12827955, 14421608, 15636914,
16606170, 16754392, 16267082, 16059508, 13455877, 13627429, 8599775, 9721241,
2436760, 4763482, -4096570, -709553, -10007343, -6105835, -14397295, -10841634,
-16599282, -14404668,
};

int unit_test_asrc_stereo(int test_case) {
    int input_frequency;
    int output_frequency;
    int *expected_wave;
    int steps = 100;
    if (test_case == 8) {
        input_frequency = 96000;
        output_frequency = 48000;
        expected_wave = (int *)expected_wave_stereo_96_48;   // 0 FILTER_DEFS_ASRC_FIR_BL_ID
        steps = 200;
    } else if (test_case == 9) {
        input_frequency = 176400;
        output_frequency = 176400;
        expected_wave = (int *)expected_wave_stereo_176_176;
        steps = 5000;
    } else {
        printf("Bad test case %d\n", test_case);
        return 1;
    }

    asrc_state_t sASRCState[LOCAL_SRC_CHANNELS_PER_INSTANCE];                                   // ASRC state machine state
    int iASRCStack[LOCAL_SRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES * 100]; // Buffer between filter stages
    asrc_ctrl_t sASRCCtrl[LOCAL_SRC_CHANNELS_PER_INSTANCE];                                     // Control structure
    asrc_adfir_coefs_t asrc_adfir_coefs;                                                  // Adaptive filter coefficients

    uint64_t fs_ratio = 0;
    int sine_cnt = 0;
    int sine2_cnt = 0;

    int inputFsCode = fs_code(input_frequency);
    int outputFsCode = fs_code(output_frequency);

    for(int ui = 0; ui < LOCAL_SRC_CHANNELS_PER_INSTANCE; ui++)
    {
            // Set state, stack and coefs into ctrl structure
            sASRCCtrl[ui].psState                   = &sASRCState[ui];
            sASRCCtrl[ui].piStack                   = iASRCStack[ui];
            sASRCCtrl[ui].piADCoefs                 = asrc_adfir_coefs.iASRCADFIRCoefs;
    }

    fs_ratio = asrc_init(inputFsCode, outputFsCode, sASRCCtrl, LOCAL_SRC_CHANNELS_PER_INSTANCE, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);
    fs_ratio *= 0.9991;
    int in_samples[4][LOCAL_SRC_CHANNELS_PER_INSTANCE];
    int out_samples[20][LOCAL_SRC_CHANNELS_PER_INSTANCE];
    int exp_cnt = 0;
    int errors = 0;
    for(int32_t i = 0; i < steps; i+=4) {
        for(int j = 0; j < 4; j++) {
            in_samples[j][0] = sine_wave[sine_cnt];
            sine_cnt++;
            if (sine_cnt == 16) sine_cnt = 0;
            in_samples[j][1] = sine2_wave[sine2_cnt];
            sine2_cnt++;
            if (sine2_cnt == 19) sine2_cnt = 0;
        }
        int num_samples = asrc_process((int *)in_samples, (int *) out_samples, fs_ratio, sASRCCtrl);
        for(int j = 0; j < num_samples; j++) {
            for(int k = 0; k < 2; k++) {
                int idx = exp_cnt*LOCAL_SRC_CHANNELS_PER_INSTANCE+k;
                if (abs(out_samples[j][k] - expected_wave[idx]) > 8) {
                    printf("saw %d expected %d\n", out_samples[j][k], expected_wave[idx]);
                    errors++;
                }
            }
            exp_cnt++;
        }
    }
    printf("%d -> %d: %d errors\n", input_frequency, output_frequency, errors);
    return errors;
}

#define ONE  0x1000000

int unit_test_fir() {
    int        DWORD_ALIGNED piData[64]  = {
        1*ONE, 2*ONE, 4*ONE, 8*ONE, -1*ONE, -8*ONE, -4*ONE, -9*ONE+6,
        1*ONE, 2*ONE, 4*ONE, 8*ONE, -4*ONE, -8*ONE, -4*ONE+3, -9*ONE,
        1*ONE, 2*ONE, 4*ONE, 8*ONE, -1*ONE, -1*ONE-12345, -4*ONE, -9*ONE,
        1*ONE, 2*ONE, 4*ONE, 8*ONE, -1*ONE, -8*ONE, -5*ONE, -9*ONE,
        4*ONE, 2*ONE, 4*ONE, 8*ONE, -1*ONE, -8*ONE, -4*ONE, -9*ONE,
        1*ONE, -3*ONE, 4*ONE, 8*ONE, -4*ONE, -8*ONE, -4*ONE, -9*ONE,
        1*ONE, 2*ONE, 1*ONE, 8*ONE, -1*ONE, -1*ONE, -4*ONE, -9*ONE,
        1*ONE, 2*ONE, 4*ONE, -7*ONE, -1*ONE, -8*ONE, -5*ONE, -9*ONE,
    };
    int        DWORD_ALIGNED piCoefs[64] = {
        ONE, 2*ONE,
        -1*ONE, -3*ONE,
        5*ONE, 7*ONE,
        8*ONE, -7*ONE,
        ONE, 2*ONE,
        -1*ONE, -3*ONE,
        5*ONE, 7*ONE,
        8*ONE, -9*ONE,

        ONE, 5*ONE,
        -5*ONE, -3*ONE,
        5*ONE, 7*ONE,
        8*ONE, -7*ONE,
        ONE, 2*ONE,
        -1*ONE, -3*ONE,
        5*ONE, 7*ONE,
        8*ONE, -8*ONE,

        ONE, 5*ONE,
        -5*ONE, -3*ONE,
        5*ONE, 7*ONE,
        8*ONE, -7*ONE,
        ONE, 2*ONE,
        -1*ONE, -3*ONE,
        6*ONE, 1*ONE,
        9*ONE, -8*ONE,
    
        ONE, 5*ONE,
        -5*ONE, -3*ONE,
        5*ONE, 7*ONE,
        2*ONE, -7*ONE,
        ONE, 2*ONE,
        -1*ONE, -3*ONE,
        5*ONE, 7*ONE,
        8*ONE, -1*ONE
    };
    int        DWORD_ALIGNED piCoefs2[64] = {
        ONE, -1*ONE, 5*ONE, 8*ONE, ONE, -1*ONE, 5*ONE, 8*ONE,
        2*ONE, -3*ONE, 7*ONE, -7*ONE, 2*ONE, -3*ONE, 7*ONE,  -9*ONE,
        ONE, -5*ONE, 5*ONE, 8*ONE, ONE, -1*ONE, 5*ONE, 8*ONE,
        5*ONE, -3*ONE, 7*ONE, -7*ONE, 2*ONE, -3*ONE, 7*ONE,  -8*ONE,
        ONE, -5*ONE, 5*ONE, 8*ONE, ONE, -1*ONE, 6*ONE, 9*ONE,
        5*ONE, -3*ONE, 7*ONE, -7*ONE, 2*ONE, -3*ONE, 1*ONE,  -8*ONE,
        ONE, -5*ONE, 5*ONE, 2*ONE, ONE, -1*ONE, 5*ONE, 8*ONE,
        5*ONE, -3*ONE, 7*ONE, -7*ONE, 2*ONE, -3*ONE, 7*ONE,  -1*ONE,
    };
    for(int i = 0; i < 64; i++) {
        int o = (i&1) * 8 + (i&~15) + (i&14)/2;
//        printf("%08x  %08x\n", piCoefs
        piCoefs2[o] = piCoefs[i];
    }
    int        DWORD_ALIGNED        iData1[2];
    int        DWORD_ALIGNED        iData2[2];

    iData1[0] = 0x1234;
    iData1[1] = 0x2234;
    iData2[0] = 0x3234;
    iData2[1] = 0x4234;
    int t0, t1, t2, t3;
    asm volatile("gettime %0" : "=r" (t0));
    src_mrhf_fir_os_inner_loop_asm(piData, piCoefs, iData1, 16);
    asm volatile("gettime %0" : "=r" (t1));
    printf("Time old %d\n", t1-t0);
    printf("%08x %08x\n", iData1[0], iData1[1]);
    asm volatile("gettime %0" : "=r" (t2));
    src_mrhf_fir_os_inner_loop_asm_xs3(piData, piCoefs2, iData2, 16);
    asm volatile("gettime %0" : "=r" (t3));
    printf("Time new %d\n", t3-t2);
    printf("%08x %08x\n", iData2[0], iData2[1]);

    asm volatile("gettime %0" : "=r" (t0));
    src_mrhf_fir_inner_loop_asm(piData, piCoefs, iData1, 32);
    asm volatile("gettime %0" : "=r" (t1));
    printf("Time old %d\n", t1-t0);
    printf("%08x \n", iData1[0]);
    asm volatile("gettime %0" : "=r" (t2));
    src_mrhf_fir_inner_loop_asm_xs3(piData, piCoefs, iData2, 32);
    asm volatile("gettime %0" : "=r" (t3));
    printf("Time new %d\n", t3-t2);
    printf("%08x \n", iData2[0]);

    asm volatile("gettime %0" : "=r" (t0));
    src_mrhf_adfir_inner_loop_asm(piData, piCoefs, iData1, 32);
    asm volatile("gettime %0" : "=r" (t1));
    printf("Time old %d\n", t1-t0);
    printf("%08x \n", iData1[0]);
    asm volatile("gettime %0" : "=r" (t2));
    src_mrhf_adfir_inner_loop_asm_xs3(piData, piCoefs, iData2, 32);
    asm volatile("gettime %0" : "=r" (t3));
    printf("Time new %d\n", t3-t2);
    printf("%08x \n", iData2[0]);
    return 0;
}

int run_all_combinations(int pos_deviation, int neg_deviation) {
    int errors = 0;
    hwtimer_free_xc_timer();
    clear_energies();
    errors += test_44100_low(pos_deviation, neg_deviation);
    errors += test_48000_low(pos_deviation, neg_deviation);
    errors += test_4xx00_high(pos_deviation, neg_deviation);
    errors += test_88200_low(pos_deviation, neg_deviation);
    errors += test_96000_low(pos_deviation, neg_deviation);
    errors += test_9xx00_high(pos_deviation, neg_deviation);
    errors += test_176400_low(pos_deviation, neg_deviation);
    errors += test_192000_low(pos_deviation, neg_deviation);
    errors += test_1xxx00_high(pos_deviation, neg_deviation);
    errors += verify_energy_limits();
    hwtimer_realloc_xc_timer();
    return errors;
}

int main(void) {
    int errors = 0;
//    errors += unit_test_spline();
//    errors += unit_test_fir();
//    errors += unit_test_asrc(0);
//    errors += unit_test_asrc(1);
//    errors += unit_test_asrc(2);
//    errors += unit_test_asrc(3);
//    errors += unit_test_asrc(4);
//    errors += unit_test_asrc(5);
//    errors += unit_test_asrc(6);
//    errors += unit_test_asrc(7);
//    errors += unit_test_asrc(8);
//    errors += unit_test_asrc_stereo(8);
//    errors += unit_test_asrc_stereo(9);
//    return errors;

//    test_async(48000, 192000, 1, &errors, -6, 12);
    test_async(48000, 192000, 1, &errors, 12, -6);
//    test_async(192000, 192000, 1, &errors, 12, -6);
    return errors;
//    errors += run_all_combinations(+12, -6);
    errors += run_all_combinations(-6, +12);
    if (errors == 0) {
        printf("PASS\n");
    } else {
        printf("FAIL: %d errors\n", errors);
    }

}


