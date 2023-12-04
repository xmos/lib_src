#include <string.h>
#include "asrc_block.h"

#include <xcore/chanend.h>
#include <platform.h>
#include <xs1.h>
#include <print.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src.h"

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

void asrc_init_buffer(asrc_block_t *asrc_data_buffer, int channel_count, int max_fifo_depth) {
    asrc_data_buffer->sample_valid = 0;
    asrc_data_buffer->reset = 0;
    asrc_data_buffer->max_fifo_depth = max_fifo_depth;
    asrc_data_buffer->write_ptr = max_fifo_depth/2;
    asrc_data_buffer->read_ptr = 0;
    asrc_data_buffer->last_phase_error = 0;
    asrc_data_buffer->last_timestamp = 0;
    asrc_data_buffer->output_sample_number = 0;
    asrc_data_buffer->converted_sample_number = 0;
    asrc_data_buffer->marked_converted_sample_number = 0;
    asrc_data_buffer->marked_converted_sample_time = 0;
    asrc_data_buffer->channel_count = channel_count;
    asrc_data_buffer->fs_ratio = 0x1000000000000000ULL;
    memset(asrc_data_buffer->buffer, 0, channel_count * max_fifo_depth * sizeof(int));
#ifdef REAL_ASRC
    int inputFsCode = FS_CODE_48;
    int outputFsCode = FS_CODE_48;

    for(int ui = 0; ui < SRC_CHANNELS_PER_INSTANCE; ui++)
    {
            // Set state, stack and coefs into ctrl structure
            sASRCCtrl[ui].psState                   = &sASRCState[ui];
            sASRCCtrl[ui].piStack                   = iASRCStack[ui];
            sASRCCtrl[ui].piADCoefs                 = asrc_adfir_coefs.iASRCADFIRCoefs;
    }

    asrc_data_buffer->fs_ratio = asrc_init(inputFsCode, outputFsCode, sASRCCtrl, SRC_CHANNELS_PER_INSTANCE, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);
    printf("%08llx\n", asrc_data_buffer->fs_ratio);
#endif
    asrc_data_buffer->fractional_credit = 0;
}

void asrc_exit_buffer(asrc_block_t *asrc_data_buffer) {
}
    
