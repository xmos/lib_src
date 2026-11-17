// Copyright 2023-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcore/channel.h>
#include <xcore/hwtimer.h>
#include <xcore/parallel.h>

#include "src.h"

#define SSRC_N_CHANNELS 2
#define SSRC_N_IN_SAMPLES 4
#define SSRC_N_OUT_IN_RATIO_MAX 5
#define SSRC_STOP UINT32_MAX

static const int sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000};
static const char *input_files[SSRC_N_CHANNELS];
static const char *output_files[SSRC_N_CHANNELS];
static int input_rate = -1;
static int output_rate = -1;
static int total_samples = -1;

DECLARE_JOB(dsp_slave, (chanend_t));
DECLARE_JOB(dsp_mgr, (chanend_t, int *));

static int sample_rate_code(int sample_rate)
{
    for (unsigned index = 0; index < sizeof(sample_rates) / sizeof(sample_rates[0]); index++) {
        if (sample_rates[index] == sample_rate) {
            return (int)index;
        }
    }
    return -1;
}

void dsp_slave(chanend_t channel)
{
    ssrc_state_t state[SSRC_N_CHANNELS];
    int stack[SSRC_N_CHANNELS][SSRC_STACK_LENGTH_MULT * SSRC_N_IN_SAMPLES];
    ssrc_ctrl_t ctrl[SSRC_N_CHANNELS];
    int input[SSRC_N_IN_SAMPLES * SSRC_N_CHANNELS];
    int output[SSRC_N_IN_SAMPLES * SSRC_N_OUT_IN_RATIO_MAX * SSRC_N_CHANNELS] = {0};
    unsigned output_count = 0;
    unsigned input_count = 0;
    unsigned rate_codes = UINT32_MAX;
    uint32_t start_time = 0;

    for (unsigned channel_index = 0; channel_index < SSRC_N_CHANNELS; channel_index++) {
        ctrl[channel_index].psState = &state[channel_index];
        ctrl[channel_index].piStack = stack[channel_index];
    }

    while (1) {
        uint32_t end_time = get_reference_time();
        if (input_count) {
            printf("Process time per chan ticks=%u, Tot samp in count=%u\n",
                   (unsigned)((end_time - start_time) / (SSRC_N_CHANNELS * SSRC_N_IN_SAMPLES)), input_count);
        }

        unsigned new_rate_codes = chan_in_word(channel);
        if (new_rate_codes == SSRC_STOP) {
            break;
        }
        for (unsigned index = 0; index < SSRC_N_IN_SAMPLES * SSRC_N_CHANNELS; index++) {
            input[index] = (int32_t)chan_in_word(channel);
        }
        input_count += SSRC_N_IN_SAMPLES;

        chan_out_word(channel, output_count);
        for (unsigned index = 0; index < output_count * SSRC_N_CHANNELS; index++) {
            chan_out_word(channel, (uint32_t)output[index]);
        }

        if (new_rate_codes != rate_codes) {
            unsigned input_code = (new_rate_codes >> 16) & 0xffff;
            unsigned output_code = new_rate_codes & 0xffff;
            ssrc_init(input_code, output_code, ctrl, SSRC_N_CHANNELS, SSRC_N_IN_SAMPLES, 0);
            rate_codes = new_rate_codes;
            printf("SSRC sample rate in=%d, out=%d\n", sample_rates[input_code], sample_rates[output_code]);
        }

        start_time = get_reference_time();
        output_count = ssrc_process(input, output, ctrl);
    }
}

