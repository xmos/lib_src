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

typedef struct src_task_t
{
    uint64_t fsRatio;
    int32_t idealFsRatio;
    int32_t xscopeUsed;
    asynchronous_fifo_t * unsafe async_fifo;
} src_task_t;

fs_code_t sr_to_fscode(unsigned sr);

void src_task_write(streaming chanend c_src[SRC_N_INSTANCES],
                                int srcInputBuff[SRC_N_INSTANCES][SRC_N_IN_SAMPLES][SRC_CHANNELS_PER_INSTANCE],
                                int32_t now,
                                src_task_t * unsafe srcState);

#define src_task_read(a, b, c) unsafe{asynchronous_fifo_consumer_get(a->async_fifo, b, c);}

#ifdef __XC__
void src_task(streaming chanend c[numInstances], unsigned  numInstances, int i, int o);
void src_task_set_sr(src_task_t * unsafe srcState, int inputSr, int outputSr, streaming chanend c[numInstances], unsigned numInstances);
void src_task_init(src_task_t * unsafe srcState, int64_t array[], int numChans, int fifoLength, int xscopeUsed);
#else
void src_task(streaming chanend c[], unsigned numInstances, int i, int o);
void src_task_set_sr(src_task_t * srcState, int inputSr, int outputSr, streaming chanend c[], unsigned numInstances);
void src_task_init(src_task_t * unsafe srcState, int64_t array[], int numChans, int fifoLength, int xscopeUsed);
#endif

#endif
