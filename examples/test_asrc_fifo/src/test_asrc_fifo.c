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


#define POSITIVE_DEVIATION(F)    ((F) + ((F)*12)/48000)
#define NEGATIVE_DEVIATION(F)    ((F) - ((F)* 8)/48000)

extern int async_resets;

DECLARE_JOB(producer,   (asynchronous_fifo_t *, int, int, int, int *));
DECLARE_JOB(consumer,   (asynchronous_fifo_t *, int, int, int *));
DECLARE_JOB(test_async, (int, int, int, int *));

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


static int interpolation_ticks_2D[6][6] = {
    {  2268, 2268, 2268, 2268, 2268, 2268},
    {  2083, 2083, 2083, 2083, 2083, 2083},
    {  2268, 2268, 1134, 1134, 1134, 1134},
    {  2083, 2083, 1042, 1042, 1042, 1042},
    {  2268, 2268, 1134, 1134,  567,  567},
    {  2083, 2083, 1042, 1042,  521,  521}
};

void producer(asynchronous_fifo_t *a, int input_frequency, int output_frequency, int xscope_used, int *errors) {
    int interpolation_ticks = interpolation_ticks_2D[fs_code(input_frequency)][fs_code(output_frequency)];
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
    ideal_fs_ratio = (fs_ratio + (1<<31)) >> 32;

    int in_samples[4];
    int out_samples[25];
    int freq1 = NEGATIVE_DEVIATION(output_frequency);
    int freq2 = (POSITIVE_DEVIATION(input_frequency)/4)*4;
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
    if (ms > 3500 && ms < 5300) {
        *errors = 0;
    } else {
        printf("Failure: ratio %08x low %08x high %08x\n", (int)(fs_ratio >> 32), expected_ratio_low, expected_ratio_high);
        printf("         stable counter %d from %d ms input %6d output %6d total resets %d\n",
               good_error_count, ms, input_frequency, output_frequency, async_resets);
        *errors = 1;
    }
}

void consumer(asynchronous_fifo_t *a, int output_frequency, int xscope_used, int *limits) {
    hwtimer_t tmr = hwtimer_alloc();
    uint64_t now = hwtimer_get_time(tmr);
    int freq = POSITIVE_DEVIATION(output_frequency);
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
            freq = NEGATIVE_DEVIATION(output_frequency);
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
                int *errors) {
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_LENGTH, 1)];
    asynchronous_fifo_t *asynchronous_fifo_state = (asynchronous_fifo_t *)array;

    *errors = 1;
    asynchronous_fifo_init(asynchronous_fifo_state, 1, FIFO_LENGTH);
    asynchronous_fifo_init_PID_fs_codes(asynchronous_fifo_state,
                                        fs_code(input_frequency),
                                        fs_code(output_frequency));

    PAR_JOBS(
        PJOB(producer, (asynchronous_fifo_state, input_frequency, output_frequency, xscope_used, errors)),
        PJOB(consumer, (asynchronous_fifo_state, output_frequency, xscope_used,
                        energies[fs_code(input_frequency)][fs_code(output_frequency)]))
        );
    asynchronous_fifo_exit(asynchronous_fifo_state);

}

int test_44100_low() {
    int e0, e1, e2, e3;
    printf("Testing 44100 low\n");
    PAR_JOBS(
        PJOB(test_async, (44100, 44100, 0, &e0)), // OK, 4 sec or so.
        PJOB(test_async, (44100, 48000, 0, &e1)), // OK, 4 sec or so. Slight overshoot
        PJOB(test_async, (44100, 88200, 0, &e2)), // OK, 4 sec or so.
        PJOB(test_async, (44100, 96000, 0, &e3)) // OK  4 sec or so.
        );
    return e0 + e1 + e2 + e3;
}

int test_48000_low() {
    int e0, e1, e2, e3;
    printf("Testing 48000 low\n");
    PAR_JOBS(
        PJOB(test_async, (48000, 44100, 0, &e0)), // OK, 4 sec or so
        PJOB(test_async, (48000, 48000, 0, &e1)), // OK  4.16 s stable slight overshoot
        PJOB(test_async, (48000, 88200, 0, &e2)), // OK, 4 sec or so
        PJOB(test_async, (48000, 96000, 0, &e3)) // OK  4.16 s stable
        );
    return e0 + e1 + e2 + e3;
}

