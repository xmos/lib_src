#ifndef _asynchronous_fifo_h__
#define _asynchronous_fifo_h__

#include <stdint.h>

#define FREQUENCY_RATIO_EXPONENT     (32)

/**
 * Data structure that holds the status of an asynchronous FIFO
 */
typedef struct asynchronous_fifo_t asynchronous_fifo_t;

/**
 * Function that must be called to initialise the asynchronous FIFO. You must pass in an array
 * of ``int64_t [ASYNCHRONOUS_FIFO_INT64_ELEMENTS(fifo_max_depth, channel_count)]``
 * The same pointer must be used on both sides of the asynchronous FIFO.
 *
 * @param   state               Asynchronous FIFO to be initialised
 *
 * @param   channel_count       Number of audio channels
 *
 * @param   max_fifo_depth      Length of the FIFO, delay when stable will be max_fifo_depth/2
 *
 * @param   ticks_between_samples  Expected number of ticks between two subsequent samples
 *                                 Round this number to the nearest tick, eg, 2083 for 48 kHz.
 *
 * @param   invalid_time_stamp_rate  Expected number of number of calls to producer over
 *                                   which a one valid timestamp will be given. Typically
 *                                   1 if all calls will have a valid timestamp or 4 if one
 *                                   in four calls will have a valid timestamp. Used to tune
 *                                   the PID filter.
 */
void asynchronous_fifo_init(asynchronous_fifo_t *state,
                            int channel_count,
                            int max_fifo_depth,
                            int ticks_between_samples,
                            int invalid_time_stamp_rate);

/**
 * Function that must be called to deinitalise the asynchronous FIFO
 *
 * @param   state                   ASRC structure to be de-initialised
 */
void asynchronous_fifo_exit(asynchronous_fifo_t *state);

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
 * @param   timestamp           The number of ticks when this sample was input.
 *
 * @param   timestamp_valid     States whether the timestamp is valid.
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
int32_t asynchronous_fifo_produce(asynchronous_fifo_t *state,
                                  int32_t *samples,
                                  int32_t timestamp,
                                  int timestamp_valid);


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
void asynchronous_fifo_consume(asynchronous_fifo_t *state,
                               int32_t *samples,
                               int32_t timestamp);


/**
 * Data structure that holds the state
 * Internal use only. Declared here so that one can create one of these
 * on the stack
 */
struct asynchronous_fifo_t {
    // Updated on initialisation only
    int32_t   channel_count;                  /* Number of audio channels */
    int32_t   max_fifo_depth;                 /* Length of buffer[] in channel_counts */
    int32_t   ideal_phase_error_ticks;        /* Ideal ticks between samples */
    int32_t   Ki;                             /* Ki PID coefficient */
    int32_t   Kp;                             /* Kp PID coefficient */
    
    // Updated on the producer side only
    int       skip_ctr;                       /* Set to indicate initialisation runs */
    int32_t   write_ptr;                      /* Write index in the buffer */
    int32_t   converted_sample_number;        /* Sample number counter of producer */
    int64_t   last_phase_error;               /* previous error, used for proportional */
    int64_t   frequency_ratio;                /* Current ratio of frequencies in 64.64 */
    int32_t   stop_producing;                 /* In case of overflow, stops producer until consumer restarts and requests a reset */
    int32_t   diff_error_samples;             /* Number of samples over which the last timestamp was measured */

    // Updated on the consumer side only
    uint32_t  read_ptr;                       /* Read index in the buffer */
    uint32_t  output_sample_number;           /* Current consumer output sample */
    uint32_t  sample_timestamp;               /* Timestamp calculated by consumer */
    int32_t   sample_number;                  /* Sample number of the timestamp */

    // Set by producer, reset by consumer
    uint32_t  reset;                          /* Set to 1 if consumer wants a reset */

    // Updated from both sides
    uint32_t  *timestamps;                    /* Timestamps of samples */
    int32_t   buffer[0];                      /* Buffer of data */
};

/**
 * macro that calculates the number of int64_t to be allocated for the fifo
 * for a FIFO of N elements and C channels
 */
#define ASYNCHRONOUS_FIFO_INT64_ELEMENTS(N, C) (sizeof(asynchronous_fifo_t)/sizeof(int64_t) + (N*(C+1))/2+1)
#endif
