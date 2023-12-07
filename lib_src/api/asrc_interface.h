#ifndef _asrc_interface_h__
#define _asrc_interface_h__

#include <stdint.h>

/**
 * Data structure that holds the status of an ASRC interface
 */
typedef struct asrc_interface_t asrc_interface_t;

/**
 * Function that must be called to initialise the ASRC. You must pass in an array
 * of ``int [sizeof(asrc_interface_t)/sizeof(int) + channel_count * fifo_max_depth]``
 * The same pointer must be used on both sides of the ASRC interface.
 *
 * @param   asrc_state          ASRC structure to be initialised
 *
 * @param   channel_count       Number of audio channels
 *
 * @param   max_fifo_depth      Length of the FIFO, delay when stable will be max_fifo_depth/2
 */
void asrc_init_buffer(asrc_interface_t *asrc_state,
                      int channel_count,
                      int max_fifo_depth);

/**
 * Function that must be called to deinitalise the ASRC
 *
 * @param   asrc_state          ASRC structure to be de-initialised
 */
void asrc_exit_buffer(asrc_interface_t *asrc_state);

/**
 * Function that provides the next four (multi-channel) samples to the ASRC.
 * This function must be called at exactly the input frequency divided by four.
 *
 * This function and the single_output function both need a timestamp,
 * which is the time that the last sample was input (this function) or
 * output (``asrc_get_single_output``). The timestamps will be compared to
 * each other, and attempted to be equalised. Therefore, the timestamps
 * have to be measured on either the same clock or two very similar clocks.
 * It is probably fine to use the reference clocks on two tiles, provided
 * the tiles came out of reset at more or less the same time. Using the
 * clocks from two different chips would require the two chips to share an
 * oscillator, and for them to come out of reset simultaneously.
 *
 * @param   asrc_state          ASRC structure to push the samples into 
 *
 * @param   samples             The samples stored as an 2D array of four frames
 *                              each frame containing channel_count samples.
 *
 * @param   timestamp           A timestamp taken at the time that the last sample
 *                              was input.
 */
void asrc_add_quad_input(asrc_interface_t *asrc_state,
                         int32_t *samples,
                         int32_t timestamp);


/**
 * Function that extracts an output sample from the ASRC.
 * This function must be called at exactly the output frequency.
 *
 * @param   asrc_state          ASRC structure to pull a sample out off.
 *
 * @param   samples             The array where the frame with output
 *                              samples will be stored.
 *
 * @param   timestamp           A timestamp taken at the time that the
 *                              last sample was output. See
 *                              ``asrc_add_quad_input`` for requirements.
 */
void asrc_get_single_output(asrc_interface_t *asrc_state,
                            int32_t *samples,
                            int32_t timestamp);


/**
 * Data structure that holds the state
 * Internal use only. Declared here so that one can create one of these
 * on the stack
 */
struct asrc_interface_t {
    // Updated on initialisation only
    uint32_t  channel_count;                  /* Number of audio channels */
    uint32_t  max_fifo_depth;                 /* Length of buffer[] in channel_counts */
    uint32_t  ideal_freq_input;               /* Ideal producer freq in Hz */
    
    // Updated on the producer side only
    int       skip_ctr;                       /* Set to indicate initialisation runs */
    uint32_t  write_ptr;                      /* Write index in the buffer */
    int32_t   converted_sample_number;        /* Sample number counter of producer */
    int64_t   marked_converted_sample_time;   /* Sample time calculated by producer */
    int32_t   marked_converted_sample_number; /* Sample number that the sample time relates to  */
    int64_t   last_phase_error;               /* previous error, used for proportional */
    uint32_t  last_timestamp;                 /* Last time stamp, used for proportional */
    uint64_t  fs_ratio;                       /* Current ratio of frequencies */
    int64_t   fractional_credit;              /* Estimate of fractional samples held by ASRC */

    // Updated on the consumer side only
    uint32_t  read_ptr;                       /* Read index in the buffer */
    uint32_t  output_sample_number;           /* Current consumer output sample */
    uint32_t  sample_timestamp;               /* Timestamp calculated by consumer */
    int32_t   sample_number;                  /* Sample number of the timestamp */

    // Set by producer, reset by consumer
    uint32_t  reset;                          /* Set to 1 if consumer wants a reset */
    uint32_t  sample_valid;                   /* Set to 1 if consumer has made a timestamp */

    // Updated from both sides
    int32_t   buffer[0];                      /* Buffer of data */
};

#endif
