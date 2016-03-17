#ifndef BLOCK_SERIAL_H_
#define BLOCK_SERIAL_H_

#include <stddef.h>
#include "app_config.h"
#include "main.h"

typedef interface block_transfer_if {
    void push(int * movable &p_buffer, const unsigned n_samps);
} block_transfer_if;

typedef interface serial_transfer_push_if {
    void push(int samples[], const size_t n_samps);
} serial_transfer_push_if;

typedef interface serial_transfer_pull_if {
    int pull(const size_t n_samps);
} serial_transfer_pull_if;

[[distributable]] void serial2block(server serial_transfer_push_if i_serial_in, client block_transfer_if i_block_transfer[ASRC_N_INSTANCES], server sample_rate_enquiry_if i_input_rate);
[[distributable]] unsafe void block2serial(server block_transfer_if i_block2serial[ASRC_N_INSTANCES], server serial_transfer_pull_if i_serial_out, server sample_rate_enquiry_if i_output_rate);


#endif /* BLOCK_SERIAL_H_ */
