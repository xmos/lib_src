// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef BLOCK_SERIAL_H_
#define BLOCK_SERIAL_H_

#include <stddef.h>
#include "app_config.h"
#include "main.h"

typedef interface block_transfer_if {
    int * unsafe push(const unsigned n_samps);
} block_transfer_if;


typedef interface serial_transfer_pull_if {
    int pull(const size_t chan_idx);
} serial_transfer_pull_if;

typedef interface serial_transfer_push_if {
    void push(int sample, const size_t chan_idx);
} serial_transfer_push_if;


//Structure containing information about the FIFO
typedef struct b2s_fifo_t{
    int * unsafe ptr_base;
    int * unsafe ptr_top_max;
    int * unsafe ptr_top_curr;
    int * unsafe ptr_rd;
    int * unsafe ptr_wr;
    int size_curr;
    int fill_level;
} b2s_fifo_t;

#define PASS    1
#define FAIL    0

[[distributable]] void serial2block(server serial_transfer_push_if i_serial_in, client block_transfer_if i_block_transfer[ASRC_N_INSTANCES], server sample_rate_enquiry_if i_input_rate);
[[distributable]] unsafe void block2serial(server block_transfer_if i_block2serial[ASRC_N_INSTANCES], server serial_transfer_pull_if i_serial_out, server sample_rate_enquiry_if i_output_rate);


#endif /* BLOCK_SERIAL_H_ */
