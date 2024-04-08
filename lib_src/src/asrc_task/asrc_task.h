// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _ASRC_TASK_H_
#define _ASRC_TASK_H_

#include <stdint.h>

/**
 * \addtogroup src_asrc_task
 *
 * The public API for using ASRC Task
 * @{
 */


#if (!defined(ASRC_TASK_CONFIG)) || defined(__DOXYGEN__)
    #warning ASRC_TASK using defaults. Please set in asrc_task_config.h and globally define ASRC_TASK_CONFIG=1
    /** @brief Maximum number of audio channels in total. Used for buffer sizing and FIFO sizing (statically defined). */
    #define MAX_ASRC_CHANNELS_TOTAL             1
    /** @brief Maximum number of threads to be spawned by ASRC task. Used for buffer sizing and FIFO sizing (statically defined).*/
    #define MAX_ASRC_THREADS                    1
    /** @brief Block size of input to the low level asrc_process function. Must be a power of 2 and minimum value is 4, maximum is 16. Used for buffer sizing and FIFO sizing (statically defined). */
    #define SRC_N_IN_SAMPLES                    4
    /** @brief  Max ratio between samples out:in per processing step (44.1->192 is worst case). Used for buffer sizing and FIFO sizing (statically defined). */
    #define SRC_N_OUT_IN_RATIO_MAX              5
    /** @brief Enables or disables quantisation of output with dithering to 24b. */
    #define SRC_DITHER_SETTING                  0

#else
    // Check for required static defines    
    #include "asrc_task_config.h"

    #ifndef     MAX_ASRC_CHANNELS_TOTAL
    #error      Please set MAX_ASRC_CHANNELS_TOTAL in asrc_task_config.h
    #endif

    #ifndef     MAX_ASRC_THREADS
    #error      Please set MAX_ASRC_THREADS in asrc_task_config.h
    #endif

    #ifndef     SRC_N_IN_SAMPLES
    #error      Please set SRC_N_IN_SAMPLES in asrc_task_config.h
    #endif

    #ifndef     SRC_N_OUT_IN_RATIO_MAX
    #error      Please set SRC_N_OUT_IN_RATIO_MAX in asrc_task_config.h
    #endif

    #ifndef     SRC_DITHER_SETTING
    #error      Please set SRC_DITHER_SETTING in asrc_task_config.h
    #endif

#endif

/** @brief Decorator for user ASRC producer receive callback. Must be used to allow stack usage calculation. */
#define  ASRC_TASK_ISR_CALLBACK_ATTR            __attribute__((fptrgroup("asrc_callback_isr_fptr_grp"))) 

#ifndef __DOXYGEN__

// Internally calculated defines
#define  SRC_MAX_SRC_CHANNELS_PER_INSTANCE      (((MAX_ASRC_CHANNELS_TOTAL + (MAX_ASRC_THREADS - 1)) / MAX_ASRC_THREADS)) // Round up divide

#include "src.h"
#include "asynchronous_fifo.h"

#define     SRC_MAX_NUM_SAMPS_OUT               (SRC_N_OUT_IN_RATIO_MAX * SRC_N_IN_SAMPLES)

// These must be defined for lib_src
#define     ASRC_N_IN_SAMPLES                   (SRC_N_IN_SAMPLES)                  // Used by SRC_STACK_LENGTH_MULT in src_mrhf_asrc.h
#define     ASRC_N_CHANNELS                     (SRC_MAX_SRC_CHANNELS_PER_INSTANCE) // Used by SRC_STACK_LENGTH_MULT in src_mrhf_asrc.h


#ifdef __XC__
#define UNSAFE unsafe
#else
#include <xcore/chanend.h>
#define UNSAFE
#endif

#endif // ifndef __DOXYGEN__

/**
 * Structure used for holding the IO context and state of the ASRC_TASK. Should be initialised to {{{0}}}.
 */
typedef struct asrc_in_out_t_{
    /** Double buffer input array */
    int32_t input_samples[2][ASRC_N_IN_SAMPLES * MAX_ASRC_CHANNELS_TOTAL];
    /** Double buffer idx */
    unsigned input_write_idx;
    /** Timestamp of last received input sample */                                        
    int32_t input_timestamp;
    /** Nominal input sample rate  44100..192000 (set by producer) */  
    unsigned input_frequency;
    /** This is set by the producer and can change dynamically */                                           
    unsigned input_channel_count;
    /** The function pointer of the ASRC_TASK producer receive callback. Must be defined by user to receive samples from producer over channel. */
    void * UNSAFE asrc_task_produce_cb;

    /** Output sample array */
    int32_t output_samples[SRC_MAX_NUM_SAMPS_OUT * MAX_ASRC_CHANNELS_TOTAL];
    /** Output sample rate (set by consumer) */
    unsigned output_frequency;
    /** The consumption timestamp (set by consumer) */
    int32_t output_time_stamp;

    //* The values below are internal state and are not intended to be accessed by the user */
    /** Currently configured channel count. Used by process and consumer */
    unsigned asrc_channel_count;
    /** Flag to indicate ASRC ready to accept samples */
    int ready_flag_to_receive;
    /** Flag to indicate ASRC is configured and OK to pull from FIFO */                                               
    int ready_flag_configured;

}asrc_in_out_t;


