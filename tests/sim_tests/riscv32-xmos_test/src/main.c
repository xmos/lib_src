// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include "src.h"
#include "asynchronous_fifo.h"

#define SSRC_N_CHANNELS 1
#define SSRC_N_IN_SAMPLES 4
#define OUTPUT_SAMPLES (2 * SSRC_N_IN_SAMPLES)
#define FIFO_DEPTH 16

static ssrc_ctrl_t control[SSRC_N_CHANNELS];
static ssrc_state_t state[SSRC_N_CHANNELS];
static int stack[SSRC_STACK_LENGTH_MULT * SSRC_N_IN_SAMPLES];
static int64_t fifo_storage[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_DEPTH, SSRC_N_CHANNELS)];

int main(void)
{
    int input[SSRC_N_IN_SAMPLES] = {0};
    int output[OUTPUT_SAMPLES];

    for (unsigned i = 0; i < OUTPUT_SAMPLES; ++i)
        output[i] = 1;

    control[0].psState = &state[0];
    control[0].piStack = stack;
    ssrc_init(FS_CODE_48, FS_CODE_96, control, SSRC_N_CHANNELS, SSRC_N_IN_SAMPLES, OFF);

    unsigned count = ssrc_process(input, output, control);
    if (count != OUTPUT_SAMPLES) {
        printf("Expected %d output samples, got %u\n", OUTPUT_SAMPLES, count);
        return 1;
    }
    for (unsigned i = 0; i < count; ++i) {
        if (output[i] != 0) {
            printf("Expected silence at output sample %u, got %d\n", i, output[i]);
            return 1;
        }
    }

    asynchronous_fifo_t *fifo = (asynchronous_fifo_t *)fifo_storage;
    asynchronous_fifo_init(fifo, SSRC_N_CHANNELS, FIFO_DEPTH);
    asynchronous_fifo_init_PID_fs_codes(fifo, FS_CODE_48, FS_CODE_48);
    int32_t sample = 1;
    if (asynchronous_fifo_consumer_get(fifo, &sample, 0) != ASYNCH_FIFO_OK || sample != 0) {
        puts("Expected a zero sample from the FIFO prefill");
        return 1;
    }
    asynchronous_fifo_exit(fifo);

    puts("VX4 SSRC and asynchronous FIFO smoke test passed");
    return 0;
}