void asrc_add_quad_input(asrc_block_t *asrc_data_buffer, int32_t *samples) {
    int timestamp;
    asm volatile("gettime %0" : "=r" (timestamp));
    int outputBuff[SRC_OUT_BUFF_SIZE];
    int sampsOut = 0;

//    asrc_data_buffer->fs_ratio = 0x1000000000000000ULL; // todo: delete

#ifdef REAL_ASRC
    /* Run ASRC from input buffer into output buffer */
    sampsOut = asrc_process(samples, outputBuff, asrc_data_buffer->fs_ratio, sASRCCtrl);
#else
    static uint64_t remn = 0;
    for(int i = 0; i < 4; i++) {
        remn += 0x1000000000000000ULL;
        if (remn >= asrc_data_buffer->fs_ratio) {
            outputBuff[sampsOut++] = samples[i];
            remn -= asrc_data_buffer->fs_ratio;
        }
    }
#endif

    int read_ptr = asrc_data_buffer->read_ptr;
    int write_ptr = asrc_data_buffer->write_ptr;
    int max_fifo_depth = asrc_data_buffer->max_fifo_depth;
    int channel_count = asrc_data_buffer->channel_count;
    int len = (write_ptr - read_ptr + max_fifo_depth) % max_fifo_depth;
    if ( asrc_data_buffer->reset || len >= max_fifo_depth - 6) {
        printf("@@@@@@@@@@@@ RESET @@@@@@@@@@ %lu len %d\n", asrc_data_buffer->reset, len );    
        asrc_data_buffer->write_ptr = (read_ptr + asrc_data_buffer->max_fifo_depth/2) % asrc_data_buffer->max_fifo_depth;
        // TODO: reset all ASRC values, indices, etc.
        asrc_data_buffer->reset= 0;
    } else  {
        for(int i = 0; i < sampsOut; i++) {
            int size = channel_count * sizeof(int);
            memcpy(asrc_data_buffer->buffer + write_ptr * channel_count, outputBuff + i, size);
            write_ptr = (write_ptr + 1) % asrc_data_buffer->max_fifo_depth;
        }
            int ideal_freq_input = 48000;
            int ideal_freq_output = 48000;
        asrc_data_buffer->write_ptr = write_ptr;
        // fractional_credit is the number of samples that are unprocessed in the input.
        asrc_data_buffer->fractional_credit += 0x4000000000000000LL;
        while (asrc_data_buffer->fractional_credit >= asrc_data_buffer->fs_ratio) {
            asrc_data_buffer->fractional_credit -= asrc_data_buffer->fs_ratio;
            asrc_data_buffer->converted_sample_number += 1;
        }
        int converted_sample_time = timestamp + (((0x1000000000000000LL - asrc_data_buffer->fractional_credit) >> 32) * 100000000/ideal_freq_input) / 0x10000000;
        
        if (asrc_data_buffer->sample_valid) {
            int32_t phase_error = asrc_data_buffer->sample_timestamp - asrc_data_buffer->marked_converted_sample_time;
            phase_error -= ((asrc_data_buffer->sample_number - asrc_data_buffer->marked_converted_sample_number) * 100000000LL / ideal_freq_input);
            // Now that we have a phase error, calculate the proportional error
            // and use that and the integral error to correct the ASRC factor
            int32_t diff_error = phase_error - asrc_data_buffer->last_phase_error;
            static int printcnt = 0;
            printcnt++;
            if (printcnt > 10) {
                printint((asrc_data_buffer->sample_number - asrc_data_buffer->marked_converted_sample_number));printchar(' ');
                printint(phase_error); printchar(' ');
                printint(diff_error); printchar(' ');
                printintln(len);
                printcnt = 0;
            }
            int32_t diff_time = timestamp - asrc_data_buffer->last_timestamp;
            if (diff_time == 0) diff_time = 0x7fffffff;

            // TODO: set SPEEDUP to 1 and measure
            float SPEEDUP = 4;
            float Kp = 0.0001 * SPEEDUP;
            int64_t Kp_i = Kp * 0x1000000000000000LL;
            float Ki = 0.000025 * 1e-8 * SPEEDUP;
            int64_t Ki_i = Ki * 0x1000000000000000LL;
            // TODO: verify that this won't overflow in an ugly way with various clock frequencies
            asrc_data_buffer->fs_ratio +=
                diff_error * Kp_i / diff_time +
                phase_error * Ki_i;
            asrc_data_buffer->last_phase_error = phase_error;
            asrc_data_buffer->last_timestamp = timestamp;

            asrc_data_buffer->marked_converted_sample_number = asrc_data_buffer->converted_sample_number;
            asrc_data_buffer->marked_converted_sample_time   = converted_sample_time;
            asrc_data_buffer->sample_valid = 0;
        }
    }
}

void asrc_get_single_output(asrc_block_t *asrc_data_buffer, int32_t *samples) {
    int timestamp;
    asm volatile("gettime %0" : "=r" (timestamp));
    int read_ptr = asrc_data_buffer->read_ptr;
    int write_ptr = asrc_data_buffer->write_ptr;
    int max_fifo_depth = asrc_data_buffer->max_fifo_depth;
    int channel_count = asrc_data_buffer->channel_count;
    int size = channel_count * sizeof(int);
    int len = (write_ptr - read_ptr + max_fifo_depth) % max_fifo_depth;
    memcpy(samples, asrc_data_buffer->buffer + read_ptr * channel_count, size);
    if (len > 2) {
        read_ptr = (read_ptr + 1) % asrc_data_buffer->max_fifo_depth;
        asrc_data_buffer->read_ptr = read_ptr;
        int output_sample_number = asrc_data_buffer->output_sample_number;
        asrc_data_buffer->output_sample_number = output_sample_number + 1;
        if (!asrc_data_buffer->sample_valid) {
            asrc_data_buffer->sample_number = output_sample_number;
            asrc_data_buffer->sample_timestamp = timestamp;
            asrc_data_buffer->sample_valid = 1;
        }
    } else {
        asrc_data_buffer->sample_number = 0; // This must happen in this thread
        asrc_data_buffer->reset = 1;         // The rest must happen in the other thread
    }
}
