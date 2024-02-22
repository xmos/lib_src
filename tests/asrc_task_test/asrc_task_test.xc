// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// General includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xs1.h>
#include <math.h>

#include "asrc_task_config.h"
#include "asrc_task.h"


const unsigned sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000};


void test_master(chanend c_control[2]){
    delay_milliseconds(1); // Test startup safe
    c_control[0] <: 44100;
    c_control[0] <: 4;
    delay_milliseconds(1); // Test startup safe
    c_control[1] <: 44100;
}

unsigned receive_asrc_input_samples(chanend c_producer, asrc_in_out_t &asrc_io, unsigned &new_input_rate){
    static unsigned asrc_in_counter = 0;

    new_input_rate = inuint(c_producer);
    asrc_io.input_timestamp = inuint(c_producer);
    asrc_io.input_channel_count = inuint(c_producer);

    // Pack into array properly LRLRLRLR or 123412341234 etc.
    for(int i = 0; i < asrc_io.input_channel_count; i++){
        int idx = i + asrc_io.input_channel_count * asrc_in_counter;
        asrc_io.input_samples[asrc_io.input_write_idx][idx] = inuint(c_producer);
    }

    if(++asrc_in_counter == SRC_N_IN_SAMPLES){
        asrc_in_counter = 0;
    }

    return asrc_in_counter;
}

void send_asrc_input_samples(chanend c_producer, int32_t samples[MAX_ASRC_CHANNELS_TOTAL], unsigned channel_count, unsigned sample_rate, int32_t time_stamp){

    outuint(c_producer, sample_rate);
    outuint(c_producer, time_stamp);
    outuint(c_producer, channel_count);
    for(int i = 0; i < channel_count; i++){
        outuint(c_producer,samples[i]);
    }
}

void producer(chanend c_producer, chanend c_control){
    unsigned sample_rate = 0;
    unsigned channel_count = 0;
    int32_t samples[MAX_ASRC_CHANNELS_TOTAL];

    #define N_SINE 100
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
    int32_t sample_period = 0;

    while(1){
        select{
            case c_control :> sample_rate:
                c_control :> channel_count;
                sample_period = XS1_TIMER_HZ / sample_rate;
                printf("Config producer %u %u\n", sample_rate, channel_count);
            break;

            case sample_rate => t when timerafter(time_trigger) :> int32_t time_stamp:
                for(int ch = 0; ch < channel_count; ch++){
                    samples[ch] = sine[sine_counter];
                }
                sine_counter++;
                if(sine_counter == N_SINE){
                    sine_counter = 0;
                }

                send_asrc_input_samples(c_producer, samples, channel_count, sample_rate, time_stamp);
                time_trigger += sample_period;
                printf("PUSH: %ld\n", samples[0]);
            break;
        }
    }
}

void consumer(chanend c_control){
    unsigned sample_rate = 0;
    int32_t samples[MAX_ASRC_CHANNELS_TOTAL];

    timer t;
    int32_t time_trigger;
    t :> time_trigger;
    int32_t sample_period = 0;

    while(1){
        select{
            case c_control :> sample_rate:
                sample_period = XS1_TIMER_HZ / sample_rate;
                printf("Config producer %u\n", sample_rate);
            break;

            case sample_rate => t when timerafter(time_trigger) :> int32_t time_stamp:
                pull_samples(samples, sample_rate, time_stamp);
                time_trigger += sample_period;
                printf("PULL: %ld\n", samples[0]);
            break;
        }
    }
}


int main(void)
{
    chan c_producer;
    chan c_control[2];

    par
    {
        producer(c_producer, c_control[0]);
        asrc_processor(c_producer);
        consumer(c_control[1]);
        test_master(c_control);

    }
    return 0;
}
