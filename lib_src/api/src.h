// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _SRC_H_
#define _SRC_H_
#include "src_mrhf_ssrc.h"
#include "src_mrhf_asrc.h"
#include "src_ff3_ds3.h"
#include "src_ff3_os3.h"
#include "src_ff3v_fir.h"
#if (defined(__XS3A__)) // Only available for XS3 with VPU
#include "src_poly.h"
#endif // __XS3A__
#include <stdint.h>

#if defined(__cplusplus) || defined(__XC__)
extern "C" {
#endif

/** List of sample rate codes allowing the core SRC algorithms to use an index value */
typedef enum fs_code_t {
    FS_CODE_44 = 0,
    FS_CODE_48 = 1,
    FS_CODE_88 = 2,
    FS_CODE_96 = 3,
    FS_CODE_176 = 4,
    FS_CODE_192 = 5
} fs_code_t;

/** Flag to switch dither processing to 24b (from 32b) on or off */
typedef enum dither_flag_t {
    OFF = 0,
    ON = 1
} dither_flag_t;

/**
 * \addtogroup src_ssrc src_ssrc
 *
 * The public API for using SSRC.
 * @{
 */

/** initializes synchronous sample rate conversion instance.
 *  \param   sr_in                    Nominal sample rate code of input stream
 *  \param   sr_out                   Nominal sample rate code of output stream
 *  \param   ssrc_ctrl                Reference to array of SSRC control stuctures
 *  \param   n_channels_per_instance  Number of channels handled by this instance of SSRC
 *  \param   n_in_samples             Number of input samples per SSRC call
 *  \param   dither_on_off            Dither to 24b on/off
 */
void ssrc_init(const fs_code_t sr_in, const fs_code_t sr_out, ssrc_ctrl_t ssrc_ctrl[],
               const unsigned n_channels_per_instance, const unsigned n_in_samples,
               const dither_flag_t dither_on_off);

/** Perform synchronous sample rate conversion processing on block of input samples using previously initialized settings.
 *
 *  \param   in_buff          Reference to input sample buffer array
 *  \param   out_buff         Reference to output sample buffer array
 *  \param   ssrc_ctrl        Reference to array of SSRC control stuctures
 *  \returns The number of output samples produced by the SRC operation
 */
unsigned ssrc_process(int in_buff[], int out_buff[], ssrc_ctrl_t ssrc_ctrl[]);

/**@}*/ // END: addtogroup src_ssrc

/**
 * \addtogroup src_asrc src_asrc
 *
 * The public API for using ASRC.
 * @{
 */

/** initializes asynchronous sample rate conversion instance.
 *
 *  \param   sr_in           Nominal sample rate code of input stream
 *  \param   sr_out          Nominal sample rate code of output stream
 *  \param   asrc_ctrl       Reference to array of ASRC control structures
 *  \param   n_channels_per_instance  Number of channels handled by this instance of SSRC
 *  \param   n_in_samples             Number of input samples per SSRC call
 *  \param   dither_on_off            Dither to 24b on/off
 *  \returns The nominal sample rate ratio of in to out in Q4.60 format
 */
uint64_t asrc_init(const fs_code_t sr_in, const fs_code_t sr_out,
                   asrc_ctrl_t asrc_ctrl[], const unsigned n_channels_per_instance,
                   const unsigned n_in_samples, const dither_flag_t dither_on_off);

/** Perform asynchronous sample rate conversion processing on block of input samples using previously initialized settings.
 *
 *  \param   in_buff          Reference to input sample buffer array
 *  \param   out_buff         Reference to output sample buffer array
 *  \param   fs_ratio         Fixed point ratio of in/out sample rates in Q4.60 format
 *  \param   asrc_ctrl        Reference to array of ASRC control structures
 *  \returns The number of output samples produced by the SRC operation.
 */
unsigned asrc_process(int in_buff[], int out_buff[], uint64_t fs_ratio,
                      asrc_ctrl_t asrc_ctrl[]);

/**@}*/ // END: addtogroup src_asrc


// To avoid C type definitions when including this file from assembler
#ifndef INCLUDE_FROM_ASM

/** Fixed factor of 3 return codes
 *
 * This type describes the possible error status states from calls to the DS3
 * and OS3 API.
 */
typedef enum src_ff3_return_code_t
{
    SRC_FF3_NO_ERROR = 0,
    SRC_FF3_ERROR    = 1
} src_ff3_return_code_t;

/** Downsample by 3 control structure */
typedef struct src_ds3_ctrl_t
{
    int*         in_data;      //!< Pointer to input data (3 samples)
    int*         out_data;     //!< Pointer to output data (1 sample)
    int*         delay_base;   //!< Pointer to delay line base
    unsigned int delay_len;    //!< Total length of delay line
    int*         delay_pos;    //!< Pointer to current position in delay line
    int*         delay_wrap;   //!< Delay buffer wrap around address (for circular buffer simulation)
    unsigned int delay_offset; //!< Delay line offset for second write (for circular buffer simulation)
    unsigned int inner_loops;  //!< Number of inner loop iterations
    unsigned int num_coeffs;   //!< Number of coefficients
    int*         coeffs;       //!< Pointer to coefficients
} src_ds3_ctrl_t;

/** This function initializes the decimate by 3 function for a given instance
 *
 *  \param      src_ds3_ctrl   DS3 control structure
 *  \returns    SRC_FF3_NO_ERROR on success, SRC_FF3_ERROR on failure
 */
src_ff3_return_code_t src_ds3_init(src_ds3_ctrl_t* src_ds3_ctrl);

/** This function clears the decimate by 3 delay line for a given instance
 *
 *  \param      src_ds3_ctrl   DS3 control structure
 *  \returns    SRC_FF3_NO_ERROR on success, SRC_FF3_ERROR on failure
 */
src_ff3_return_code_t src_ds3_sync(src_ds3_ctrl_t* src_ds3_ctrl);

/** This function performs the decimation on three input samples and outputs one sample.
 *  The input and output buffers are pointed to by members of the src_ds3_ctrl structure
 *
 *  \param      src_ds3_ctrl   DS3 control structure
 *  \returns    SRC_FF3_NO_ERROR on success, SRC_FF3_ERROR on failure
 */
src_ff3_return_code_t src_ds3_proc(src_ds3_ctrl_t* src_ds3_ctrl);

/** Oversample by 3 control structure */
typedef struct src_os3_ctrl_t
{
    int          in_data;      //!< Input data (to be updated every 3 output samples, i.e. when iPhase == 0)
    int          out_data;     //!< Output data (1 sample)
    int          phase;        //!< Current output phase (when reaching '0', a new input sample is required)
    int*         delay_base;   //!< Pointer to delay line base
    unsigned int delay_len;    //!< Total length of delay line
    int*         delay_pos;    //!< Pointer to current position in delay line
    int*         delay_wrap;   //!< Delay buffer wrap around address (for circular buffer simulation)
    unsigned int delay_offset; //!< Delay line offset for second write (for circular buffer simulation)
    unsigned int inner_loops;  //!< Number of inner loop iterations
    unsigned int num_coeffs;   //!< Number of coefficients
    int*         coeffs;       //!< Pointer to coefficients
} src_os3_ctrl_t;

/** This function initializes the oversample by 3 function for a given instance
 *
 *  \param      src_os3_ctrl   OS3 control structure
 *  \returns    SRC_FF3_NO_ERROR on success, SRC_FF3_ERROR on failure
 */
src_ff3_return_code_t src_os3_init(src_os3_ctrl_t* src_os3_ctrl);

/** This function clears the oversample by 3 delay line for a given instance
 *
 *  \param      src_os3_ctrl   OS3 control structure
 *  \returns    SRC_FF3_NO_ERROR on success, SRC_FF3_ERROR on failure
 */
src_ff3_return_code_t src_os3_sync(src_os3_ctrl_t* src_os3_ctrl);

/** This function pushes a single input sample into the filter.
 *  It should be called three times for each FIROS3_proc call
 *
 *  \param      src_os3_ctrl   OS3 control structure
 *  \returns    SRC_FF3_NO_ERROR on success, SRC_FF3_ERROR on failure
 */
src_ff3_return_code_t src_os3_input(src_os3_ctrl_t* src_os3_ctrl);

/** This function performs the oversampling by 3 and outputs one sample.
 *  The input and output buffers are pointed to by members of the src_os3_ctrl structure
 *
 *  \param      src_os3_ctrl   OS3 control structure
 *  \returns    SRC_FF3_NO_ERROR on success, SRC_FF3_ERROR on failure
 */
src_ff3_return_code_t src_os3_proc(src_os3_ctrl_t* src_os3_ctrl);

#endif // INCLUDE_FROM_ASM

#if defined(__cplusplus) || defined(__XC__)
}
#endif

