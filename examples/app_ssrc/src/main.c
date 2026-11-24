// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <basix/architecture.h>
#include <xcore/hwtimer.h>

#define SSRC_N_CHANNELS 4
#define SSRC_N_IN_SAMPLES 4
#include "src.h"
#include "sine_generator.h"
#include "wav_writer.h"

#define AUDIO_DURATION_MS 10
#if AUDIO_DURATION_MS < 1 || AUDIO_DURATION_MS > 100
#error "AUDIO_DURATION_MS must be between 1 and 100"
#endif
#define MAX_INPUT_FRAMES (192000 * AUDIO_DURATION_MS / 1000)
#define MAX_OUTPUT_FRAMES (MAX_INPUT_FRAMES + 20)
#define MAX_BLOCK_OUTPUT_FRAMES (SSRC_N_IN_SAMPLES * 5)

static const unsigned sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000};
static ssrc_state_t state[SSRC_N_CHANNELS];
static ssrc_ctrl_t ctrl[SSRC_N_CHANNELS];
static int stack[SSRC_N_CHANNELS][SSRC_STACK_LENGTH_MULT * SSRC_N_IN_SAMPLES];
static int input[MAX_INPUT_FRAMES * SSRC_N_CHANNELS];
static int output[MAX_OUTPUT_FRAMES * SSRC_N_CHANNELS];

/* Map a sample rate in Hz to its fs_code_t index, or -1 if invalid. */
static int rate_code(const char *text)
{
    char *end;
    long rate = strtol(text, &end, 10);
    if (*text == '\0' || *end != '\0') {
        return -1;
    }
    for (unsigned i = 0; i < sizeof(sample_rates) / sizeof(sample_rates[0]); ++i) {
        if (rate == sample_rates[i]) {
            return (int)i;
        }
    }
    return -1;
}

int main(int argc, char **argv)
{
    int input_code = argc == 1 ? FS_CODE_44 : argc == 3 ? rate_code(argv[1]) : -1;
    int output_code = argc == 1 ? FS_CODE_96 : argc == 3 ? rate_code(argv[2]) : -1;
    if (input_code < 0 || output_code < 0) {
        printf("Usage: xsim --args bin/app_ssrc.xe <input rate> <output rate>\n");
        printf("Without arguments: xsim bin/app_ssrc.xe (44100 -> 96000 Hz)\n");
        printf("Rates: 44100, 48000, 88200, 96000, 176400, 192000 Hz\n");
        return 1;
    }

    unsigned input_rate = sample_rates[input_code];
    unsigned output_rate = sample_rates[output_code];
    unsigned input_frames = input_rate * AUDIO_DURATION_MS / 1000;
    input_frames -= input_frames % SSRC_N_IN_SAMPLES;
    printf("Starting SSRC %u -> %u Hz; generating sine waves\n", input_rate, output_rate);
    fflush(stdout);
    
    if (sine_generate(input, input_frames, SSRC_N_CHANNELS, input_rate)) {
        return 1;
    }
    printf("Generated %u input frames; converting\n", input_frames);
    fflush(stdout);
    
    for (unsigned channel = 0; channel < SSRC_N_CHANNELS; ++channel) {
        ctrl[channel].psState = &state[channel];
        ctrl[channel].piStack = stack[channel];
    }
    ssrc_init((fs_code_t)input_code, (fs_code_t)output_code, ctrl,
              SSRC_N_CHANNELS, SSRC_N_IN_SAMPLES, OFF);

    // Keep track of the maximum processing time and output frames per block
    unsigned output_frames = 0;
    uint32_t max_process_ticks = 0;
    unsigned max_output_frames = 0;

    for (unsigned frame = 0; frame < input_frames; frame += SSRC_N_IN_SAMPLES) {
        int block_output[MAX_BLOCK_OUTPUT_FRAMES * SSRC_N_CHANNELS];
        uint32_t start = get_reference_time();
        unsigned count = ssrc_process(input + frame * SSRC_N_CHANNELS, block_output, ctrl);
        uint32_t elapsed = get_reference_time() - start;
        if (elapsed > max_process_ticks) {
            max_process_ticks = elapsed;
            max_output_frames = count;
        }
        if (count > MAX_BLOCK_OUTPUT_FRAMES || output_frames + count > MAX_OUTPUT_FRAMES) {
            printf("Error: SSRC output exceeds capture buffer\n");
            return 1;
        }
        for (unsigned i = 0; i < count * SSRC_N_CHANNELS; ++i) {
            output[output_frames * SSRC_N_CHANNELS + i] = block_output[i];
        }
        output_frames += count;
    }
    uint64_t max_process_ns = (uint64_t)max_process_ticks * 1000000000 / BASIX_TIMER_HZ;
    printf("Max ssrc_process time: %u.%03u us (%u ticks at %u Hz); "
           "%u input samples and %u output samples per channel, %u channels\n",
           (unsigned)(max_process_ns / 1000), (unsigned)(max_process_ns % 1000),
           (unsigned)max_process_ticks, (unsigned)BASIX_TIMER_HZ,
           SSRC_N_IN_SAMPLES, max_output_frames, SSRC_N_CHANNELS);

    printf("Writing WAV files\n");
    if (wav_write("input.wav", input_rate, SSRC_N_CHANNELS, input, input_frames) ||
        wav_write("output.wav", output_rate, SSRC_N_CHANNELS, output, output_frames)) {
        return 1;
    }
    printf("Complete: SSRC %u -> %u Hz, %u input frames, %u output frames; wrote input.wav and output.wav\n",
           input_rate, output_rate, input_frames, output_frames);

    return 0;
}
