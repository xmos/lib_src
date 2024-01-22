#ifndef _asynchronous_fifo_h__
#define _asynchronous_fifo_h__

#include <stdint.h>
#include <xccompat.h>

#define FREQUENCY_RATIO_EXPONENT     (32)

#ifdef __XC__
#define UNSAFE unsafe
#else
#define UNSAFE
#endif

/**
 * Data structure that holds the status of an asynchronous FIFO
 */
typedef struct asynchronous_fifo_t asynchronous_fifo_t;

/**
 * Function that must be called to initialise the asynchronous FIFO. You must pass in an array
 * of ``int64_t [ASYNCHRONOUS_FIFO_INT64_ELEMENTS(fifo_max_depth, channel_count)]``
 * The same pointer must be used on both sides of the asynchronous FIFO.
 *
 * After initialising, you must initialise the PID by calling one of
 * ``asynchronous_fifo_init_PID_fs_codes()`` or
 * ``asynchronous_fifo_init_PID_raw()``
 *
 * @param   state               Asynchronous FIFO to be initialised
 *
 * @param   channel_count       Number of audio channels
 *
 * @param   max_fifo_depth      Length of the FIFO, delay when stable will be max_fifo_depth/2
 */
void asynchronous_fifo_init(asynchronous_fifo_t * UNSAFE state,
                            int channel_count,
                            int max_fifo_depth);

/**
 * Function that that initialises the PID of a FIFO. Either this function
 * or ``asynchronous_fifo_init_PID_raw()`` should be called. This function
 * uses frequency codes as defined in the ASRC for a quick default setup,
 * the raw function allows full control
 *
 * @param   state               Asynchronous FIFO to be initialised
 *
 * @param   fs_input            Input FS ratio, used to pick appropriate Kp, Ki
 *
 * @param   fs_output           Input FS ratio, used to pick appropriate Kp, Ki, ideal phase
 */
void asynchronous_fifo_init_PID_fs_codes(asynchronous_fifo_t *state,
                                         int fs_input, int fs_output);

/**
 * Function that that initialises the PID of a FIFO. Either this function
 * or ``asynchronous_fifo_init_PID_raw()`` should be called. This function
 * uses frequency codes as defined in the ASRC for a quick default setup,
 * the raw function allows full control.
 *
 * This function may be called at any time by the producer in order to alter the PID
 * and midpoint settings. It does not reset the error;  one of the
 * ``asynchronous_fifo_init_reset()`` functions should be called for that.
 *
 * @param   state               Asynchronous FIFO to be initialised
 *
 * @param   Kp                  Proportional constant for the FIFO.
 *                              This gets multiplied by the differential
 *                              error measured in ticks (typically -2..2)
 *                              and added to the ratio_error. A typical
 *                              value is 30,000,000 - 60,000,000.
 *
 * @param   Ki                  Integral constant for the FIFO.
 *                              This gets multiplied by the phase error
 *                              measured in ticks (typically -20,000 - 20,000)
 *                              and added to the ratio_error.
 *                              A typical value is 200 - 300.
 *
 * @param  ticks_between_samples The number of ticks between samples is used to
 *                              estimate the expected phase error halfway down
 *                              the FIFO.
 */
void asynchronous_fifo_init_PID_raw(asynchronous_fifo_t *state,
                                    int Kp, int Ki, int ticks_between_samples);

/**
 * Function that that resets the FIFO from the producer side. Either this function should
 * be called on the producing side, or ``asynchronous_fifo_reset_consumer()``
 * should be called on the consumer side. In both cases the whole FIFO will be reset back
 *
 * @param   state               Asynchronous FIFO to be initialised
 */
void asynchronous_fifo_reset_producer(asynchronous_fifo_t *state);

/**
 * Function that that resets the FIFO from the consumer side. Either this function should
 * be called on the consuming side, or ``asynchronous_fifo_reset_producer()``
 * should be called on the producer side. In both cases the whole FIFO will be reset back
 *
 * @param   state               Asynchronous FIFO to be initialised
 */
void asynchronous_fifo_reset_consumer(asynchronous_fifo_t *state);

/**
 * Function that must be called to deinitalise the asynchronous FIFO
 *
 * @param   state                   ASRC structure to be de-initialised
 */
