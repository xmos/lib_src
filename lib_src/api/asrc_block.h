#ifndef _asrc_block_h__
#define _asrc_block_h__

#include <stdint.h>

typedef struct {
    uint32_t  write_ptr;                      /* Write index in the buffer */
    uint32_t  read_ptr;                       /* Read index in the buffer */
    uint32_t  channel_count;                  /* Number of audio channels */
    uint32_t  max_fifo_depth;                 /* Length of buffer[] */
    uint32_t  reset;                          /* Set to 1 if consumer wants a reset */
    uint32_t  output_sample_number;           /* Sample number counter of consumer */
    uint32_t  sample_timestamp;               /* Timestamp calculated by consumer */
    int32_t   sample_number;                  /* Sample number that the timestamp relates to */
    int32_t   converted_sample_number;        /* Sample number counter of producer */
    uint32_t  sample_valid;                   /* Set to 1 if producer has produced a timestamp */
    int32_t   last_phase_error;               /* Last phase error seen, used for proportional error */
    uint32_t  last_timestamp;                 /* Last time stamp, used for proportional */
    int32_t   marked_converted_sample_time;   /* Sample time calculated by producer */
    int32_t   marked_converted_sample_number; /* Sample number that the sample time relates to  */
    uint64_t  fs_ratio;                       /* Current ratio of frequencies */
    int64_t   fractional_credit;              /* Estimate of fractional sample used by ASRC */
    int32_t   buffer[0];                      /* Buffer of data */
} asrc_block_t;

/** Function that must be called to initialise the ASRC. You must pass in an array
 * of ``int [sizeof(asrc_block_t)/sizeof(int) + channel_count * fifo_max_depth]``
 * The same pointer must be used on both sides of the ASRC interface.
 *
 * @param   asrc_data_buffer    ASRC structure to be initialised
 * @param   channel_count       Number of audio channels
 * @param   max_fifo_depth      Length of the FIFO, delay when stable will be max_fifo_depth/2
 */
void asrc_init_buffer(asrc_block_t *asrc_data_buffer, int channel_count, int max_fifo_depth);

/** Function that must be called to deinitalise the ASRC
 *
 * @param   asrc_data_buffer    ASRC structure to be de-initialised
 */
void asrc_exit_buffer(asrc_block_t *asrc_data_buffer);

/** Function that provides the next four (multi-channel) samples to the ASRC.
 * This function must be called at exactly the input frequency divided by four.
 *
 * @param   asrc_data_buffer    ASRC structure to push the samples into 
 * @param   samples             The samples stored as an 2D array of four frames
 *                              each frame containing channel_count samples.
 */
void asrc_add_quad_input(asrc_block_t *asrc_data_buffer, int32_t *samples);


/** Function that extracts an output sample from the ASRC.
 * This function must be called at exactly the output frequency.
 *
 * @param   asrc_data_buffer    ASRC structure to be de-initialised
 * @param   samples             The array where the frame with output samples will be stored.
 */
void asrc_get_single_output(asrc_block_t *asrc_data_buffer, int32_t *samples);

#endif
