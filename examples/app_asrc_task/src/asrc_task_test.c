// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <xcore/chanend.h>
#include <xcore/hwtimer.h>
#include <xcore/parallel.h>
#include <xs1.h>
#include <xscope.h>

#include "asrc_task_config.h"
#include "asrc_task.h"

#define INPUT_SAMPLING_RATE (44100)
#define OUTPUT_SAMPLING_RATE (96000)
#define NUM_CHANS (8)
#define FIFO_LENGTH (SRC_MAX_NUM_SAMPS_OUT * 3)

// This is limited by the time taken to instrument each sample. 8 breaks timing.
#define NUM_CHANS_TO_XSCOPE 2

DECLARE_JOB(producer, (chanend_t));
DECLARE_JOB(asrc_task_job, (chanend_t, asrc_in_out_t *, asynchronous_fifo_t *, unsigned));
DECLARE_JOB(consumer, (asrc_in_out_t *, asynchronous_fifo_t *));

void producer(chanend_t c_producer)
{
    const unsigned sample_rate = INPUT_SAMPLING_RATE;
    const unsigned channel_count = NUM_CHANS;

    #define N_SINE 44
    #define AMPLITUDE 0.8
    int32_t sine[N_SINE];
    for (int i = 0; i < N_SINE; i++) {
        sine[i] = (int32_t)((double)INT32_MAX * AMPLITUDE *
                            sin(i * 2.0 * 3.14159265F / N_SINE));
    }
    int32_t samples[MAX_ASRC_CHANNELS_TOTAL];
    unsigned sine_counter = 0;

    hwtimer_t timer = hwtimer_alloc();
    uint32_t time_trigger = hwtimer_get_time(timer);
    const uint32_t sample_period = XS1_TIMER_HZ / sample_rate;
    printf("Config producer %u %u\n", sample_rate, channel_count);

    while (1) {
        hwtimer_set_trigger_time(timer, time_trigger);
        int32_t time_stamp = (int32_t)hwtimer_get_time(timer);

        for (unsigned channel = 0; channel < channel_count; channel++) {
            samples[channel] = sine[sine_counter % N_SINE];
        }
        sine_counter++;

        send_asrc_input_samples_default(c_producer, sample_rate, time_stamp,
                                        channel_count, samples);

        for(int channel = 0; channel < NUM_CHANS_TO_XSCOPE; channel++) {
            xscope_int(channel, samples[channel]);
        }

        time_trigger += sample_period;
    }
}

void asrc_task_job(chanend_t c_asrc, asrc_in_out_t *asrc_io,
                   asynchronous_fifo_t *fifo, unsigned fifo_length)
{
    asrc_task(c_asrc, asrc_io, fifo, fifo_length);
}

void consumer(asrc_in_out_t *asrc_io, asynchronous_fifo_t *fifo)
{
    const unsigned sample_rate = OUTPUT_SAMPLING_RATE;
    int32_t samples[MAX_ASRC_CHANNELS_TOTAL] = {0};

    hwtimer_t timer = hwtimer_alloc();
    uint32_t time_trigger = hwtimer_get_time(timer);
    const uint32_t sample_period = XS1_TIMER_HZ / sample_rate;

    while (1) {
        hwtimer_set_trigger_time(timer, time_trigger);
        int32_t time_stamp = (int32_t)hwtimer_get_time(timer);

        pull_samples(asrc_io, fifo, samples, sample_rate, time_stamp);
        for(int channel = 0; channel < NUM_CHANS_TO_XSCOPE; channel++) {
            xscope_int(channel + NUM_CHANS_TO_XSCOPE, samples[channel]);
        }
        time_trigger += sample_period;
    }
}

extern void setup_asrc_io_custom_callback(asrc_in_out_t *asrc_io);

int main(void)
{
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_LENGTH, MAX_ASRC_CHANNELS_TOTAL)];
    asrc_in_out_t asrc_io = {{{0}}};
    asynchronous_fifo_t *fifo = (asynchronous_fifo_t *)array;
    chanend_t c_producer = chanend_alloc();
    chanend_t c_asrc = chanend_alloc();

    if (c_producer == 0 || c_asrc == 0) {
        printf("Failed to allocate ASRC channel ends\n");
        return 1;
    }

    chanend_set_dest(c_producer, c_asrc);
    chanend_set_dest(c_asrc, c_producer);
    setup_asrc_io_custom_callback(&asrc_io);

    PAR_JOBS(
        PJOB(producer, (c_producer)),
        PJOB(asrc_task_job, (c_asrc, &asrc_io, fifo, FIFO_LENGTH)),
        PJOB(consumer, (&asrc_io, fifo))
    );

    return 0;
}