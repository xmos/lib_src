#ifndef _SRC_TASK_
#define _SRC_TASK_

#include <xs1.h>
#include <xccompat.h>
#include "src.h"
#include "asynchronous_fifo.h"
#include "asrc_timestamp_interpolation.h"
#include "xassert.h"

#ifdef __XC__
#define streaming streaming
#define unsafe unsafe
#else
#define streaming
#define unsafe
#endif

fs_code_t sr_to_fscode(unsigned sr);

uint64_t trigger_src(streaming chanend c_src[SRC_N_INSTANCES],
                                int srcInputBuff[SRC_N_INSTANCES][SRC_N_IN_SAMPLES][SRC_CHANNELS_PER_INSTANCE],
                                uint64_t fsRatio,
                                asynchronous_fifo_t * unsafe a,
                                int32_t now, int xscope_used, int idealFsRatio);

#ifdef __XC__
void src_task(streaming chanend c[numInstances], int numInstances, int inputSr, int outputSr);
#else
void src_task(streaming chanend c[], int numInstances, int inputSr, int outputSr);
#endif

#endif
