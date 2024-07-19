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


void test_master(chanend c_control[2], unsigned commands[MAX_CMDS][CMD_LEN], unsigned n_cmds){
    xscope_mode_lossless();

    delay_milliseconds(1); // Test startup safe

    // Format is SR_IN, IN_CHANS, SR_OUT, POST_DELAY_MS 
    for(unsigned i = 0; i < n_cmds; i++){
        printf("Setting ASRC to in_sr: %u n_chans: %u out_sr: %u post_delay_ms: %u\n",
                commands[i][0],
                commands[i][1],
                commands[i][2],
                commands[i][3]);
        c_control[0] <: commands[i][0];
        c_control[0] <: commands[i][1];
        c_control[1] <: commands[i][2];
        
        delay_milliseconds(commands[i][3]); // Test startup safe     
    }
    printf("Normal exit: no more commands\n");
    delay_milliseconds(100); // Ensure last xscope write finishes
    _Exit(0);
}

void send_asrc_input_samples(chanend c_producer, int32_t samples[MAX_ASRC_CHANNELS_TOTAL], unsigned channel_count, unsigned sample_rate, int32_t time_stamp){

    outuint(c_producer, sample_rate);
    outuint(c_producer, time_stamp);
    outuint(c_producer, channel_count);
    for(int i = 0; i < channel_count; i++){
        outuint(c_producer,samples[i]);
    }
}

void producer(chanend c_producer, chanend c_control, const unsigned multi_tone){
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
                    if(multi_tone){
                        samples[ch] = sine[(sine_counter * (ch + 1)) % N_SINE];
                    } else {
                        samples[ch] = sine[sine_counter % N_SINE];
                    }
                }
                sine_counter++;

                send_asrc_input_samples(c_producer, samples, channel_count, sample_rate, time_stamp);
                time_trigger += sample_period;
            break;
        }
    }
}

void consumer(chanend c_control, asrc_in_out_t * unsafe asrc_io, asynchronous_fifo_t * unsafe fifo, const unsigned multi_tone){
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
                pull_samples(asrc_io, fifo, samples, sample_rate, time_stamp);
                time_trigger += sample_period;
                if(!multi_tone){
                    xscope_int(0, samples[0]);
                } else unsafe{
                    for(int ch = 0; ch < asrc_io->asrc_channel_count; ch++){
                        xscope_int(ch, samples[ch]);
                    }
                }
            break;
        }
    }
}

unsigned parse_cmd_line(unsigned commands[MAX_CMDS][CMD_LEN], unsigned &multi_tone, unsigned argc, char * unsafe argv[argc])
{
    multi_tone = atoi((char*)argv[1]);

    for(int i = 2; i < argc; i++){
        printf("Arg %u: %s\n", i, argv[i]);
        unsigned val = (unsigned)(atoi((char *)argv[i]));
        unsigned cmd_idx = (i - 2) % CMD_LEN;
        unsigned cmd_n = (i - 2) / CMD_LEN;
        commands[cmd_n][cmd_idx] = val;
    }

    return (argc - 1) / CMD_LEN;
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


        // Format is SR_IN, IN_CHANS, SR_OUT, POST_DELAY_MS 
        unsigned commands[MAX_CMDS][CMD_LEN] = {{0}};
        unsigned multi_tone = 0;
        unsigned n_cmds = parse_cmd_line(commands, multi_tone, argc, argv);

        par
        {
            test_master(c_control, commands, n_cmds);
            producer(c_producer, c_control[0], multi_tone);
            asrc_task(c_producer, asrc_io_ptr, fifo, FIFO_LENGTH);
            consumer(c_control[1], asrc_io_ptr, fifo, multi_tone);

        }
    } // unsafe region
    return 0;
}
