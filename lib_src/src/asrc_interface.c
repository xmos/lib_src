#include <string.h>
#include "asrc_interface.h"

#include <xcore/chanend.h>
#include <platform.h>
#include <xs1.h>
#include <print.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xscope.h>
#include "src.h"


// TODO: Make it work on overflow beyond 20 seconds
// TODO: Implement RESET
// TODO: Verify that RESET never leaves it at 75% thinking its in the middle
// TODO: Make fifo offset from N/2 a very small component in PID, 
// TODO: Check it works on 192/48.

// Software engineering TODOs
// TODO: Make sample rate changeable
// TODO: extract ASRC parameters from this file


#define REAL_ASRC

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

/**
 * Function that resets the producing side of the ASRC; called on initialisation, and
 * and called during reset by the producer after the consumer is known to have thrown
 * in the towel
 */
static void asrc_init_producing_side(asrc_interface_t *asrc_data_buffer) {
    asrc_data_buffer->skip_ctr = 2;
    asrc_data_buffer->write_ptr = (asrc_data_buffer->read_ptr + asrc_data_buffer->max_fifo_depth/2) % asrc_data_buffer->max_fifo_depth;
    asrc_data_buffer->converted_sample_number = 0;
    asrc_data_buffer->marked_converted_sample_number = 0;
    asrc_data_buffer->marked_converted_sample_time = 0;
    asrc_data_buffer->last_phase_error = 0;
    asrc_data_buffer->last_timestamp = 0;
    asrc_data_buffer->fs_ratio = (uint64_t)(0x1000000000000000ULL); // TODO, right value
    asrc_data_buffer->fractional_credit = 0;
    asrc_data_buffer->stop_producing = 0;
}

/**
 * Function that resets the consuming side of the ASRC; called on initialisation, and
 * and called during reset by the consumer before it notifies the producer that a reset
 * is needed.
 */
static void asrc_init_consuming_side(asrc_interface_t *asrc_data_buffer) {
    asrc_data_buffer->output_sample_number = 0;
    asrc_data_buffer->sample_timestamp = 0;
    asrc_data_buffer->sample_number = 0;
}

static void asrc_reset_consumer_flags(asrc_interface_t *asrc_data_buffer) {
    asrc_data_buffer->sample_valid = 0;
    asrc_data_buffer->reset = 0;        // This has to be the last one
}

void asrc_init_buffer(asrc_interface_t *asrc_data_buffer, int channel_count, int max_fifo_depth) {
    asrc_data_buffer->max_fifo_depth = max_fifo_depth;
    asrc_data_buffer->channel_count = channel_count;
    // TODO: parameter
    asrc_data_buffer->ideal_freq_input = 44100;
    // TODO: make Kp int32_t clamped to MAXINT
    // All this maths shall be compiled out to just one LDIVU each
    float SPEEDUP = 400;
    float Kp = 0.0001 * SPEEDUP;
    float Ki = 0.000025 * 1e-8 * SPEEDUP;
    asrc_data_buffer->Kp = ((uint64_t)(Kp * 0x1000000000000000LL)) / asrc_data_buffer->ideal_freq_input;
    asrc_data_buffer->Ki = ((uint64_t)(Ki * 0x1000000000000000LL)) / asrc_data_buffer->ideal_freq_input;
    if (asrc_data_buffer->Ki < 1) {
        asrc_data_buffer->Ki = 1;       // Avoid underflow.
    }
    printf("Kp %016llx   Ki %08lx\n", asrc_data_buffer->Kp, asrc_data_buffer->Ki);

    // First initialise shared variables, or those that shouldn't reset on a RESET.
    asrc_data_buffer->read_ptr = 0;
    // Now clear the buffer.
    memset(asrc_data_buffer->buffer, 0, channel_count * max_fifo_depth * sizeof(int));
    // Finally initialise those parts that are reset on a RESET
    asrc_init_consuming_side(asrc_data_buffer);    // Does not reset read_ptr
    asrc_init_producing_side(asrc_data_buffer);    // uses read_ptr
    asrc_reset_consumer_flags(asrc_data_buffer);
    
#ifdef REAL_ASRC
    int inputFsCode = FS_CODE_48;
    int outputFsCode = FS_CODE_44;

    for(int ui = 0; ui < SRC_CHANNELS_PER_INSTANCE; ui++)
    {
            // Set state, stack and coefs into ctrl structure
            sASRCCtrl[ui].psState                   = &sASRCState[ui];
            sASRCCtrl[ui].piStack                   = iASRCStack[ui];
            sASRCCtrl[ui].piADCoefs                 = asrc_adfir_coefs.iASRCADFIRCoefs;
    }

    asrc_data_buffer->fs_ratio = asrc_init(inputFsCode, outputFsCode, sASRCCtrl, SRC_CHANNELS_PER_INSTANCE, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);
    printf("%016llx\n",  asrc_data_buffer->fs_ratio);
#endif
}

void asrc_exit_buffer(asrc_interface_t *asrc_data_buffer) {
}
    
