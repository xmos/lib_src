// Copyright (c) 2015, XMOS Ltd, All rights reserved
#ifndef __src_h__
#define __src_h__
#include <src_config.h>

/** Initialises SRC instance
   *  \param   sr_in           Nominal sample rate of input stream in Hz
   *  \param   sr_out          Nominal sample rate of output stream in Hz
   */
void ssrc_init(unsigned sr_in, unsigned sr_out, unsigned processing_core);

 /** Perform sample rate conversion processing on block of input samples.
   *
   *  \param   in_buff          Reference to input sample buffer array
   *  \param   out_buff         Reference to output sample buffer array
   *  \param   processing_core  Index of logical core 
   *  
   *  \returns The number of output samples produced by SRC operation
   */
unsigned ssrc_process(int in_buff[], int out_buff[], unsigned processing_core);


unsigned asrc_init(unsigned sr_in, unsigned sr_out, unsigned processing_core);
unsigned asrc_process(int in_buff[], int out_buff[], unsigned FsRatio, unsigned processing_core);

#endif // __src_h__
