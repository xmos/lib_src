// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

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
    state->skip_ctr = state->max_fifo_depth / 2 + 2;
    state->write_ptr = (state->read_ptr + state->max_fifo_depth/2) % state->max_fifo_depth;
    state->last_phase_error = 0;
    state->frequency_ratio = 0;   // Assume perfect match
    state->stop_producing = 0;
}

/**
 * Function that resets the consuming side of the ASRC; called on initialisation, and
 * and called during reset by the consumer before it notifies the producer that a reset
 * is needed.
 */
static void asynchronous_fifo_init_consuming_side(asynchronous_fifo_t *state) {
    state->sample_timestamp = 0;
}

static void asynchronous_fifo_reset_consumer_flags(asynchronous_fifo_t *state) {
    state->reset = 0;        // This has to be the last one
}

#define K_SHIFT 16

static int ticks_between_samples_1D[6] = {
    2268,2083,1134,1042, 567, 521
};

static int Ki_2D[6][6] = {
    {  422,  422,  422,  422,  422,  422 },
    {  422,  422,  422,  422,  422,  422 },
    {  211,  211,  211,  211,  211,  211 },
    {  211,  211,  211,  211,  211,  211 },
    {  105,  105,  105,  105,  105,  105 },
    {  105,  105,  105,  105,  105,  105 }
};

static int Kp_2D[6][6] = {
    {  28147498,  28147498,  56294996,  56294996, 112589992, 112589992 },
    {  28147498,  28147498,  56294996,  56294996, 112589992, 112589992 },
    {  14073749,  14073749,  28147498,  28147498,  56294996,  56294996 },
    {  14073749,  14073749,  28147498,  28147498,  56294996,  56294996 },
    {   7036874,   7036874,  14073749,  14073749,  28147498,  28147498 },
    {   7036874,   7036874,  14073749,  14073749,  28147498,  28147498 }
};

void asynchronous_fifo_init_PID_fs_codes(asynchronous_fifo_t *state,
                                         int fs_input, int fs_output) {
    int max_fifo_depth = state->max_fifo_depth;
    state->Kp = Kp_2D[fs_input][fs_output];
    state->Ki = Ki_2D[fs_input][fs_output];
    state->ideal_phase_error_ticks = ticks_between_samples_1D[fs_output] * (max_fifo_depth/2 + 1);
}

void asynchronous_fifo_init_PID_raw(asynchronous_fifo_t *state,
                                    int Kp, int Ki, int ticks_between_samples) {
    int max_fifo_depth = state->max_fifo_depth;
    state->Kp = Kp;
    state->Ki = Ki;
    state->ideal_phase_error_ticks = ticks_between_samples * (max_fifo_depth/2 + 1);
}

void asynchronous_fifo_init(asynchronous_fifo_t *state, int channel_count,
                            int max_fifo_depth) {
    state->max_fifo_depth = max_fifo_depth;
    state->timestamps = (uint32_t *)state->buffer + max_fifo_depth * channel_count;
    state->channel_count = channel_count;
    state->copy_mask     = (1 << (4*channel_count)) - 1;

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

int async_resets = 0;

void asynchronous_fifo_reset_producer(asynchronous_fifo_t *state) {
    state->stop_producing = 1;
}

void asynchronous_fifo_reset_consumer(asynchronous_fifo_t *state) {
    state->reset = 1;
}

int32_t asynchronous_fifo_produce(asynchronous_fifo_t *state, int32_t *samples,
                                  int n,
                                  int32_t timestamp,
                                  int xscope_used) {
    int read_ptr = state->read_ptr;
    int write_ptr = state->write_ptr;
    int max_fifo_depth = state->max_fifo_depth;
    int channel_count = state->channel_count;
    int copy_mask = state->copy_mask;
    int len = (write_ptr - read_ptr + max_fifo_depth) % max_fifo_depth;
    if (state->reset) {
        async_resets++;
        asynchronous_fifo_init_producing_side(state);    // uses read_ptr
        asynchronous_fifo_reset_consumer_flags(state);   // Last step - clears reset
    } else if (len >= max_fifo_depth - 2 - n) {
        state->stop_producing = 1;
    } else if (!state->stop_producing && n) {
        for(int j = 0; j < n; j++) {

#ifdef __XS2A__
            memcpy(state->buffer + write_ptr * channel_count, samples, channel_count * sizeof(int));
#else
            register int32_t *ptr asm("r11") = samples;
            asm("vldr %0[0]" :: "r" (ptr));
            asm("vstrpv %0[0], %1" :: "r" (state->buffer + write_ptr * channel_count), "r" (copy_mask));
#endif
            samples += channel_count;
            write_ptr = (write_ptr + 1);
            if (write_ptr >= max_fifo_depth) {
                write_ptr = 0;
            }
        }

        /* Difference between timestamp recorded by consumer and current timestamp */
        state->write_ptr = write_ptr;
        int32_t phase_error = state->timestamps[write_ptr] - timestamp;

        /* Ideal phase error is the middle of the fifo measured in ticks */
        phase_error += state->ideal_phase_error_ticks;

        /* Don't try and use timestamps that haven't been recorded yet! */
        if (state->skip_ctr != 0) {
            state->skip_ctr--;
        } else {
            // Now that we have a phase error, calculate the proportional error
            // and use that and the integral error to correct the ASRC factor
            int32_t diff_error = phase_error - state->last_phase_error;

            state->frequency_ratio +=
                (diff_error  * (int64_t) (state->Kp / n)) +  // TODO: make this lookup table
                (phase_error * (int64_t) state->Ki);
            if (xscope_used) {
                xscope_int(1, phase_error);
                xscope_int(2, diff_error);
            }
        }
        state->last_phase_error = phase_error;
    }
    if (xscope_used) {
        xscope_int(3, len);
        xscope_int(4, state->frequency_ratio >> K_SHIFT);
    }
    return (state->frequency_ratio + (1<<(K_SHIFT-1))) >> K_SHIFT;
}

/**
 * Function that implements the consumer interface. Control communication
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
    int copy_mask = state->copy_mask;
    int len = (write_ptr - read_ptr + max_fifo_depth) % max_fifo_depth;
#ifdef __XS2A__
    memcpy(samples, state->buffer + read_ptr * channel_count, channel_count * sizeof(int));
#else
    register int32_t *ptr asm("r11") = state->buffer + read_ptr * channel_count;
    asm("vldr %0[0]" :: "r" (ptr));
    asm("vstrpv %0[0], %1" :: "r" (samples), "r" (copy_mask));
#else
    if (state->reset) {
        return;
    }
    if (len > 2) {
        // TODO: use IF not %
        read_ptr = (read_ptr + 1) % state->max_fifo_depth;
        state->read_ptr = read_ptr;
        state->timestamps[read_ptr] = timestamp;
    } else {
        asynchronous_fifo_init_consuming_side(state); // This must happen in this thread
        state->reset = 1;                // The rest must happen in the other thread
    }
}
