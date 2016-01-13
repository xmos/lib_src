// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef __src_h__
#define __src_h__
#include <src_config.h>
#include <ssrc.h>
#include <asrc.h>

   /** Initialises synchronous sample rate conversion instance.
   *  \param   sr_in           Nominal sample rate of input stream in Hz
   *  \param   sr_out          Nominal sample rate of output stream in Hz
   *  \param   sSSRCCtrl       Pointer to SSRC control stucture
   */
#ifdef __XC__
void ssrc_init(unsigned sr_in, unsigned sr_out, SSRCCtrl_t * unsafe sSSRCCtrl);
#else
void ssrc_init(unsigned sr_in, unsigned sr_out, SSRCCtrl_t *sSSRCCtrl);
#endif
   /** Perform synchronous sample rate conversion processing on block of input samples.
   * 
   *  \param   in_buff          Reference to input sample buffer array
   *  \param   out_buff         Reference to output sample buffer array
   *  \param   sSSRCCtrl        Pointer to SSRC control stucture
   *  \returns The number of output samples produced by the SRC operation
   */
#ifdef __XC__
unsigned ssrc_process(int in_buff[], int out_buff[], SSRCCtrl_t * unsafe sSSRCCtrl);
#else
unsigned ssrc_process(int in_buff[], int out_buff[], SSRCCtrl_t *sSSRCCtrl);
#endif

   /** Initialises asynchronous sample rate conversion instance.
   * 
   *  \param   sr_in           Nominal sample rate of input stream in Hz
   *  \param   sr_out          Nominal sample rate of output stream in Hz
   *  \param   sASRCCtrl       Reference to array of ASRC control structures
   *  \returns The nominal sample rate ratio of in to out in Q4.28 format
   */
unsigned asrc_init(unsigned sr_in, unsigned sr_out, ASRCCtrl_t sASRCCtrl[ASRC_CHANNELS_PER_CORE]);

   /** Perform asynchronous sample rate conversion processing on block of input samples.
   * 
   *  \param   in_buff          Reference to input sample buffer array
   *  \param   out_buff         Reference to output sample buffer array
   *  \param   FsRatio          Fixed point ratio of in/out sample rates in Q4.28 format 
   *  \param   sASRCCtrl        Reference to array of ASRC control structures
   *  \returns The number of output samples produced by the SRC operation.
   */
unsigned asrc_process(int in_buff[], int out_buff[], unsigned FsRatio, ASRCCtrl_t sASRCCtrl[ASRC_CHANNELS_PER_CORE]);

#endif // __src_h__
