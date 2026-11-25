// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <xcore/channel.h>
#include <xcore/parallel.h>

#include "src.h"
#include "sine_generator.h"
#include "wav_writer.h"

#define CHANNELS 4
#define SAMPLE_RATE 16000
#define DURATION_MS 20
#define FRAMES (SAMPLE_RATE * DURATION_MS / 1000)
#define FACTOR 3

static int input[FRAMES * CHANNELS];
static int output[FRAMES * CHANNELS];

DECLARE_JOB(app, (chanend_t, chanend_t, int *));
DECLARE_JOB(upsample_task, (chanend_t, chanend_t));
DECLARE_JOB(downsample_task, (chanend_t, chanend_t));

static void check_ff3(src_ff3_return_code_t result, const char *operation)
{
    if (result != SRC_FF3_NO_ERROR) {
        printf("Error: %s failed\n", operation);
        exit(1);
    }
}

void app(chanend_t to_upsampler, chanend_t from_downsampler, int *result)
{
    printf("Generating %u frames at %u Hz\n", FRAMES, SAMPLE_RATE);
    *result = sine_generate(input, FRAMES, CHANNELS, SAMPLE_RATE);
    chan_out_word(to_upsampler, *result ? 0 : FRAMES);
    if (*result) {
        return;
    }

    for (unsigned frame = 0; frame < FRAMES; ++frame) {
        for (unsigned channel = 0; channel < CHANNELS; ++channel) {
            chan_out_word(to_upsampler, (uint32_t)input[frame * CHANNELS + channel]);
        }
        for (unsigned channel = 0; channel < CHANNELS; ++channel) {
            output[frame * CHANNELS + channel] = (int32_t)chan_in_word(from_downsampler);
        }
    }

    if (wav_write("input.wav", SAMPLE_RATE, CHANNELS, input, FRAMES) ||
        wav_write("output.wav", SAMPLE_RATE, CHANNELS, output, FRAMES)) {
        *result = 1;
        return;
    }
    printf("Complete: %u frames; wrote input.wav and output.wav\n", FRAMES);
}

void upsample_task(chanend_t from_app, chanend_t to_downsampler)
{
    int delay[CHANNELS][2 * SRC_FF3_OS3_N_COEFS / SRC_FF3_OS3_N_PHASES];
    src_os3_ctrl_t ctrl[CHANNELS] = {{0}};
    for (unsigned channel = 0; channel < CHANNELS; ++channel) {
        ctrl[channel].delay_base = delay[channel];
        check_ff3(src_os3_init(&ctrl[channel]), "OS3 initialization");
    }

    unsigned frames = chan_in_word(from_app);
    chan_out_word(to_downsampler, frames);
    for (unsigned frame = 0; frame < frames; ++frame) {
        for (unsigned channel = 0; channel < CHANNELS; ++channel) {
            ctrl[channel].in_data = (int32_t)chan_in_word(from_app);
            check_ff3(src_os3_input(&ctrl[channel]), "OS3 input");
        }
        for (unsigned phase = 0; phase < FACTOR; ++phase) {
            for (unsigned channel = 0; channel < CHANNELS; ++channel) {
                check_ff3(src_os3_proc(&ctrl[channel]), "OS3 processing");
                chan_out_word(to_downsampler, (uint32_t)ctrl[channel].out_data);
            }
        }
    }
}

void downsample_task(chanend_t from_upsampler, chanend_t to_app)
{
    int delay[CHANNELS][2 * SRC_FF3_DS3_N_COEFS];
    int samples[CHANNELS][FACTOR];
    int converted[CHANNELS];
    src_ds3_ctrl_t ctrl[CHANNELS] = {{0}};
    for (unsigned channel = 0; channel < CHANNELS; ++channel) {
        ctrl[channel].delay_base = delay[channel];
        ctrl[channel].in_data = samples[channel];
        ctrl[channel].out_data = &converted[channel];
        check_ff3(src_ds3_init(&ctrl[channel]), "DS3 initialization");
    }

    unsigned frames = chan_in_word(from_upsampler);
    for (unsigned frame = 0; frame < frames; ++frame) {
        for (unsigned phase = 0; phase < FACTOR; ++phase) {
            for (unsigned channel = 0; channel < CHANNELS; ++channel) {
                samples[channel][phase] = (int32_t)chan_in_word(from_upsampler);
            }
        }
        for (unsigned channel = 0; channel < CHANNELS; ++channel) {
            check_ff3(src_ds3_proc(&ctrl[channel]), "DS3 processing");
            chan_out_word(to_app, (uint32_t)converted[channel]);
        }
    }
}

int main(void)
{
    channel_t app_to_up = chan_alloc();
    channel_t up_to_down = chan_alloc();
    channel_t down_to_app = chan_alloc();

    int result = 0;
    PAR_JOBS(
        PJOB(app, (app_to_up.end_a, down_to_app.end_b, &result)),
        PJOB(upsample_task, (app_to_up.end_b, up_to_down.end_a)),
        PJOB(downsample_task, (up_to_down.end_b, down_to_app.end_a))
    );

    chan_free(app_to_up);
    chan_free(up_to_down);
    chan_free(down_to_app);
    return result;
}
