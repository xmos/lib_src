#include <xcore/parallel.h>
#include <xcore/hwtimer.h>
#include <stdio.h>
#include <print.h>
#include <platform.h>
#include <xscope.h>
#include "asynchronous_fifo.h"

DECLARE_JOB(producer, (asynchronous_fifo_t *));
DECLARE_JOB(consumer, (asynchronous_fifo_t *));

#define seconds 10
#define OFFSET 0 // 0x70000000
#define FREQ_Q_SHIFT  8
#define FREQ_Q_VALUE  (1 << FREQ_Q_SHIFT)

void producer(asynchronous_fifo_t *a) {
    hwtimer_t tmr = hwtimer_alloc();
    uint64_t now = hwtimer_get_time(tmr);
    int freq = 48012 * FREQ_Q_VALUE ;
    int step = 100000000 * (int64_t) FREQ_Q_VALUE / freq;
    int mod = 100000000 * (int64_t) FREQ_Q_VALUE % freq;
    int mod_acc = 0;
    
    for(int32_t i = 0; i < 48000 * seconds; i++) {
        xscope_int(5, freq);
        now += step;
        mod_acc += mod;
        if (mod_acc >= freq) {
            mod_acc -= freq;
            now++;
        }
        hwtimer_set_trigger_time(tmr, now);
        (void) hwtimer_get_time(tmr);
        int32_t error = asynchronous_fifo_produce(a, &i, now+OFFSET, 1);
        freq = 48012 * FREQ_Q_VALUE - ((error * 48012LL * FREQ_Q_VALUE + 0x80000000LL) >> 32) ;
        step = 100000000 * (int64_t) FREQ_Q_VALUE / freq;
        mod = 100000000 * (int64_t) FREQ_Q_VALUE % freq;
    }
    hwtimer_free(tmr);
}

void consumer(asynchronous_fifo_t *a) {
    hwtimer_t tmr = hwtimer_alloc();
    uint64_t now = hwtimer_get_time(tmr);
    int freq = 48012;                  // Very close to 48012
    int step = 100000000 / freq;
    int mod = 100000000 % freq;
    int mod_acc = 0;
    int32_t output_data;
    
    for(int i = 0; i < 48000 * seconds; i++) {
        now += step;
        mod_acc += mod;
        if (mod_acc >= freq) {
            mod_acc -= freq;
            now++;
        }
        hwtimer_set_trigger_time(tmr, now);
        (void) hwtimer_get_time(tmr);
//        printchar('*');
        asynchronous_fifo_consume(a, &output_data, now + OFFSET);
        xscope_int(0, output_data);
        if (i == 2205) {
            freq = 47993;
            step = 100000000 / freq;
            mod = 100000000 % freq;
        }
    }
    hwtimer_free(tmr);
}

#define FIFO_LENGTH   100

int main(void) {
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_LENGTH, 1)];
    asynchronous_fifo_t *asynchronous_fifo_state = (asynchronous_fifo_t *)array;

    asynchronous_fifo_init(asynchronous_fifo_state, 1, FIFO_LENGTH,
                           100000000/48000);
    PAR_JOBS(
        PJOB(producer, (asynchronous_fifo_state)),
        PJOB(consumer, (asynchronous_fifo_state))
        );
    asynchronous_fifo_exit(asynchronous_fifo_state);

}