/** This function performs the first two iterations of the downsampling process
 *
 *  \param      sum      Partially accumulated value returned during previous cycle
 *  \param      data     Data delay line
 *  \param      coefs    FIR filter coefficients
 *  \param      sample   The newest sample
 *  \returns    Partially accumulated value, passed as ``sum`` parameter next cycle
 */
int64_t src_ds3_voice_add_sample(int64_t sum, int32_t data[],
                                 const int32_t coefs[], int32_t sample);

/** This function performs the final iteration of the downsampling process
 *
 *  \param      sum      Partially accumulated value returned during previous cycle
 *  \param      data     Data delay line
 *  \param      coefs    FIR filter coefficients
 *  \param      sample   The newest sample
 *  \returns    The decimated sample
 */
int64_t src_ds3_voice_add_final_sample(int64_t sum, int32_t data[],
                                       const int32_t coefs[], int32_t sample);

/** This function performs the initial iteration of the upsampling process
 *
 *  \param      data     Data delay line
 *  \param      coefs    FIR filter coefficients
 *  \param      sample   The newest sample
 *  \returns    A decimated sample
 */
int32_t src_us3_voice_input_sample(int32_t data[], const int32_t coefs[],
                                   int32_t sample);

/** This function performs the final two iterations of the upsampling process
 *
 *  \param      data     Data delay line
 *  \param      coefs    FIR filter coefficients
 *  \returns    A decimated sample
 */
int32_t src_us3_voice_get_next_sample(int32_t data[], const int32_t coefs[]);

#endif // _SRC_H_
