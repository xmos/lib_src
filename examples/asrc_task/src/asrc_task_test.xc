// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// General includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <xs1.h>
#include <xscope.h>
#include <platform.h>

#include "asrc_task_config.h"
#include "asrc_task.h"

#define CMD_LEN     4   // Format is SR_IN, IN_CHANS, SR_OUT, POST_DELAY_MS
#define MAX_CMDS    128

#define INPUT_SAMPLING_RATE (44100)
#define OUTPUT_SAMPLING_RATE (96000)
#define NUM_CHANS (8)

void send_asrc_input_samples(chanend c_producer, int32_t samples[MAX_ASRC_CHANNELS_TOTAL], unsigned channel_count, unsigned sample_rate, int32_t time_stamp){

    outuint(c_producer, sample_rate);
    outuint(c_producer, time_stamp);
    outuint(c_producer, channel_count);
    for(int i = 0; i < channel_count; i++){
        outuint(c_producer,samples[i]);
    }
}

void producer(chanend c_producer){
    unsigned sample_rate = INPUT_SAMPLING_RATE;
    unsigned channel_count = NUM_CHANS;
    int32_t samples[MAX_ASRC_CHANNELS_TOTAL];

    #define N_SINE 44       // 1kHz at 44.1k, 4.36kHz at 192k
    #define AMPLITUDE 0.8
    int32_t sine[N_SINE];
    for(int i = 0; i < N_SINE; i++){
        sine[i] = (int32_t) ((double)((1<<31) - 1) * AMPLITUDE * sin(i * 2.0 * 3.14159265F / N_SINE));
        // printf("sin: %d %ld\n", i, sine[i]);
    }
    unsigned sine_counter = 0;


    timer t;
    int32_t time_trigger;
    t :> time_trigger;
    int32_t sample_period = XS1_TIMER_HZ / sample_rate;
    printf("Config producer %u %u\n", sample_rate, channel_count);

    while(1){
        select{
            case t when timerafter(time_trigger) :> int32_t time_stamp:
                for(int ch = 0; ch < channel_count; ch++) {
                    samples[ch] = sine[sine_counter % N_SINE];
                }
                sine_counter++;

                send_asrc_input_samples(c_producer, samples, channel_count, sample_rate, time_stamp);
                time_trigger += sample_period;
            break;
        }
    }
}

void consumer(asrc_in_out_t * unsafe asrc_io, asynchronous_fifo_t * unsafe fifo){
    unsigned sample_rate = OUTPUT_SAMPLING_RATE;
    int32_t samples[MAX_ASRC_CHANNELS_TOTAL];

    timer t;
    int32_t time_trigger;
    t :> time_trigger;
    int32_t sample_period = XS1_TIMER_HZ / sample_rate;

    while(1){
        select{
            case t when timerafter(time_trigger) :> int32_t time_stamp:
                pull_samples(asrc_io, fifo, samples, sample_rate, time_stamp);
                time_trigger += sample_period;
                xscope_int(0, samples[0]);
            break;
        }
    }
}

extern void setup_asrc_io_custom_callback(asrc_in_out_t *unsafe asrc_io);

int main(unsigned argc, char * unsafe argv[argc])
{
    chan c_producer;
    chan c_control[2];

    // FIFO and ASRC I/O declaration. Global to allow producer and consumer to access it
    #define FIFO_LENGTH     (SRC_MAX_NUM_SAMPS_OUT * 3) // Half full is target so *2 is nominal size but we need wiggle room at startup
    int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_LENGTH, MAX_ASRC_CHANNELS_TOTAL)];

    unsafe{
        // IO struct for ASRC must be passed to both asrc_proc and consumer
        asrc_in_out_t asrc_io = {{{0}}};
        asrc_in_out_t * unsafe asrc_io_ptr = &asrc_io;
        asynchronous_fifo_t * unsafe fifo = (asynchronous_fifo_t *)array;
        setup_asrc_io_custom_callback(asrc_io_ptr);

        par
        {
            producer(c_producer);
            asrc_task(c_producer, asrc_io_ptr, fifo, FIFO_LENGTH);
            consumer(asrc_io_ptr, fifo);

        }
    } // unsafe region
    return 0;
}
