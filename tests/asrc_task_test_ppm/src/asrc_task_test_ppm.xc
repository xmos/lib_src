// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// General includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <xs1.h>
#include <xscope.h>
#include <platform.h>

#include "asrc_task_config.h"
#include "asrc_task.h"

#define MAX_FIFO_LENGTH     (SRC_MAX_NUM_SAMPS_OUT * 128)


void send_asrc_input_samples(chanend c_producer, int32_t samples[MAX_ASRC_CHANNELS_TOTAL], unsigned channel_count, unsigned sample_rate, int32_t time_stamp){

    outuint(c_producer, sample_rate);
    outuint(c_producer, time_stamp);
    outuint(c_producer, channel_count);
    for(int i = 0; i < channel_count; i++){
        outuint(c_producer,samples[i]);
    }
}

void producer(chanend c_producer, unsigned sample_rate, chanend c_latency_measure){
    unsigned channel_count = MAX_ASRC_CHANNELS_TOTAL;
    int32_t samples[MAX_ASRC_CHANNELS_TOTAL];

    #define N_SINE 44       // 1kHz at 44.1k, 4.36kHz at 192k
    #define AMPLITUDE 0.8
    int32_t sine[N_SINE];
    for(int i = 0; i < N_SINE; i++){
        sine[i] = (int32_t) ((double)((1<<31) - 1) * AMPLITUDE * sin(i * 2.0 * 3.14159265F / N_SINE));
        // printf("sin: %d %ld\n", i, sine[i]);
    }
    unsigned sine_counter = 0;
    int do_step = 0;


    timer t;
    int32_t time_trigger;
    t :> time_trigger;
    int32_t sample_period = XS1_TIMER_HZ / sample_rate;

    while(1){
        select{
            case t when timerafter(time_trigger) :> int32_t time_stamp:
                for(int ch = 0; ch < channel_count; ch++){
#if DO_SINE
                    samples[ch] = sine[sine_counter % N_SINE];
#else
                    samples[ch] = 0;
#endif
                    if (do_step){
                        samples[ch] = 0x60000000; // 3/4 scale
                    }
                }
                sine_counter++;

                send_asrc_input_samples(c_producer, samples, channel_count, sample_rate, time_stamp);
                time_trigger += sample_period;
            break;

            case c_latency_measure :> int _:
                do_step = 1;
            break; 
        }
    }
}

void consumer(  unsigned test_len_s,
                unsigned sample_rate_nominal,
                unsigned sample_rate_actual,
                asrc_in_out_t * unsafe asrc_io,
                asynchronous_fifo_t * unsafe fifo,
                chanend c_latency_measure){
 
    int32_t samples[MAX_ASRC_CHANNELS_TOTAL];

    // Setup timers
    timer t;
    int32_t time_trigger;
    t :> time_trigger;
    int32_t sample_period = XS1_TIMER_HZ / sample_rate_actual;
    assert(test_len_s < 40);
    int32_t time_end = time_trigger + (int32_t)(test_len_s * XS1_TIMER_HZ); // Warning - 40s max to avoid wrap 

    // For sending a trigger back to the producer
    unsigned sample_count = 0;
    int done = 0;

    // Don't report zero FIFO at start
    int fifo_init = 0;

    while(1){
        select{
            case t when timerafter(time_trigger) :> int32_t time_stamp:
                pull_samples(asrc_io, fifo, samples, sample_rate_nominal, time_stamp);
                time_trigger += sample_period;
                unsafe{
                    for(int ch = 0; ch < MAX_ASRC_CHANNELS_TOTAL; ch++){
                        xscope_int(ch, samples[ch]);
                        delay_microseconds(1); // Allow xscope write to complete
                    }
                    xscope_int(MAX_ASRC_CHANNELS_TOTAL, fifo->last_phase_error);
                    delay_microseconds(1); // Allow xscope write to complete
                    int fifo_level = (fifo->write_ptr - fifo->read_ptr + fifo->max_fifo_depth) % fifo->max_fifo_depth;
                    if (fifo_level != 0){
                        fifo_init = 1;
                    }
                    xscope_int(MAX_ASRC_CHANNELS_TOTAL + 1, fifo_init ? fifo_level : fifo->max_fifo_depth / 2);

                }
#if DO_STEP
                if(sample_count >= sample_rate_nominal * 9 && !done){
                    c_latency_measure <: 0;
                    done = 1;
                }
#endif
                sample_count ++;

                if(timeafter(time_stamp, time_end)) {
                    delay_milliseconds(100); // Ensure last xscope write finishes
                    _Exit(0);
                }
            break;
        }
    }
}

unsigned parse_cmd_line(    unsigned &sample_rate_producer,
                            unsigned &sample_rate_consumer_nominal,
                            unsigned &sample_rate_consumer_actual,
                            unsigned &test_len_s,
                            unsigned &fifo_len,
                            unsigned argc,
                            char * unsafe argv[argc])
{
    for(int i = 1; i < argc; i++){
        printf("Arg %u: %s\n", i, argv[i]);
        unsigned val = (unsigned)(atoi((char *)argv[i]));
        switch(i){
            case 1:
                sample_rate_producer = val;
            break;
            case 2:
                sample_rate_consumer_nominal = val;
            break;
            case 3:
                sample_rate_consumer_actual = val;
            break;
            case 4:
                test_len_s = val;
            break;
            case 5:
                fifo_len = val;
            break;
            default:
                printf("Error: unexpected arg: %u\n", val);
            break;
        }
    }

    return (argc - 1);
}

extern void setup_asrc_io_custom_callback(asrc_in_out_t *unsafe asrc_io);

int main(unsigned argc, char * unsafe argv[argc])
{
    chan c_producer;

    chan c_latency_measure;

    xscope_mode_lossless();

    // FIFO and ASRC I/O declaration. Global to allow producer and consumer to access it
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(MAX_FIFO_LENGTH, MAX_ASRC_CHANNELS_TOTAL)] = {0};

    unsafe{
        unsigned sample_rate_producer = 0;
        unsigned sample_rate_consumer_nominal = 0;
        unsigned sample_rate_consumer_actual = 0;
        unsigned test_len_s = 0;
        unsigned fifo_len = 0;

        parse_cmd_line(sample_rate_producer, sample_rate_consumer_nominal, sample_rate_consumer_actual, test_len_s, fifo_len, argc, argv);

        printf("Setting ASRC to in_sr: %u out_sr_nominal: %u out_sr_actual: %u test_len_s: %u fifo_len: %u\n", sample_rate_producer, sample_rate_consumer_nominal, sample_rate_consumer_actual, test_len_s, fifo_len);
        assert(fifo_len <= MAX_FIFO_LENGTH); // per channel


        // IO struct for ASRC must be passed to both asrc_proc and consumer
        asrc_in_out_t asrc_io = {{{0}}};
        asrc_in_out_t * unsafe asrc_io_ptr = &asrc_io;
        asynchronous_fifo_t * unsafe fifo = (asynchronous_fifo_t *)array;
        setup_asrc_io_custom_callback(asrc_io_ptr);


        par
        {
            producer(c_producer, sample_rate_producer, c_latency_measure);
            asrc_task(c_producer, asrc_io_ptr, fifo, fifo_len);
            consumer(test_len_s, sample_rate_consumer_nominal, sample_rate_consumer_actual, asrc_io_ptr, fifo, c_latency_measure);

        }
    } // unsafe region
    return 0;
}