int test_4xx00_high() {
    int e0, e1, e2, e3;
    printf("Testing 44100/48000 high\n");
    PAR_JOBS(
        PJOB(test_async, (44100, 176400, 0, &e2)), // OK, 4 sec or so.
        PJOB(test_async, (44100, 192000, 0, &e3)), // OK  4 sec or so.
        PJOB(test_async, (48000, 176400, 0, &e0)), // OK, 4 sec or so
        PJOB(test_async, (48000, 192000, 0, &e1)) // OK  4.16 s stable
        );
    return e0 + e1 + e2 + e3;
}

int test_88200_low() {
    int e0, e1, e2, e3;
    printf("Testing 88200 low\n");
    PAR_JOBS(
        PJOB(test_async, (88200, 44100, 0, &e0)), // OK, 4 sec or so
        PJOB(test_async, (88200, 48000, 0, &e1)), // OK, 4 sec or so. Slight overshoot, phase error not zero at the end?
        PJOB(test_async, (88200, 88200, 0, &e2)), // OK, 4 sec or so. Slight overshoot
        PJOB(test_async, (88200, 96000, 0, &e3))  // OK  4 sec or so. Slight overshoot
        );
    return e0 + e1 + e2 + e3;
}

int test_96000_low() {
    int e0, e1, e2, e3;
    printf("Testing 96000 low\n");
    PAR_JOBS(
        PJOB(test_async, (96000, 44100, 0, &e0)), // OK, 4 sec or so. Non zero phase difference
        PJOB(test_async, (96000, 48000, 0, &e1)), // OK  4.16 s stable slight overshoot
        PJOB(test_async, (96000, 88200, 0, &e2)), // OK, 4 sec or so. Phase error not zero
        PJOB(test_async, (96000, 96000, 0, &e3)) // OK  4.14 s stable
        );
    return e0 + e1 + e2 + e3;
}

int test_9xx00_high() {
    int e0, e1, e2, e3;
    printf("Testing 44100/48000 high\n");
    PAR_JOBS(
        PJOB(test_async, (88200, 176400, 0, &e2)), // OK, 4 sec or so. Slight overshoot
        PJOB(test_async, (88200, 192000, 0, &e3)), // OK  4 sec or so. Slight overshoot
        PJOB(test_async, (96000, 176400, 0, &e0)), // OK, 4 sec or so. Phase error not zero
        PJOB(test_async, (96000, 192000, 0, &e1)) // OK  4.14 s stable
        );
    return e0 + e1 + e2 + e3;
}

int test_176400_low() {
    int e0, e1, e2, e3;
    printf("Testing 176400 low\n");
    PAR_JOBS(
        PJOB(test_async, (176400, 44100, 0, &e0)),
        PJOB(test_async, (176400, 48000, 0, &e1)),
        PJOB(test_async, (176400, 88200, 0, &e2)),
        PJOB(test_async, (176400, 96000, 0, &e3)) // phase != 0?
        );
    return e0 + e1 + e2 + e3;
}

int test_192000_low() {
    int e0, e1, e2, e3;
    printf("Testing 192000 low\n");
    PAR_JOBS(
        PJOB(test_async, (192000, 44100, 0, &e0)),
        PJOB(test_async, (192000, 48000, 0, &e1)),
        PJOB(test_async, (192000, 88200, 0, &e2)),
        PJOB(test_async, (192000, 96000, 0, &e3)) 
        );
    return e0 + e1 + e2 + e3;
}

int test_1xxx00_high() {
    int e0, e1, e2, e3;
    printf("Testing 176400/192000 high\n");
    PAR_JOBS(
        PJOB(test_async, (176400, 176400, 0, &e2)),
        PJOB(test_async, (176400, 192000, 0, &e3)),
        PJOB(test_async, (192000, 176400, 0, &e0)),
        PJOB(test_async, (192000, 192000, 0, &e1))
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
//    return errors;
    hwtimer_free_xc_timer();
    clear_energies();
    errors += test_44100_low();
    errors += test_48000_low();
    errors += test_4xx00_high();
    errors += test_88200_low();
    errors += test_96000_low();
    errors += test_9xx00_high();
    errors += test_176400_low();
    errors += test_192000_low();
    errors += test_1xxx00_high();
    errors += verify_energy_limits();
    if (errors == 0) {
        printf("PASS\n");
    } else {
        printf("FAIL: %d errors\n", errors);
    }

    hwtimer_realloc_xc_timer();
}


