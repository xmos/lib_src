// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <xcore/parallel.h>
#include <xcore/hwtimer.h>
#include <xs1.h>
#include <stdlib.h>
#include <stdio.h>
#include <print.h>
#include <platform.h>
#include <xscope.h>
#include "asynchronous_fifo.h"
#include "asrc_timestamp_interpolation.h"

#include <string.h>

#include "src.h"

#define NUM_SAMPLES 1000

DECLARE_JOB(producer,   (asynchronous_fifo_t *));
DECLARE_JOB(consumer,   (asynchronous_fifo_t *, int32_t *));

const int time_step_10ns_ticks = XS1_TIMER_HZ / 48000;


void producer(asynchronous_fifo_t *a) {
    hwtimer_t tmr = hwtimer_alloc();
    uint32_t now = hwtimer_get_time(tmr);

    for(int32_t i = 0; i < NUM_SAMPLES; i++) {
        now += time_step_10ns_ticks;
        hwtimer_set_trigger_time(tmr, now);
        int ts = hwtimer_get_time(tmr);
        int32_t data = i * i;
        asynchronous_fifo_producer_put(a, &data, 1, ts);
    }
    hwtimer_free(tmr);
}

void consumer(asynchronous_fifo_t *a, int32_t *output_samples) {
    hwtimer_t tmr = hwtimer_alloc();
    uint32_t now = hwtimer_get_time(tmr);

    for(int i = 0; i < NUM_SAMPLES; i++) {
        now += time_step_10ns_ticks;
        hwtimer_set_trigger_time(tmr, now);
        int ts = hwtimer_get_time(tmr);
        if (asynchronous_fifo_consumer_get(a, &output_samples[i], ts) != ASYNCH_FIFO_OK) {
            output_samples[i] = -1;
        }
    }
    hwtimer_free(tmr);
}

#define FIFO_LENGTH   100

int main(void) {
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_LENGTH, 1)];
    int32_t output_samples[NUM_SAMPLES];
    asynchronous_fifo_t *asynchronous_fifo_state = (asynchronous_fifo_t *)array;

    asynchronous_fifo_init(asynchronous_fifo_state, 1, FIFO_LENGTH);
    asynchronous_fifo_init_PID_fs_codes(asynchronous_fifo_state, FS_CODE_48, FS_CODE_48);
    PAR_JOBS(
        PJOB(producer, (asynchronous_fifo_state)),
        PJOB(consumer, (asynchronous_fifo_state, output_samples))
        );
    asynchronous_fifo_exit(asynchronous_fifo_state);

    for (int i = 0; i < NUM_SAMPLES; i++) {
        printintln(output_samples[i]);
    }
}
