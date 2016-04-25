// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef __src_h__
#define __src_h__
#include <ssrc.h>
#include <asrc.h>

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
    ON,
    OFF
} dither_flag_t;

   /** Initialises synchronous sample rate conversion instance.
   *  \param   sr_in                    Nominal sample rate of input stream in Hz
   *  \param   sr_out                   Nominal sample rate of output stream in Hz
   *  \param   ssrc_ctrl                Reference to array of SSRC control stuctures
   *  \param   n_channels_per_instance  Number of channels handled by this instance of SSRC
   *  \param   n_in_samples             Number of input samples per SSRC call
   *  \param   dither_on_off            Dither to 24b on/off
   */
void ssrc_init(const fs_code_t sr_in, const fs_code_t sr_out, ssrc_ctrl_t ssrc_ctrl[], const unsigned n_channels_per_instance,
        const unsigned n_in_samples, const dither_flag_t dither_on_off);

   /** Perform synchronous sample rate conversion processing on block of input samples using previously initialized settings.
   * 
   *  \param   in_buff          Reference to input sample buffer array
   *  \param   out_buff         Reference to output sample buffer array
   *  \param   ssrc_ctrl        Reference to array of SSRC control stuctures
   *  \returns The number of output samples produced by the SRC operation
   */
unsigned ssrc_process(int in_buff[], int out_buff[], ssrc_ctrl_t ssrc_ctrl[]);

   /** Initialises asynchronous sample rate conversion instance.
   * 
   *  \param   sr_in           Nominal sample rate of input stream in Hz
   *  \param   sr_out          Nominal sample rate of output stream in Hz
   *  \param   asrc_ctrl       Reference to array of ASRC control structures
   *  \param   n_channels_per_instance  Number of channels handled by this instance of SSRC
   *  \param   n_in_samples             Number of input samples per SSRC call
   *  \param   dither_on_off            Dither to 24b on/off
   *  \returns The nominal sample rate ratio of in to out in Q4.28 format
   */
unsigned asrc_init(const fs_code_t sr_in, const fs_code_t sr_out, asrc_ctrl_t asrc_ctrl[], const unsigned n_channels_per_instance,
        const unsigned n_in_samples, const dither_flag_t dither_on_off);

   /** Perform asynchronous sample rate conversion processing on block of input samples using previously initialized settings.
   * 
   *  \param   in_buff          Reference to input sample buffer array
   *  \param   out_buff         Reference to output sample buffer array
   *  \param   fs_ratio         Fixed point ratio of in/out sample rates in Q4.28 format
   *  \param   asrc_ctrl        Reference to array of ASRC control structures
   *  \returns The number of output samples produced by the SRC operation.
   */
unsigned asrc_process(int in_buff[], int out_buff[], unsigned fs_ratio, asrc_ctrl_t asrc_ctrl[]);

#if defined(__cplusplus) || defined(__XC__)
}
#endif
#endif // __src_h__