void asynchronous_fifo_exit(asynchronous_fifo_t * UNSAFE state);

/**
 * Function that provides the next sample to the asynchronous FIFO.
 *
 * This function and the consume function both need a timestamp,
 * which is the time that the last sample was input (this function) or
 * output (``asynchronous_fifo_consume``). The asynchronous FIFO will hand the
 * samples across from producer to consumer through an elastic queue, and run
 * a PID algorithm to calculate the best way to equalise the input clock relative
 * to the output clock. Therefore, the timestamps
 * have to be measured on either the same clock or two very similar clocks.
 * It is probably fine to use the reference clocks on two tiles, provided
 * the tiles came out of reset at more or less the same time. Using the
 * clocks from two different chips would require the two chips to share an
 * oscillator, and for them to come out of reset simultaneously.
 *
 * @param   state               ASRC structure to push the sample into
 *
 * @param   samples             The sample values.
 *
 * @param   n                   The number of samples
 *
 * @param   timestamp           The number of ticks when this sample was input.
 *
 * @returns The current estimate of the mismatch of input and output frequencies.
 *          This is represented as a 32-bit signed number. Zero means no mismatch,
 *          a value less than zero means that the producer is faster than the consumer,
 *          a value greater than zero means that the producer is slower than the consumer.
 *          The value should be scaled by 2**-32. That is, the current best
 *          approximation for consumer_speed/producer_speed is 1 + (return_value * 2**-32)
 *
 *          The output is filtered and should be applied directly as a correction factor
 *          eg, multiplied into an ASRC ratio, or multiplied into a PLL timing.
 */
int32_t asynchronous_fifo_produce(asynchronous_fifo_t * UNSAFE state,
                                  int32_t * UNSAFE samples,
                                  int n,
                                  int32_t timestamp,
                                  int xscope_used);


/**
 * Function that extracts an output sample from the asynchronous FIFO
 *
 * @param   state               ASRC structure to pull a sample out off.
 *
 * @param   samples             The array where the frame with output
 *                              samples will be stored.
 *
 * @param   timestamp           A timestamp taken at the time that the
 *                              last sample was output. See
 *                              ``asynchronous_fifo_produce`` for requirements.
 */
void asynchronous_fifo_consume(asynchronous_fifo_t * UNSAFE state,
                               int32_t * UNSAFE samples,
                               int32_t timestamp);


/**
 * Data structure that holds the state
 * Internal use only. Declared here so that one can create one of these
 * on the stack
 */
struct asynchronous_fifo_t {
    // Updated on initialisation only
    int32_t   channel_count;                  /* Number of audio channels */
    int32_t   copy_mask;                      /* Number of audio channels */
    int32_t   max_fifo_depth;                 /* Length of buffer[] in channel_counts */
    int32_t   ideal_phase_error_ticks;        /* Ideal ticks between samples */
    int32_t   Ki;                             /* Ki PID coefficient */
    int32_t   Kp;                             /* Kp PID coefficient */

    // Updated on the producer side only
    int       skip_ctr;                       /* Set to indicate initialisation runs */
    int32_t   write_ptr;                      /* Write index in the buffer */
    int64_t   last_phase_error;               /* previous error, used for proportional */
    int64_t   frequency_ratio;                /* Current ratio of frequencies in 64.64 */
    int32_t   stop_producing;                 /* In case of overflow, stops producer until consumer restarts and requests a reset */

    // Updated on the consumer side only
    uint32_t  read_ptr;                       /* Read index in the buffer */
    uint32_t  sample_timestamp;               /* Timestamp calculated by consumer */

    // Set by producer, reset by consumer
    uint32_t  reset;                          /* Set to 1 if consumer wants a reset */

    // Updated from both sides
    uint32_t  * UNSAFE timestamps;            /* Timestamps of samples */
    int32_t   buffer[0];                      /* Buffer of data */
};

/**
 * macro that calculates the number of int64_t to be allocated for the fifo
 * for a FIFO of N elements and C channels
 */
#define ASYNCHRONOUS_FIFO_INT64_ELEMENTS(N, C) (sizeof(asynchronous_fifo_t)/sizeof(int64_t) + (N*(C+1))/2+1)
#endif