void asrc_add_quad_input(asrc_interface_t *asrc_data_buffer, int32_t *samples, int32_t timestamp) {
    int outputBuff[SRC_OUT_BUFF_SIZE];
    int sampsOut = 0;

//    asrc_data_buffer->fs_ratio = 0x1000000000000000ULL; // todo: delete

#ifdef REAL_ASRC
    /* Run ASRC from input buffer into output buffer */
    sampsOut = asrc_process((int *)samples, outputBuff, asrc_data_buffer->fs_ratio, sASRCCtrl);
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
    if ( asrc_data_buffer->reset) {
        printf("@@@@@@@@@@@@ RESET @@@@@@@@@@ %lu len %d\n", asrc_data_buffer->reset, len );    
        asrc_init_producing_side(asrc_data_buffer);    // uses read_ptr
        asrc_reset_consumer_flags(asrc_data_buffer);   // Last step - clears reset
    } else if (len >= max_fifo_depth - 6) {
        asrc_data_buffer->stop_producing = 1;
    } else  if (!asrc_data_buffer->stop_producing) {
        for(int i = 0; i < sampsOut; i++) {
            int size = channel_count * sizeof(int);
            memcpy(asrc_data_buffer->buffer + write_ptr * channel_count, outputBuff + i, size);
            write_ptr = (write_ptr + 1) % asrc_data_buffer->max_fifo_depth;
        }
        asrc_data_buffer->write_ptr = write_ptr;
        // fractional_credit is the number of samples that are unprocessed in the input.
        asrc_data_buffer->fractional_credit += 0x4000000000000000LL;
        while (asrc_data_buffer->fractional_credit >= asrc_data_buffer->fs_ratio) {
            asrc_data_buffer->fractional_credit -= asrc_data_buffer->fs_ratio;
            asrc_data_buffer->converted_sample_number += 1;
        }
        // fractional_credit is now less than fs_ratio, so we can compute an unsigned diff
        // A single SUB
        uint32_t left_over_upper_32_bits = (asrc_data_buffer->fs_ratio >> 32) - (asrc_data_buffer->fractional_credit >> 32);
        // The unsigned diff just needs a single LMUL to become a 64 bit diff in 10ns TICKS
        uint64_t left_over_ticks = left_over_upper_32_bits * 100000000ULL;
        int64_t converted_sample_time = timestamp*(int64_t) asrc_data_buffer->ideal_freq_input + left_over_ticks / (int) (asrc_data_buffer->fs_ratio >> 32);
        
        if (asrc_data_buffer->sample_valid) {
            // a time-stamp is a number in the range [-0x80000000..0x7fffffff] and wraps
            // The computation may wrap as either F*MININT - F*MAXINT or F*MAXINT - F*MININT
            // This ends up as 2*F*MININT or 2*F*MAXINT.
            // Normal values are close to 0
            // Hence - we need to add or subtract (F<<32) when we hit this case.
            int64_t max_val = ((int64_t)asrc_data_buffer->ideal_freq_input) << 32;
            int64_t phase_error = asrc_data_buffer->sample_timestamp*(int64_t)asrc_data_buffer->ideal_freq_input - asrc_data_buffer->marked_converted_sample_time;
            if (phase_error > max_val/2) {
                phase_error -= max_val;
            } else if (phase_error < -max_val/2) {
                phase_error += max_val;
            }
            phase_error -= ((asrc_data_buffer->sample_number - asrc_data_buffer->marked_converted_sample_number) * 100000000LL );
            // Now that we have a phase error, calculate the proportional error
            // and use that and the integral error to correct the ASRC factor
            if (asrc_data_buffer->skip_ctr != 0) {
                asrc_data_buffer->skip_ctr--;
            } else {
                int32_t diff_error = phase_error - asrc_data_buffer->last_phase_error;
                xscope_int(1, phase_error);
                xscope_int(2, diff_error);
                xscope_int(3, len);
                xscope_int(4, asrc_data_buffer->fs_ratio >> 32);
                int32_t diff_time = timestamp - asrc_data_buffer->last_timestamp;
                if (diff_time == 0) diff_time = 0x7fffffff; // Avoid bad stuff.

                asrc_data_buffer->fs_ratio +=
                    diff_error  * asrc_data_buffer->Kp / diff_time +
                    phase_error * asrc_data_buffer->Ki;
            }
            asrc_data_buffer->last_phase_error = phase_error;
            asrc_data_buffer->last_timestamp = timestamp;

            asrc_data_buffer->marked_converted_sample_number = asrc_data_buffer->converted_sample_number;
            asrc_data_buffer->marked_converted_sample_time   = converted_sample_time;
            asrc_data_buffer->sample_valid = 0;
        }
    }
}

/**
 * Function that implements the consumer interface. Contorl communication
 * happens through two variables: reset and sample_data_valid. These shall
 * only be set as the last action, as they signify to the production side
 * that the datastructure can now be read on that side.
 *
 * Note that the samples are filled in regardless of whether the FIFO is
 * operating or not; the consumer will repeatedly get the same sample if
 * the producer fails. The producer side is reset exactly once on reset.
 */
void asrc_get_single_output(asrc_interface_t *asrc_data_buffer, int32_t *samples, int32_t timestamp) {
    int read_ptr = asrc_data_buffer->read_ptr;
    int write_ptr = asrc_data_buffer->write_ptr;
    int max_fifo_depth = asrc_data_buffer->max_fifo_depth;
    int channel_count = asrc_data_buffer->channel_count;
    int size = channel_count * sizeof(int);
    int len = (write_ptr - read_ptr + max_fifo_depth) % max_fifo_depth;
    memcpy(samples, asrc_data_buffer->buffer + read_ptr * channel_count, size);
    if (asrc_data_buffer->reset) {
        return;
    }
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
        asrc_init_consuming_side(asrc_data_buffer); // This must happen in this thread        
        asrc_data_buffer->reset = 1;                // The rest must happen in the other thread
    }
}
