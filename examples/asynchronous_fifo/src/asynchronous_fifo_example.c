// Copyright 2024 XMOS LIMITED.
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

DECLARE_JOB(producer,   (asynchronous_fifo_t *));
DECLARE_JOB(consumer,   (asynchronous_fifo_t *));


void producer(asynchronous_fifo_t *a) {
    hwtimer_t tmr = hwtimer_alloc();
    uint32_t now = hwtimer_get_time(tmr);
    int step = 2000;
    int error;
    
    for(int32_t i = 0; i < 1000; i++) {
        now += step;
        hwtimer_set_trigger_time(tmr, now);
        int ts = hwtimer_get_time(tmr);
        int32_t data = i * i;
        error = asynchronous_fifo_producer_put(a, &data, 1, ts, 0);
    }
    hwtimer_free(tmr);
}

void consumer(asynchronous_fifo_t *a) {
    hwtimer_t tmr = hwtimer_alloc();
    uint32_t now = hwtimer_get_time(tmr);
    int step = 2000;
    int32_t output_data;

    for(int i = 0; i < 1000; i++) {
        now += step;
        hwtimer_set_trigger_time(tmr, now);
        int ts = hwtimer_get_time(tmr);
        asynchronous_fifo_consumer_get(a, &output_data, ts);
        printintln(output_data);
    }
    hwtimer_free(tmr);
}

#define FIFO_LENGTH   100

int main(void) {
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_LENGTH, 1)];
    asynchronous_fifo_t *asynchronous_fifo_state = (asynchronous_fifo_t *)array;

    asynchronous_fifo_init(asynchronous_fifo_state, 1, FIFO_LENGTH);
    asynchronous_fifo_init_PID_fs_codes(asynchronous_fifo_state, 0, 0);
    PAR_JOBS(
        PJOB(producer, (asynchronous_fifo_state)),
        PJOB(consumer, (asynchronous_fifo_state))
        );
    asynchronous_fifo_exit(asynchronous_fifo_state);

}
