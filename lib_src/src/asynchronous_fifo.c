#include <string.h>
#include "asynchronous_fifo.h"

#include <xcore/chanend.h>
#include <platform.h>
#include <xs1.h>
#include <print.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xscope.h>
#include "src.h"


// TODO: Make fifo offset from N/2 a very small component in PID, 

/**
 * Function that resets the producing side of the ASRC; called on initialisation, and
 * and called during reset by the producer after the consumer is known to have thrown
 * in the towel
 */
static void asynchronous_fifo_init_producing_side(asynchronous_fifo_t *state) {
    state->skip_ctr = 2;
    state->write_ptr = (state->read_ptr + state->max_fifo_depth/2) % state->max_fifo_depth;
    state->converted_sample_number = 0;
    state->marked_converted_sample_number = 0;
    state->marked_converted_sample_time = 0;
    state->last_phase_error = 0;
    state->last_timestamp = 0;
    state->frequency_ratio = 0;   // Assume perfect match
    state->stop_producing = 0;
}

/**
 * Function that resets the consuming side of the ASRC; called on initialisation, and
 * and called during reset by the consumer before it notifies the producer that a reset
 * is needed.
 */
static void asynchronous_fifo_init_consuming_side(asynchronous_fifo_t *state) {
    state->output_sample_number = 1;
    state->sample_timestamp = 0;
    state->sample_number = 0;
}

static void asynchronous_fifo_reset_consumer_flags(asynchronous_fifo_t *state) {
    state->sample_valid = 0;
    state->reset = 0;        // This has to be the last one
}

#define KI_SHIFT 16

void asynchronous_fifo_init(asynchronous_fifo_t *state, int channel_count,
                            int max_fifo_depth, int ticks_between_samples) {
    state->max_fifo_depth = max_fifo_depth;
    state->channel_count = channel_count;
    state->ticks_between_samples = ticks_between_samples;

    // All this maths shall be compiled out to leave nothing.

                // frequency ratio measured in 32.32 used to be in 64.60
                // So Kp/Ki should be (1<<28) higher
                // Phase error measured in 10ns not 10ns/48000
                // Ratio is 48000 times smaller
                // So Kp/Ki should be 48000 times lower
                // Old maths: Kp * (1<<60) / 48000
                // Old maths: Ki * (1<<60) / 48000
                // Old maths: 64.60 +  Kii * 10ns/48000  +  Kpi * ratio/480000
                // Old maths: 32.32 +  KiNi * 10ns  +  KpNi * ratio
                // KpNi should be 48000 * Kpi = Kp * (1<<32).
                // KiNi should be 48000 * Kpi = Kp * (1<<32).
    float SPEEDUP = 4;
    float Kp = 0.0001 * SPEEDUP;
    float Ki = 0.00025 * 1e-8 * SPEEDUP;
    state->Kp = Kp * (1LL<<32);
    double Ki_d = Ki * (1LL << 32) * (1LL << KI_SHIFT);
    state->Ki = Ki_d < 1 ? 1 : Ki_d > 0x7fffffff ? 0x7fffffff : (int32_t) Ki_d;
    printf("Kp %08lx   Ki %08lx\n", state->Kp, state->Ki);

    // First initialise shared variables, or those that shouldn't reset on a RESET.
    state->read_ptr = 0;
    // Now clear the buffer.
    memset(state->buffer, 0, channel_count * max_fifo_depth * sizeof(int));
    // Finally initialise those parts that are reset on a RESET
    asynchronous_fifo_init_consuming_side(state);    // Does not reset read_ptr
    asynchronous_fifo_init_producing_side(state);    // uses read_ptr
    asynchronous_fifo_reset_consumer_flags(state);
}

void asynchronous_fifo_exit(asynchronous_fifo_t *state) {
}
    
int32_t asynchronous_fifo_produce(asynchronous_fifo_t *state, int32_t *sample,
                                  int32_t timestamp, int timestamp_valid) {
    int read_ptr = state->read_ptr;
    int write_ptr = state->write_ptr;
    int max_fifo_depth = state->max_fifo_depth;
    int channel_count = state->channel_count;
    int len = (write_ptr - read_ptr + max_fifo_depth) % max_fifo_depth;
    if ( state->reset) {
        printf("@@@@@@@@@@@@ RESET @@@@@@@@@@ %lu len %d\n", state->reset, len );    
        asynchronous_fifo_init_producing_side(state);    // uses read_ptr
        asynchronous_fifo_reset_consumer_flags(state);   // Last step - clears reset
    } else if (len >= max_fifo_depth - 2) {
        state->stop_producing = 1;
    } else if (!state->stop_producing) {
        memcpy(state->buffer + write_ptr * channel_count, sample, channel_count * sizeof(int));
        write_ptr = (write_ptr + 1) % state->max_fifo_depth;
        state->write_ptr = write_ptr;
        state->converted_sample_number++;
        if (timestamp_valid && state->sample_valid) {
            int32_t phase_error = state->sample_timestamp - state->marked_converted_sample_time;
            phase_error -= (state->sample_number - state->marked_converted_sample_number) * state->ticks_between_samples;
            // Now that we have a phase error, calculate the proportional error
            // and use that and the integral error to correct the ASRC factor
            if (state->skip_ctr != 0) {
                state->skip_ctr--;
            } else {
                int32_t diff_error = phase_error - state->last_phase_error;
#if 1
                xscope_int(1, phase_error);
                xscope_int(2, diff_error);
                xscope_int(3, len);
                xscope_int(4, state->frequency_ratio);
#endif

                int32_t diff_time = timestamp - state->last_timestamp;
                if (diff_time == 0) diff_time = 0x7fffffff; // Avoid bad stuff.
                
                state->frequency_ratio +=
                    diff_error  * (int64_t) state->Kp / diff_time +
                    ((phase_error * (int64_t) state->Ki) >> KI_SHIFT);
            }
            state->last_phase_error = phase_error;
            state->last_timestamp = timestamp;
            state->marked_converted_sample_number = state->converted_sample_number;
            state->marked_converted_sample_time   = timestamp;
            state->sample_valid = 0;  // Do this last - permits other side to refill
        }
    }
    return state->frequency_ratio;
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
void asynchronous_fifo_consume(asynchronous_fifo_t *state, int32_t *samples, int32_t timestamp) {
    int read_ptr = state->read_ptr;
    int write_ptr = state->write_ptr;
    int max_fifo_depth = state->max_fifo_depth;
    int channel_count = state->channel_count;
    int size = channel_count * sizeof(int);
    int len = (write_ptr - read_ptr + max_fifo_depth) % max_fifo_depth;
    memcpy(samples, state->buffer + read_ptr * channel_count, size);
    if (state->reset) {
        return;
    }
    if (len > 2) {
        read_ptr = (read_ptr + 1) % state->max_fifo_depth;
        state->read_ptr = read_ptr;
        int output_sample_number = state->output_sample_number;
        state->output_sample_number = output_sample_number + 1;
        if (!state->sample_valid) {
            state->sample_number = output_sample_number;
            state->sample_timestamp = timestamp;
            state->sample_valid = 1;  // Do this last, permits other side to use it
        }
    } else {
        asynchronous_fifo_init_consuming_side(state); // This must happen in this thread
        state->reset = 1;                // The rest must happen in the other thread
    }
}