void dsp_mgr(chanend_t channel, int *result)
{
    FILE *inputs[SSRC_N_CHANNELS] = {0};
    FILE *outputs[SSRC_N_CHANNELS] = {0};
    unsigned input_count = 0;
    unsigned output_count = 0;
    int end_of_file = 0;
    unsigned rate_codes = (unsigned)input_rate << 16 | (unsigned)output_rate;

    for (unsigned channel_index = 0; channel_index < SSRC_N_CHANNELS; channel_index++) {
        inputs[channel_index] = fopen(input_files[channel_index], "rt");
        if (inputs[channel_index] == NULL) {
            printf("Error while opening input file, %s\n", input_files[channel_index]);
            *result = 1;
            goto cleanup;
        }
        printf("Simulator opening input file, %s\n", input_files[channel_index]);
        outputs[channel_index] = fopen(output_files[channel_index], "wt");
        if (outputs[channel_index] == NULL) {
            printf("Error while opening output file, %s\n", output_files[channel_index]);
            *result = 1;
            goto cleanup;
        }
        printf("Simulator opening output file, %s\n", output_files[channel_index]);
    }

    while (!end_of_file) {
        chan_out_word(channel, rate_codes);
        for (unsigned index = 0; index < SSRC_N_IN_SAMPLES * SSRC_N_CHANNELS; index++) {
            int sample = 0;
            if (input_count >= (unsigned)total_samples || fscanf(inputs[index % SSRC_N_CHANNELS], "%i", &sample) != 1) {
                end_of_file = 1;
            }
            chan_out_word(channel, (uint32_t)sample);
        }
        input_count += SSRC_N_IN_SAMPLES;

        unsigned samples_out = chan_in_word(channel);
        for (unsigned index = 0; index < samples_out * SSRC_N_CHANNELS; index++) {
            int32_t sample = (int32_t)chan_in_word(channel);
            if (*result == 0 && fprintf(outputs[index % SSRC_N_CHANNELS], "%d\n", (int)sample) < 0) {
                printf("Error while writing to output file\n");
                *result = 1;
                end_of_file = 1;
            }
        }
        output_count += samples_out;
    }

    printf("DSP manager done - %u output samples produced\n", output_count);
cleanup:
    chan_out_word(channel, SSRC_STOP);
    for (unsigned channel_index = 0; channel_index < SSRC_N_CHANNELS; channel_index++) {
        if (inputs[channel_index] != NULL) {
            fclose(inputs[channel_index]);
        }
        if (outputs[channel_index] != NULL && fclose(outputs[channel_index]) != 0) {
            *result = 1;
        }
    }
}

int main(int argc, char *argv[])
{
    printf("Running SSRC test simulation\n");

    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "-i") == 0 && index + SSRC_N_CHANNELS < argc) {
            for (unsigned channel_index = 0; channel_index < SSRC_N_CHANNELS; channel_index++) {
                input_files[channel_index] = argv[++index];
            }
        } else if (strcmp(argv[index], "-o") == 0 && index + SSRC_N_CHANNELS < argc) {
            for (unsigned channel_index = 0; channel_index < SSRC_N_CHANNELS; channel_index++) {
                output_files[channel_index] = argv[++index];
            }
        } else if (strcmp(argv[index], "-f") == 0 && index + 1 < argc) {
            input_rate = sample_rate_code(atoi(argv[++index]));
        } else if (strcmp(argv[index], "-g") == 0 && index + 1 < argc) {
            output_rate = sample_rate_code(atoi(argv[++index]));
        } else if (strcmp(argv[index], "-n") == 0 && index + 1 < argc) {
            total_samples = atoi(argv[++index]);
        } else {
            printf("Usage: -i input0 input1 -o output0 output1 -f input_rate -g output_rate -n samples\n");
            return 1;
        }
    }

    if (input_rate < 0 || output_rate < 0 || total_samples < 0 ||
        input_files[0] == NULL || input_files[1] == NULL ||
        output_files[0] == NULL || output_files[1] == NULL) {
        printf("ERROR: invalid or missing arguments\n");
        return 1;
    }

    channel_t ssrc_chan = chan_alloc();
    if (ssrc_chan.end_a == 0 || ssrc_chan.end_b == 0) {
        printf("Failed to allocate SSRC channel ends\n");
        return 1;
    }
    int result = 0;

    PAR_JOBS(
        PJOB(dsp_slave, (ssrc_chan.end_a)),
        PJOB(dsp_mgr, (ssrc_chan.end_b, &result))
    );

    chan_free(ssrc_chan);

    return result;
}