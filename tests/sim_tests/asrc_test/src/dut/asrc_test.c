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

#ifndef ASRC_N_CHANNELS
#define ASRC_N_CHANNELS 2
#endif
#define ASRC_N_IN_SAMPLES 4
#define ASRC_N_OUT_IN_RATIO_MAX 5
#define ASRC_STOP UINT32_MAX

static const int sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000};
static const char *input_files[ASRC_N_CHANNELS];
static const char *output_files[ASRC_N_CHANNELS];
static int input_rate = -1;
static int output_rate = -1;
static int total_samples = -1;
static double ratio_deviation = -1.0;

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
    asrc_state_t state[ASRC_N_CHANNELS];
    int stack[ASRC_N_CHANNELS][ASRC_STACK_LENGTH_MULT * ASRC_N_IN_SAMPLES];
    asrc_ctrl_t ctrl[ASRC_N_CHANNELS];
    asrc_adfir_coefs_t adfir_coefs;
    int input[ASRC_N_IN_SAMPLES * ASRC_N_CHANNELS];
    int output[ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX * ASRC_N_CHANNELS] = {0};
    unsigned output_count = 0;
    unsigned input_count = 0;
    unsigned rate_codes = UINT32_MAX;
    uint32_t start_time = 0;
    hwtimer_t timer = hwtimer_alloc();

    for (unsigned channel_index = 0; channel_index < ASRC_N_CHANNELS; channel_index++) {
        ctrl[channel_index].psState = &state[channel_index];
        ctrl[channel_index].piStack = stack[channel_index];
        ctrl[channel_index].piADCoefs = adfir_coefs.iASRCADFIRCoefs;
    }

    while (1) {
        uint32_t end_time = hwtimer_get_time(timer);
        if (input_count) {
            printf("Process time per chan ticks=%u, Tot samp in count=%u\n",
                   (unsigned)((end_time - start_time) / (ASRC_N_CHANNELS * ASRC_N_IN_SAMPLES)), input_count);
        }

        unsigned new_rate_codes = chan_in_word(channel);
        if (new_rate_codes == ASRC_STOP) {
            break;
        }
        uint64_t ratio = chan_in_word(channel);
        ratio |= (uint64_t)chan_in_word(channel) << 32;
        for (unsigned index = 0; index < ASRC_N_IN_SAMPLES * ASRC_N_CHANNELS; index++) {
            input[index] = (int32_t)chan_in_word(channel);
        }
        input_count += ASRC_N_IN_SAMPLES;

        chan_out_word(channel, output_count);
        for (unsigned index = 0; index < output_count * ASRC_N_CHANNELS; index++) {
            chan_out_word(channel, (uint32_t)output[index]);
        }

        if (new_rate_codes != rate_codes) {
            unsigned input_code = (new_rate_codes >> 16) & 0xffff;
            unsigned output_code = new_rate_codes & 0xffff;
            asrc_init(input_code, output_code, ctrl, ASRC_N_CHANNELS, ASRC_N_IN_SAMPLES, 0);
            rate_codes = new_rate_codes;
            printf("ASRC sample rate in=%d, out=%d\n", sample_rates[input_code], sample_rates[output_code]);
        }

        start_time = hwtimer_get_time(timer);
        output_count = asrc_process(input, output, ratio, ctrl);
    }
    hwtimer_free(timer);
}

