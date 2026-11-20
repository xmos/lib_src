// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include "src.h"

#define SSRC_N_CHANNELS 1
#define SSRC_N_IN_SAMPLES 4
#define OUTPUT_SAMPLES (2 * SSRC_N_IN_SAMPLES)

static ssrc_ctrl_t control[SSRC_N_CHANNELS];
static ssrc_state_t state[SSRC_N_CHANNELS];
static int stack[SSRC_N_CHANNELS][SSRC_STACK_LENGTH_MULT * SSRC_N_IN_SAMPLES];

int fifo_check(void);

int main(void)
{
    int input[SSRC_N_IN_SAMPLES] = {0};
    int output[OUTPUT_SAMPLES];

    for (unsigned i = 0; i < OUTPUT_SAMPLES; ++i)
        output[i] = 1;

    unsafe {
        control[0].psState = &state[0];
        control[0].piStack = stack[0];
    }
    ssrc_init(FS_CODE_48, FS_CODE_96, control, SSRC_N_CHANNELS, SSRC_N_IN_SAMPLES, OFF);

    const unsigned count = ssrc_process(input, output, control);
    if (count != OUTPUT_SAMPLES) {
        printf("Expected %u output samples, got %u\n", (unsigned)OUTPUT_SAMPLES, count);
        return 1;
    }

    for (unsigned i = 0; i < count; ++i) {
        if (output[i] != 0) {
            printf("Expected silence at output sample %u, got %d\n", i, output[i]);
            return 1;
        }
    }

    if (fifo_check() != 0)
        return 1;

    puts("Legacy xcommon SSRC and asynchronous FIFO smoke test passed");
    return 0;
}
