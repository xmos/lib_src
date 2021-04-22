// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// FIRDS3 functions implementation file
//
// ===========================================================================
// ===========================================================================


// ===========================================================================
//
// Includes
//
// ===========================================================================
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "src.h"
#include "src_ff3_ds3.h"
#include "src_ff3_fir_inner_loop_asm.h"


// ===========================================================================
//
// Defines
//
// ===========================================================================


// State init value
#define        SRC_FF3_DS3_STATE_INIT                    0




// ===========================================================================
//
// Variables
//
// ===========================================================================

int src_ds3_coeffs[SRC_FF3_DS3_N_COEFS] = {
    #include SRC_FF3_DS3_COEFS_FILE
};

// ===========================================================================
//
// Local Functions prototypes
//
// ===========================================================================



// ===========================================================================
//
// Functions implementations
//
// ===========================================================================

src_ff3_return_code_t src_ds3_init(src_ds3_ctrl_t* src_ds3_ctrl)
{
    // Check that delay line base has been set
    if (src_ds3_ctrl->delay_base == 0) {
        return SRC_FF3_ERROR;
    }

    // Setup from FIRDS2 descriptor
    src_ds3_ctrl->delay_len       = SRC_FF3_DS3_N_COEFS<<1;                                    // Double length for circular buffer simulation
    src_ds3_ctrl->delay_wrap      = src_ds3_ctrl->delay_base + SRC_FF3_DS3_N_COEFS;
    src_ds3_ctrl->delay_offset    = SRC_FF3_DS3_N_COEFS;
    src_ds3_ctrl->inner_loops     = (SRC_FF3_DS3_N_COEFS>>1) / SRC_FF3_N_LOOPS_PER_ASM;        // Right shift to 2 x 32bits read for coefs per inner loop
    src_ds3_ctrl->num_coeffs      = SRC_FF3_DS3_N_COEFS;
    src_ds3_ctrl->coeffs          = src_ds3_coeffs;

    // Sync the FIRDS3
    if (src_ds3_sync(src_ds3_ctrl) != SRC_FF3_NO_ERROR) {
        return SRC_FF3_ERROR;
    }

    return SRC_FF3_NO_ERROR;
}

src_ff3_return_code_t src_ds3_sync(src_ds3_ctrl_t* src_ds3_ctrl)
{
    // Set delay line index back to base
    src_ds3_ctrl->delay_pos    = src_ds3_ctrl->delay_base;

    // Clear delay line
    for (unsigned int ui = 0; ui < src_ds3_ctrl->delay_len; ui++) {
        src_ds3_ctrl->delay_base[ui]    = SRC_FF3_DS3_STATE_INIT;
    }

    return SRC_FF3_NO_ERROR;
}

src_ff3_return_code_t src_ds3_proc(src_ds3_ctrl_t* src_ds3_ctrl)
{
    int*            data;
    int*            coeffs;
    int             data0;
    __int64_t       accumulator;

    // Get three new data samples to delay line (double write for circular buffer simulation)
    data0                    = *src_ds3_ctrl->in_data;
    *src_ds3_ctrl->delay_pos                                       = data0;
    *(src_ds3_ctrl->delay_pos + src_ds3_ctrl->delay_offset)        = data0;

    data0                    = *(src_ds3_ctrl->in_data + 1);
    *(src_ds3_ctrl->delay_pos + 1)                                 = data0;
    *(src_ds3_ctrl->delay_pos + src_ds3_ctrl->delay_offset + 1)    = data0;

    data0                    = *(src_ds3_ctrl->in_data + 2);
    *(src_ds3_ctrl->delay_pos + 2)                                 = data0;
    *(src_ds3_ctrl->delay_pos + src_ds3_ctrl->delay_offset + 2)    = data0;

    // Step delay with circular simulation (will also rewrite to control structure for next round)
    // Note as delay line length is a multiple of 3 (because filter coefficients length is a multiple of 3)
    // we only have to do the wrap around check after having written 3 input samples
    src_ds3_ctrl->delay_pos += 3;
    if (src_ds3_ctrl->delay_pos >= src_ds3_ctrl->delay_wrap) {
        src_ds3_ctrl->delay_pos = src_ds3_ctrl->delay_base;
    }

    // Clear accumulator and set access pointers
    data                    = src_ds3_ctrl->delay_pos;
    coeffs                  = src_ds3_ctrl->coeffs;
    accumulator             = 0;
    if ((unsigned)data & 0b0100) {
        src_ff3_fir_inner_loop_asm_odd(data, coeffs, &data0, src_ds3_ctrl->inner_loops);
    } else {
        src_ff3_fir_inner_loop_asm(data, coeffs, &data0, src_ds3_ctrl->inner_loops);
    }

    *src_ds3_ctrl->out_data = data0;

    return SRC_FF3_NO_ERROR;
}

