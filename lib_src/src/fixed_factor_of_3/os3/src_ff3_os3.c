// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// FIROS3 functions implementation file
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
#include "src_ff3_os3.h"
#include "src_ff3_fir_inner_loop_asm.h"

// ===========================================================================
//
// Defines
//
// ===========================================================================

// State init value
#define        SRC_OS3_STATE_INIT                        0

// ===========================================================================
//
// Variables
//
// ===========================================================================

int                    src_os3_coeffs[SRC_FF3_OS3_N_COEFS]     = {
    #include SRC_FF3_OS3_COEFS_FILE
};

// ===========================================================================
//
// Functions implementations
//
// ===========================================================================

src_ff3_return_code_t src_os3_init(src_os3_ctrl_t* src_os3_ctrl)
{
    // Check that delay line base has been set
    if (src_os3_ctrl->delay_base == 0) {
        return SRC_FF3_ERROR;
    }

    // Setup from FIROS2 descriptor
    src_os3_ctrl->delay_len         = (SRC_FF3_OS3_N_COEFS/SRC_FF3_OS3_N_PHASES)<<1;                        // Double length for circular buffer simulation. x3 over-sampler, so only 1/3rd of coefs length needed
    src_os3_ctrl->delay_wrap        = src_os3_ctrl->delay_base + (SRC_FF3_OS3_N_COEFS/SRC_FF3_OS3_N_PHASES);
    src_os3_ctrl->delay_offset      = (SRC_FF3_OS3_N_COEFS/SRC_FF3_OS3_N_PHASES);
    src_os3_ctrl->inner_loops       = ((SRC_FF3_OS3_N_COEFS/SRC_FF3_OS3_N_PHASES)>>1) / SRC_FF3_N_LOOPS_PER_ASM;    // Right shift due to 2 x 32bits read for coefs per inner loop and x3 over-sampler, so only 1/3rd of coefs length needed
    src_os3_ctrl->num_coeffs        = SRC_FF3_OS3_N_COEFS;
    src_os3_ctrl->coeffs            = src_os3_coeffs;

    // Sync the FIROS3
    if (src_os3_sync(src_os3_ctrl) != SRC_FF3_NO_ERROR) {
        return SRC_FF3_ERROR;
    }

    return SRC_FF3_NO_ERROR;
}

src_ff3_return_code_t src_os3_sync(src_os3_ctrl_t* src_os3_ctrl)
{
    // Set delay line index back to base
    src_os3_ctrl->delay_pos = src_os3_ctrl->delay_base;

    // Clear delay line
    for (unsigned int ui = 0; ui < src_os3_ctrl->delay_len; ui++) {
        src_os3_ctrl->delay_base[ui]    = SRC_OS3_STATE_INIT;
    }

    // Set phase to '0' to indicate that a new input sample is needed
    src_os3_ctrl->phase = 0;

    return SRC_FF3_NO_ERROR;
}

src_ff3_return_code_t src_os3_input(src_os3_ctrl_t* src_os3_ctrl)
{
    // Write new input sample from control structure to delay line
    // at both positions for circular buffer simulation
    *src_os3_ctrl->delay_pos                                     = src_os3_ctrl->in_data;
    *(src_os3_ctrl->delay_pos + src_os3_ctrl->delay_offset)      = src_os3_ctrl->in_data;

    // Step delay index with circular buffer simulation
    src_os3_ctrl->delay_pos                += 1;
    if (src_os3_ctrl->delay_pos >= src_os3_ctrl->delay_wrap) {
        src_os3_ctrl->delay_pos = src_os3_ctrl->delay_base;
    }

    return SRC_FF3_NO_ERROR;
}

src_ff3_return_code_t                src_os3_proc(src_os3_ctrl_t* src_os3_ctrl)
{
    int*          data;
    int*          coeffs;
    int           data0;
    __int64_t     accumulator;

    // Clear accumulator and set data/coefs access pointers
    data            = src_os3_ctrl->delay_pos;
    coeffs          = src_os3_ctrl->coeffs + src_os3_ctrl->phase;
    accumulator     = 0;

    if ((unsigned)data & 0b0100) {
        src_ff3_fir_inner_loop_asm_odd(data, coeffs, &data0, src_os3_ctrl->inner_loops);
    } else {
        src_ff3_fir_inner_loop_asm(data, coeffs, &data0, src_os3_ctrl->inner_loops);
    }

    // Write output
    src_os3_ctrl->out_data = data0;

    // Step phase
    src_os3_ctrl->phase += (SRC_FF3_OS3_N_COEFS/SRC_FF3_OS3_N_PHASES);
    if (src_os3_ctrl->phase == SRC_FF3_OS3_N_COEFS) {
        src_os3_ctrl->phase = 0;
    }

    return SRC_FF3_NO_ERROR;
}