#ifdef __XC__
/**
 * Main ASRC processor task. Runs forever waiting on new samples from the producer. Spawns up to MAX_ASRC_THREADS during ASRC processing.
 *
 * \param c_asrc_input      The channel end used to connect the producer to the ASRC task.
 * \param asrc_io           A pointer to the structure used for holding ASRC IO and state.
 * \param fifo              A pointer to the FIFO used for outputting samples from the ASRC task to the consumer.
 * \param fifo_length       The length (depth) of the output FIFO. This is multiplied by channel count internally.
 *
 */
void asrc_task(chanend c_asrc_input, asrc_in_out_t * unsafe asrc_io, asynchronous_fifo_t * unsafe fifo, unsigned fifo_length);

/**
 * Helper function called by consumer to provide ASRC output samples. Samples are populated in the *samples array and the user
 * must provide the current nominal output frequency and a timestamp of when the last samples were consumed from the 100 MHz ref clock
 *
 * \param asrc_io           A pointer to the structure used for holding ASRC IO and state.
 * \param fifo              A pointer to the FIFO used for outputting samples from the ASRC task to the consumer.
 * \param samples           A pointer to a whole output frame (all channels in a single sample period) to populate.
 * \param output_frequency  The nominal output frequency. Used for detecting a sample rate change.
 * \param consume_timestamp The timestamp of the most recently consumed sample.
 *
 */
int pull_samples(asrc_in_out_t * unsafe asrc_io, asynchronous_fifo_t * unsafe fifo, int32_t * unsafe samples, uint32_t output_frequency, int32_t consume_timestamp);

/**
 * Helper function called by consumer to reset the FIFO. Resets to half full and clears the contents to zero.
 *
 * \param fifo              A pointer to the FIFO used for outputting samples from the ASRC task to the consumer.
 *
 */
void reset_asrc_fifo_consumer(asynchronous_fifo_t * unsafe fifo);

/**
 * Prototype that must be defined by the user to initialise the function pointer for the ASRC receive produced samples ISR.
 * Typical user function (where receive_asrc_input_samples() is the user defined rx function):
 * void init_asrc_io_callback(asrc_in_out_t *asrc_io){
 *      asrc_io->asrc_task_produce_cb = receive_asrc_input_samples;
 * }
 * 
 * Must be called before running asrc_task()
 * 
 * \param asrc_io           A pointer to the structure used for holding ASRC IO and state.
 *
 */
void init_asrc_io_callback(asrc_in_out_t * unsafe asrc_io);

#else // C
#include <xcore/chanend.h>

/**
 * Main ASRC processor task. Runs forever waiting on new samples from the producer. Spawns up to MAX_ASRC_THREADS during ASRC processing.
 *
 * \param c_asrc_input      The channel end used to connect the producer to the ASRC task.
 * \param asrc_io           A pointer to the structure used for holding ASRC IO and state.
 * \param fifo              A pointer to the FIFO used for outputting samples from the ASRC task to the consumer.
 * \param fifo_length       The length (depth) of the output FIFO. This is multiplied by channel count internally.
 *
 */
void asrc_task(chanend_t c_asrc_input, asrc_in_out_t *asrc_io, asynchronous_fifo_t * fifo, unsigned fifo_length);

/**
 * Helper function called by consumer to provide ASRC output samples. Samples are populated in the *samples array and the user
 * must provide the current nominal output frequency and a timestamp of when the last samples were consumed from the 100 MHz ref clock
 *
 * \param asrc_io           A pointer to the structure used for holding ASRC IO and state.
 * \param fifo              A pointer to the FIFO used for outputting samples from the ASRC task to the consumer.
 * \param samples           A pointer to a whole output frame (all channels in a single sample period) to populate.
 * \param output_frequency  The nominal output frequency. Used for detecting a sample rate change.
 * \param consume_timestamp The timestamp of the most recently consumed sample.
 *
 */
int pull_samples(asrc_in_out_t *asrc_io, asynchronous_fifo_t * fifo, int32_t *samples, uint32_t output_frequency, int32_t consume_timestamp);

/**
 * Helper function called by consumer to reset the FIFO. Resets to half full and clears the contents to zero.
 *
 * \param fifo              A pointer to the FIFO used for outputting samples from the ASRC task to the consumer.
 *
 */
void reset_asrc_fifo(asynchronous_fifo_t * fifo);

/**
 * Prototype that must be defined by the user to initialise the function pointer for the ASRC receive produced samples ISR.
 * Typical user function (where receive_asrc_input_samples() is the user defined rx function):
 * void init_asrc_io_callback(asrc_in_out_t *asrc_io){
 *      asrc_io->asrc_task_produce_cb = receive_asrc_input_samples;
 * }
 *
 * Must be called before running asrc_task()
 * 
 * \param asrc_io           A pointer to the structure used for holding ASRC IO and state.
 *
 */
void init_asrc_io_callback(asrc_in_out_t * asrc_io);
#endif

/**@}*/ // END: addtogroup src_asrc_task


#endif // _ASRC_TASK_H_
