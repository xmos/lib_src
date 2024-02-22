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


// const unsigned sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000};
const unsigned sample_rates[] = {44100, 48000, 88200, 96000};


void test_master(chanend c_control[2]){
    xscope_mode_lossless();

    delay_milliseconds(1); // Test startup safe

    const int order = 2;

    // for(int consumer_start_first = 0; consumer_start_first < 2; consumer_start_first++){
    for(int consumer_start_first = 0; consumer_start_first < 1; consumer_start_first++){
        // for(int consumer_finish_first = 0; consumer_finish_first < 2; consumer_finish_first++){
        for(int consumer_finish_first = 0; consumer_finish_first < 1; consumer_finish_first++){
            // for(unsigned channel_count = 1; channel_count < MAX_ASRC_CHANNELS_TOTAL; channel_count++){
            for(unsigned channel_count = 1; channel_count < 2; channel_count++){
                for(int i = 0; i < sizeof(sample_rates) / sizeof(sample_rates[0]); i++){
                    for(int o = 0; o < sizeof(sample_rates) / sizeof(sample_rates[0]); o++){

                        if(consumer_start_first){
                            c_control[1] <: sample_rates[o];
                            delay_milliseconds(1); // Test startup safe
                            c_control[0] <: sample_rates[i];
                            c_control[0] <: channel_count;
                        } else {
                            c_control[0] <: sample_rates[i];
                            c_control[0] <: channel_count;
                            delay_milliseconds(1); // Test startup safe
                            c_control[1] <: sample_rates[o];
                        }

                        delay_milliseconds(250); // Run test for a bit

                        if(consumer_finish_first){
                            c_control[1] <: 0;
                            delay_milliseconds(1); // Test shutdown safe
                            c_control[0] <: 0;
                            c_control[0] <: 0;
                        } else {
                            c_control[0] <: 0;
                            c_control[0] <: 0;
                            delay_milliseconds(1); // Test shutdown safe
                            c_control[1] <: 0;
                        }
                    }// output F
                }// input F
            }// channels
        }// consumer_finish
    }//consumer_start
    _Exit(0);
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
    int32_t sample_period = XS1_TIMER_HZ / 192000;

    while(1){
        select{
            case c_control :> sample_rate:
                c_control :> channel_count;
                if(sample_rate){
                    sample_period = XS1_TIMER_HZ / sample_rate;
                } else {
                    sample_period = XS1_TIMER_HZ / 192000;
                }
                printf("Config producer %u %u\n", sample_rate, channel_count);
            break;

            case t when timerafter(time_trigger) :> int32_t time_stamp:
                for(int ch = 0; ch < channel_count; ch++){
                    samples[ch] = sine[sine_counter];
                }
                sine_counter++;
                if(sine_counter == N_SINE){
                    sine_counter = 0;
                }

                send_asrc_input_samples(c_producer, samples, channel_count, sample_rate, time_stamp);
                time_trigger += sample_period;
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
    int32_t sample_period = XS1_TIMER_HZ / 192000;

    while(1){
        select{
            case c_control :> sample_rate:
                if(sample_rate){
                    sample_period = XS1_TIMER_HZ / sample_rate;
                } else {
                    sample_period = XS1_TIMER_HZ / 192000;
                }
                printf("Config consumer %u\n", sample_rate);
            break;

            case t when timerafter(time_trigger) :> int32_t time_stamp:
                pull_samples(samples, sample_rate, time_stamp);
                time_trigger += sample_period;
                xscope_int(0, samples[0]);
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
