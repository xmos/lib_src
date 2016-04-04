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
    FS_CODE_44100 = 0,
    FS_CODE_48000,
    FS_CODE_88200,
    FS_CODE_96000,
    FS_CODE_176400,
    FS_CODE_192000
} fs_code_t;

   /** Initialises synchronous sample rate conversion instance.
   *  \param   sr_in                    Nominal sample rate of input stream in Hz
   *  \param   sr_out                   Nominal sample rate of output stream in Hz
   *  \param   sSSRCCtrl                Reference to array of SSRC control stuctures
   *  \param   n_channels_per_instance  Number of channels handled by this instance of SSRC
   *  \param   n_in_samples             Number of input samples per SSRC call
   *  \param   dither_on_off            Dither to 24b on/off
   */
void ssrc_init(fs_code_t sr_in, fs_code_t sr_out, SSRCCtrl_t sSSRCCtrl[], const unsigned n_channels_per_instance,
        const unsigned n_in_samples, const unsigned dither_on_off);

   /** Perform synchronous sample rate conversion processing on block of input samples using previously initialized settings.
   * 
   *  \param   in_buff          Reference to input sample buffer array
   *  \param   out_buff         Reference to output sample buffer array
   *  \param   sSSRCCtrl        Reference to array of SSRC control stuctures
   *  \returns The number of output samples produced by the SRC operation
   */
unsigned ssrc_process(int in_buff[], int out_buff[], SSRCCtrl_t sSSRCCtrl[]);

   /** Initialises asynchronous sample rate conversion instance.
   * 
   *  \param   sr_in           Nominal sample rate of input stream in Hz
   *  \param   sr_out          Nominal sample rate of output stream in Hz
   *  \param   sASRCCtrl       Reference to array of ASRC control structures
   *  \param   n_channels_per_instance  Number of channels handled by this instance of SSRC
   *  \param   n_in_samples             Number of input samples per SSRC call
   *  \param   dither_on_off            Dither to 24b on/off
   *  \returns The nominal sample rate ratio of in to out in Q4.28 format
   */
unsigned asrc_init(fs_code_t sr_in, fs_code_t sr_out, ASRCCtrl_t sASRCCtrl[], const unsigned n_channels_per_instance,
        const unsigned n_in_samples, const unsigned dither_on_off);

   /** Perform asynchronous sample rate conversion processing on block of input samples using previously initialized settings.
   * 
   *  \param   in_buff          Reference to input sample buffer array
   *  \param   out_buff         Reference to output sample buffer array
   *  \param   FsRatio          Fixed point ratio of in/out sample rates in Q4.28 format 
   *  \param   sASRCCtrl        Reference to array of ASRC control structures
   *  \returns The number of output samples produced by the SRC operation.
   */
unsigned asrc_process(int in_buff[], int out_buff[], unsigned FsRatio, ASRCCtrl_t sASRCCtrl[]);

#if defined(__cplusplus) || defined(__XC__)
}
#endif
#endif // __src_h__
