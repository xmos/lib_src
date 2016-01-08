// Copyright (c) 2015, XMOS Ltd, All rights reserved
#ifndef __src_h__
#define __src_h__
#include <src_config.h>

/** Initialises synchronous sample rate conversion instance.
   *  \param   sr_in           Nominal sample rate of input stream in Hz
   *  \param   sr_out          Nominal sample rate of output stream in Hz
   *  \param   processing_core Index of logical core [0..n-1]
   */
void ssrc_init(unsigned sr_in, unsigned sr_out, unsigned processing_core);

 /** Perform synchronous sample rate conversion processing on block of input samples.
   *
   *  \param   in_buff          Reference to input sample buffer array
   *  \param   out_buff         Reference to output sample buffer array
   *  \param   processing_core  Index of logical core [0..n-1]
   *  
   *  \returns The number of output samples produced by the SRC operation
   */
unsigned ssrc_process(int in_buff[], int out_buff[], unsigned processing_core);

/** Initialises asynchronous sample rate conversion instance.
   *  \param   sr_in           Nominal sample rate of input stream in Hz
   *  \param   sr_out          Nominal sample rate of output stream in Hz
   *  \param   processing_core Index of logical core [0..n-1]
   */
unsigned asrc_init(unsigned sr_in, unsigned sr_out, unsigned processing_core);

 /** Perform asynchronous sample rate conversion processing on block of input samples.
   *
   *  \param   in_buff          Reference to input sample buffer array
   *  \param   out_buff         Reference to output sample buffer array
   *  \param   FsRatio          Fixed point ratio of in/out sample rates in Q4.28 format 
   *  \param   processing_core  Index of logical core [0..n-1]
   *  
   *  \returns The number of output samples produced by the SRC operation.
   */
unsigned asrc_process(int in_buff[], int out_buff[], unsigned FsRatio, unsigned processing_core);

#endif // __src_h__