void dsp_mgr(chanend_t channel, int *result)
{
    FILE *inputs[ASRC_N_CHANNELS] = {0};
    FILE *outputs[ASRC_N_CHANNELS] = {0};
    unsigned input_count = 0;
    unsigned output_count = 0;
    int end_of_file = 0;
    unsigned rate_codes = (unsigned)input_rate << 16 | (unsigned)output_rate;
    uint64_t ratio = (uint64_t)(((double)sample_rates[input_rate] / sample_rates[output_rate]) * ((uint64_t)1 << 60));
    ratio = (uint64_t)((double)ratio * ratio_deviation);

    for (unsigned channel_index = 0; channel_index < ASRC_N_CHANNELS; channel_index++) {
        inputs[channel_index] = fopen(input_files[channel_index], "rt");
        if (inputs[channel_index] == NULL) {
            printf("Error while opening input file, %s\n", input_files[channel_index]);
            *result = 1;
            goto cleanup;
        }
        outputs[channel_index] = fopen(output_files[channel_index], "wt");
        if (outputs[channel_index] == NULL) {
            printf("Error while opening output file, %s\n", output_files[channel_index]);
            *result = 1;
            goto cleanup;
        }
    }

    while (!end_of_file) {
        chan_out_word(channel, rate_codes);
        chan_out_word(channel, (uint32_t)ratio);
        chan_out_word(channel, (uint32_t)(ratio >> 32));
        for (unsigned index = 0; index < ASRC_N_IN_SAMPLES * ASRC_N_CHANNELS; index++) {
            int sample = 0;
            if (input_count >= (unsigned)total_samples || fscanf(inputs[index % ASRC_N_CHANNELS], "%i", &sample) != 1) {
                end_of_file = 1;
            }
            chan_out_word(channel, (uint32_t)sample);
        }
        input_count += ASRC_N_IN_SAMPLES;

        unsigned samples_out = chan_in_word(channel);
        for (unsigned index = 0; index < samples_out * ASRC_N_CHANNELS; index++) {
            int32_t sample = (int32_t)chan_in_word(channel);
            if (*result == 0 && fprintf(outputs[index % ASRC_N_CHANNELS], "%d\n", (int)sample) < 0) {
                printf("Error while writing to output file\n");
                *result = 1;
                end_of_file = 1;
            }
        }
        output_count += samples_out;
    }

    printf("DSP manager done - %u output samples produced\n", output_count);
cleanup:
    chan_out_word(channel, ASRC_STOP);
    for (unsigned channel_index = 0; channel_index < ASRC_N_CHANNELS; channel_index++) {
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
    printf("Running ASRC test simulation\n");

    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "-i") == 0 && index + ASRC_N_CHANNELS < argc) {
            for (unsigned channel_index = 0; channel_index < ASRC_N_CHANNELS; channel_index++) {
                input_files[channel_index] = argv[++index];
            }
        } else if (strcmp(argv[index], "-o") == 0 && index + ASRC_N_CHANNELS < argc) {
            for (unsigned channel_index = 0; channel_index < ASRC_N_CHANNELS; channel_index++) {
                output_files[channel_index] = argv[++index];
            }
        } else if (strcmp(argv[index], "-f") == 0 && index + 1 < argc) {
            input_rate = sample_rate_code(atoi(argv[++index]));
        } else if (strcmp(argv[index], "-g") == 0 && index + 1 < argc) {
            output_rate = sample_rate_code(atoi(argv[++index]));
        } else if (strcmp(argv[index], "-n") == 0 && index + 1 < argc) {
            total_samples = atoi(argv[++index]);
        } else if (strcmp(argv[index], "-e") == 0 && index + 1 < argc) {
            ratio_deviation = atof(argv[++index]);
        } else {
            printf("Usage: -i input0 input1 -o output0 output1 -f input_rate -g output_rate -n samples -e deviation\n");
            return 1;
        }
    }

    if (input_rate < 0 || output_rate < 0 || total_samples < 0 || ratio_deviation < 0) {
        printf("ERROR: invalid or missing arguments\n");
        return 1;
    }
    for (unsigned channel_index = 0; channel_index < ASRC_N_CHANNELS; channel_index++) {
        if (input_files[channel_index] == NULL || output_files[channel_index] == NULL) {
            printf("ERROR: invalid or missing file names\n");
            return 1;
        }
    }

    channel_t asrc_chan = chan_alloc();
    if (asrc_chan.end_a == 0 || asrc_chan.end_b == 0) {
        printf("Failed to allocate ASRC channel ends\n");
        return 1;
    }
    int result = 0;
    PAR_JOBS(
        PJOB(dsp_slave, (asrc_chan.end_a)),
        PJOB(dsp_mgr, (asrc_chan.end_b, &result))
    );
    chan_free(asrc_chan);
    return result;
}