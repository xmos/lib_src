#ifndef _ASRC_TASK_H_
#define _ASRC_TASK_H_

#include <stdint.h>

#ifdef ASRC_TASK_CONFIG_PATH
    #include "asrc_task_config.h"

    // Used for buffer sizing and FIFO sizing (statically defined)
    #ifndef     MAX_ASRC_CHANNELS_TOTAL
    #error      Please set MAX_ASRC_CHANNELS_TOTAL in asrc_task_config.h
    #endif

    // Sets upper limit of worker threads for ASRC task
    #ifndef     MAX_ASRC_THREADS
    #error      Please set MAX_ASRC_THREADS in asrc_task_config.h
    #endif

    // Sets maximum number of SRC per thread. Allocates all ASRC storage so minimise to save memory
    #ifndef     SRC_MAX_SRC_CHANNELS_PER_INSTANCE
    #error      Please set SRC_MAX_SRC_CHANNELS_PER_INSTANCE in asrc_task_config.h
    #endif

    // Number of samples per channel in each block passed into SRC each call
    // Must be a power of 2 and minimum value is 4 (due to two /2 decimation stages)
    // Lower improves latency and memory usage but costs MIPS
    #ifndef     SRC_N_IN_SAMPLES
    #error      Please set SRC_N_IN_SAMPLES in asrc_task_config.h
    #endif

    // Max ratio between samples out:in per processing step (44.1->192 is worst case)
    #ifndef     SRC_N_OUT_IN_RATIO_MAX
    #error      Please set SRC_N_OUT_IN_RATIO_MAX in asrc_task_config.h
    #endif

    // Enables or disables quantisation of output with dithering to 24b
    #ifndef     SRC_DITHER_SETTING
    #error      Please set SRC_DITHER_SETTING in asrc_task_config.h
    #endif

#else
    #warning ASRC_TASK using defaults. Please set in asrc_task_config.h and globally define ASRC_TASK_CONFIG="path_to_asrc_task_config.h"
    #define MAX_ASRC_CHANNELS_TOTAL             1
    #define MAX_ASRC_THREADS                    1
    #define SRC_MAX_SRC_CHANNELS_PER_INSTANCE   1
    #define SRC_N_IN_SAMPLES                    4
    #define SRC_N_OUT_IN_RATIO_MAX              5
    #define SRC_DITHER_SETTING                  0
#endif

#include "src.h"

#define     SRC_MAX_NUM_SAMPS_OUT               (SRC_N_OUT_IN_RATIO_MAX * SRC_N_IN_SAMPLES)

// These must be defined for lib_src
#define     ASRC_N_IN_SAMPLES                   (SRC_N_IN_SAMPLES)                  // Used by SRC_STACK_LENGTH_MULT in src_mrhf_asrc.h
#define     ASRC_N_CHANNELS                     (SRC_MAX_SRC_CHANNELS_PER_INSTANCE) // Used by SRC_STACK_LENGTH_MULT in src_mrhf_asrc.h

// Structure used for 
typedef struct asrc_in_out_t{
    int32_t input_samples[2][ASRC_N_IN_SAMPLES * MAX_ASRC_CHANNELS_TOTAL];  // Double buffer input array
    unsigned input_write_idx;                                               // Double buffer idx
    int ready_flag_to_receive;                                              // Flag to indicate ASRC ready to accept samples
    int ready_flag_configured;                                              // Flag to indicate ASRC is configured and OK to pull from FIFO
    int32_t input_timestamp;                                                // Timestamp of last received input sample
    unsigned input_frequency;                                               // Nominal input sample rate  44100..192000
    unsigned input_channel_count;                                           // This is set by the producer
    int32_t output_samples[SRC_MAX_NUM_SAMPS_OUT * MAX_ASRC_CHANNELS_TOTAL];// Output sample array
    uint32_t num_output_samples;                                            // How many sample periods worth of channels
    int32_t output_time_stamp;                                              // The consumption timestamp (set by consumer)
}asrc_in_out_t;

#ifdef __XC__
void asrc_processor(chanend c_asrc_input);
int pull_samples(int32_t * unsafe samples, uint32_t output_frequency, int32_t consume_timestamp);
unsigned receive_asrc_input_samples(chanend c_asrc_input_samples, asrc_in_out_t &asrc_io, unsigned &new_input_rate);

extern "C" {
    // unsigned receive_asrc_input_samples(chanend_t c_asrc_input_samples, asrc_in_out_t *asrc_io, unsigned *asrc_channel_count, unsigned *new_input_rate);

}
#else
#include <xcore/chanend.h>
void asrc_processor(chanend_t c_asrc_input);
int pull_samples(int32_t *samples, uint32_t output_frequency, int32_t consume_timestamp);
unsigned receive_asrc_input_samples(chanend_t c_asrc_input_samples, asrc_in_out_t *asrc_io, unsigned *new_input_rate);
#endif
void reset_asrc_fifo(void);

#endif // _ASRC_TASK_H_