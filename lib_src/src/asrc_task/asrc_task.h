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

// Compatibility for XC and C
#ifdef __XC__
#define UNSAFE unsafe
#else
#include <xcore/chanend.h>
#include <xccompat.h>
#define UNSAFE
#endif // __XC__

#else
#define UNSAFE
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

#ifndef __XC__
/**
 * Type definition of the callback function if a user version is required. May only be used from "C" (XC does not support function pointers).
 */
typedef unsigned (*asrc_task_produce_isr_cb_t)(chanend_t c_asrc_input, asrc_in_out_t *asrc_io, unsigned *new_input_rate);
#endif


/**
 * Main ASRC processor task. Runs forever waiting on new samples from the producer. Spawns up to MAX_ASRC_THREADS during ASRC processing.
 *
 * \param c_asrc_input      The channel end used to connect the producer to the ASRC task.
 * \param asrc_io           A pointer to the structure used for holding ASRC IO and state.
 * \param fifo              A pointer to the FIFO used for outputting samples from the ASRC task to the consumer.
 * \param fifo_length       The length (depth) of the output FIFO. This is multiplied by channel count internally.
 *
 */
void asrc_task(chanend c_asrc_input, asrc_in_out_t * UNSAFE asrc_io, asynchronous_fifo_t * UNSAFE fifo, unsigned fifo_length);

/**
 * Helper function called by consumer to provide ASRC output samples. Samples are populated in the *samples array and the user
 * must provide the current nominal output frequency and a timestamp of when the last samples were consumed from the 100 MHz ref clock
 *
 * \param asrc_io           A pointer to the structure used for holding ASRC IO and state.
 * \param fifo              A pointer to the FIFO used for outputting samples from the ASRC task to the consumer.
 * \param samples           A pointer to a whole output frame (all channels in a single sample period) to populate.
 * \param output_frequency  The nominal output frequency. Used for detecting a sample rate change.
 * \param consume_timestamp The timestamp of the first consumed sample.
 *
 */
int pull_samples(asrc_in_out_t * UNSAFE asrc_io, asynchronous_fifo_t * UNSAFE fifo, int32_t * UNSAFE samples, uint32_t output_frequency, int32_t consume_timestamp);

/**
 * Helper function called by consumer to reset the FIFO. Resets to half full and clears the contents to zero.
 *
 * \param fifo              A pointer to the FIFO used for outputting samples from the ASRC task to the consumer.
 *
 */
void reset_asrc_fifo_consumer(asynchronous_fifo_t * UNSAFE fifo);

#ifndef __XC__
/**
 * Prototype that can optionally be defined by the user to initialise the function pointer for the ASRC receive produced samples ISR.
 * If this is not called then receive_asrc_input_samples_cb_default() is used and the you may call send_asrc_input_samples_default()
 * from the application to send samples to the ASRC task.
 * 
 * Must be called before running asrc_task()
 * 
 * \param asrc_io           A pointer to the structure used for holding ASRC IO and state.
 * \param asrc_rx_fp        A pointer to the user asrc_receive_samples function. NOTE - This MUST be decorated by ASRC_TASK_ISR_CALLBACK_ATTR
 *                          to allow proper stack calculation by the compiler. See receive_asrc_input_samples_cb_default() in asrc_task.c
 *                          for an example of how to do this.
 *
 */
void init_asrc_io_callback(asrc_in_out_t * UNSAFE asrc_io, asrc_task_produce_isr_cb_t asrc_rx_fp);
#endif

/**
 * If the init_asrc_io_callback() function is not called then a default implementation of the ASRC receive will be used.
 * This send function (called by the user producer side) mirrors the receive and can be used to push samples into the ASRC.
 * 
 * \param c_asrc_input          The chan end on the application producer side connecting to the ASRC task.
 * \param input_frequency       The sample rate of the input stream (44100, 48000, ...).
 * \param input_timestamp       The ref clock timestamp of latest received input sample.
 * \param input_channel_count   The number of input audio channels (1, 2, 3 ...).
 * \param input_samples         A pointer to the input samples array (channel 0, 1, ...).
 *
 */
void send_asrc_input_samples_default(chanend c_asrc_input, unsigned input_frequency, int32_t input_timestamp, unsigned input_channel_count, int32_t * UNSAFE input_samples);

/**@}*/ // END: addtogroup src_asrc_task

#endif // _ASRC_TASK_H_
