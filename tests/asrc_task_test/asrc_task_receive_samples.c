// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "asrc_task.h"

ASRC_TASK_ISR_CALLBACK_ATTR
unsigned receive_asrc_input_samples(chanend_t c_producer, asrc_in_out_t *asrc_io, unsigned *new_input_rate){
    static unsigned asrc_in_counter = 0;

    // Receive stream info from producer
    *new_input_rate = chanend_in_word(c_producer);
    asrc_io->input_timestamp = chanend_in_word(c_producer);
    asrc_io->input_channel_count = chanend_in_word(c_producer);

    // Pack into array properly LRLRLRLR or 123412341234 etc.
    for(int i = 0; i < asrc_io->input_channel_count; i++){
        int idx = i + asrc_io->input_channel_count * asrc_in_counter;
        asrc_io->input_samples[asrc_io->input_write_idx][idx] = chanend_in_word(c_producer);
    }

    // Keep track of frame block to ASRC task
    if(++asrc_in_counter == SRC_N_IN_SAMPLES){
        asrc_in_counter = 0;
    }

    return asrc_in_counter;
}

// Register the above function for ASRC task
void setup_asrc_io_custom_callback(asrc_in_out_t *asrc_io){
    init_asrc_io_callback(asrc_io, receive_asrc_input_samples);
}