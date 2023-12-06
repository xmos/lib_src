#include <xcore/parallel.h>
#include <xcore/hwtimer.h>
#include <stdio.h>
#include <print.h>
#include <platform.h>
#include <xscope.h>
#include "asrc_block.h"


DECLARE_JOB(producer, (asrc_block_t *));
DECLARE_JOB(consumer, (asrc_block_t *));

int32_t input_data[48] = {
    0,
    70075715,
    138952416,
    205451603,
    268435456,
    326826303,
    379625062,
    425928331,
    464943848,
    496004047,
    518577479,
    532277906,
    536870912,
    532277906,
    518577479,
    496004047,
    464943848,
    425928331,
    379625062,
    326826303,
    268435455,
    205451603,
    138952416,
    70075715,
    0,
    -70075715,
    -138952416,
    -205451603,
    -268435456,
    -326826303,
    -379625062,
    -425928331,
    -464943848,
    -496004047,
    -518577479,
    -532277906,
    -536870912,
    -532277906,
    -518577479,
    -496004047,
    -464943848,
    -425928331,
    -379625062,
    -326826303,
    -268435455,
    -205451603,
    -138952416,
    -70075715,
};

#define seconds 10

void producer(asrc_block_t *a) {
    hwtimer_t tmr = hwtimer_alloc();
    int now = hwtimer_get_time(tmr);
    int freq = 48012 / 4;
    int step = 100000000 / freq;
    int mod = 100000000 % freq;
    int mod_acc = 0;
    int cntr = 0;
    
    for(int i = 0; i < 12000 * seconds; i++) {
        now += step;
        mod_acc += mod;
        if (mod_acc >= freq) {
            mod_acc -= freq;
            now++;
        }
        hwtimer_set_trigger_time(tmr, now);
        (void) hwtimer_get_time(tmr);
        asrc_add_quad_input(a, &input_data[cntr]);
        cntr = (cntr + 4) % 48;
    }
    hwtimer_free(tmr);
}

void consumer(asrc_block_t *a) {
    hwtimer_t tmr = hwtimer_alloc();
    int now = hwtimer_get_time(tmr);
    int freq = 48012;
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
        asrc_get_single_output(a, &output_data);
        xscope_int(0, output_data);
//        printintln(output_data>>20);
        if (i == 24000) {
            freq = 47993;
            step = 100000000 / freq;
            mod = 100000000 % freq;
        }
    }
    hwtimer_free(tmr);
}

#define FIFO_LENGTH   100

int main(void) {
    int32_t array[sizeof(asrc_block_t)/sizeof(int) + FIFO_LENGTH];
    asrc_block_t *block = (asrc_block_t *)array;

    asrc_init_buffer(block, 1, FIFO_LENGTH);
    PAR_JOBS(
        PJOB(producer, (block)),
        PJOB(consumer, (block))
        );
    asrc_exit_buffer(block);

}
