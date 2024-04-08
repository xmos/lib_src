// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/chanend.h>
#include <xcore/parallel.h>
#include <xcore/assert.h>
#include <xcore/hwtimer.h>
#include <xcore/interrupt.h>
#include <xcore/interrupt_wrappers.h>
#include <xcore/triggerable.h>

#include <stdio.h>
#include <print.h>

#include <stdbool.h>
#include <string.h>

#include "asrc_task.h"
#include "asrc_timestamp_interpolation.h"

#ifdef DEBUG_ASRC_TASK
#define dprintf(...)   printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif


static int frequency_to_fs_code(int frequency) {
    if(frequency == 44100) {
        return  FS_CODE_44;
    } else if(frequency == 48000) {
        return  FS_CODE_48;
    } else if(frequency == 88200) {
        return  FS_CODE_88;
    } else if(frequency == 96000) {
        return  FS_CODE_96;
    } else if(frequency == 176400) {
        return  FS_CODE_176;
    } else if(frequency == 192000) {
        return  FS_CODE_192;
    } else {
        xassert(0);
    }
    return -1;
}


// Structure used for thread scheduling of parallel ASRC
typedef struct schedule_info_t{
    int num_channels;
    int channel_start_idx;
} schedule_info_t;


// Generates a schedule based on the number of channels of ASRC needed vs available threads
int calculate_job_share(int asrc_channel_count,
                        schedule_info_t *schedule){
    int channels_per_first_job = (asrc_channel_count + MAX_ASRC_THREADS - 1) / MAX_ASRC_THREADS; // Rounded up channels per max jobs
    int channels_per_last_job = 0;

    int num_jobs = 0;
    while(asrc_channel_count > 0){
        channels_per_last_job = asrc_channel_count;
        asrc_channel_count -= channels_per_first_job;
        schedule[num_jobs].num_channels = channels_per_first_job;
        schedule[num_jobs].channel_start_idx = channels_per_first_job * num_jobs;
        num_jobs++;
    };
    schedule[num_jobs - 1].num_channels = channels_per_last_job;

    return num_jobs;
}


// A single worker thread which operates on a group of channels in parallel with other worker threads
DECLARE_JOB(do_asrc_group, (schedule_info_t*, uint64_t, asrc_in_out_t*, unsigned, int*, asrc_ctrl_t*));
void do_asrc_group(schedule_info_t *schedule, uint64_t fs_ratio, asrc_in_out_t * asrc_io, unsigned input_write_idx, int* num_output_samples, asrc_ctrl_t asrc_ctrl[]){

    // Make copies for readability
    int num_worker_channels = schedule->num_channels;
    int worker_channel_start_idx = schedule->channel_start_idx;

    // Pack into the frame this instance of ASRC expects
    int input_samples[ASRC_N_IN_SAMPLES * MAX_ASRC_CHANNELS_TOTAL];
    for(int i = 0; i < ASRC_N_IN_SAMPLES * num_worker_channels; i++){
        // int rd_idx = i % num_worker_channels + (i / num_worker_channels) * asrc_io->asrc_channel_count + worker_channel_start_idx; 
        int rd_idx = i + (asrc_io->asrc_channel_count - num_worker_channels) * (i / num_worker_channels) + worker_channel_start_idx; // Optimisation of above
        input_samples[i] = asrc_io->input_samples[input_write_idx][rd_idx];
    }

    // Declare output buffer suitable for this instance
    int output_samples[SRC_MAX_NUM_SAMPS_OUT * MAX_ASRC_CHANNELS_TOTAL];
    // Do the ASRC for this group of samples
    *num_output_samples = asrc_process(input_samples, output_samples, fs_ratio, asrc_ctrl);

    // Unpack to combined output frame
    for(int i = 0; i < *num_output_samples * num_worker_channels; i++){
        // int wr_idx = i % num_worker_channels + (i / num_worker_channels) * asrc_io->asrc_channel_count + worker_channel_start_idx;
        int wr_idx = i + (asrc_io->asrc_channel_count - num_worker_channels) * (i / num_worker_channels) + worker_channel_start_idx; // Optimisation of above
        asrc_io->output_samples[wr_idx] = output_samples[i];
    }
}


// Wrapper which forks and joins the parallel ASRC worker functions
// Only about 55 ticks ticks overhead to fork and join at 120MIPS 8 channels/ 4 threads
int par_asrc(int num_jobs, schedule_info_t schedule[], uint64_t fs_ratio, asrc_in_out_t * asrc_io, unsigned input_write_idx, asrc_ctrl_t asrc_ctrl[MAX_ASRC_THREADS][SRC_MAX_SRC_CHANNELS_PER_INSTANCE]){
    int num_output_samples = 0;

    switch(num_jobs){
        case 0:
            return 0; // Nothing to do
        break;
        case 1:
            PAR_JOBS(
                PJOB(do_asrc_group, (&schedule[0], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[0]))
            );
        break;
#if MAX_ASRC_THREADS > 1
        case 2:
            PAR_JOBS(
                PJOB(do_asrc_group, (&schedule[0], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[0])),
                PJOB(do_asrc_group, (&schedule[1], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[1]))
            );
        break;
#endif
#if MAX_ASRC_THREADS > 2
        case 3:
            PAR_JOBS(
                PJOB(do_asrc_group, (&schedule[0], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[0])),
                PJOB(do_asrc_group, (&schedule[1], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[1])),
                PJOB(do_asrc_group, (&schedule[2], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[2]))
            );
        break;
#endif
#if MAX_ASRC_THREADS > 3
        case 4:
            PAR_JOBS(
                PJOB(do_asrc_group, (&schedule[0], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[0])),
                PJOB(do_asrc_group, (&schedule[1], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[1])),
                PJOB(do_asrc_group, (&schedule[2], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[2])),
                PJOB(do_asrc_group, (&schedule[3], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[3]))
            );
        break;
#endif
#if MAX_ASRC_THREADS > 4
        case 5:
            PAR_JOBS(
                PJOB(do_asrc_group, (&schedule[0], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[0])),
                PJOB(do_asrc_group, (&schedule[1], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[1])),
                PJOB(do_asrc_group, (&schedule[2], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[2])),
                PJOB(do_asrc_group, (&schedule[3], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[3])),
                PJOB(do_asrc_group, (&schedule[4], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[4]))
            );
        break;
#endif
#if MAX_ASRC_THREADS > 5
        case 6:
            PAR_JOBS(
                PJOB(do_asrc_group, (&schedule[0], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[0])),
                PJOB(do_asrc_group, (&schedule[1], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[1])),
                PJOB(do_asrc_group, (&schedule[2], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[2])),
                PJOB(do_asrc_group, (&schedule[3], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[3])),
                PJOB(do_asrc_group, (&schedule[4], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[4])),
                PJOB(do_asrc_group, (&schedule[5], fs_ratio, asrc_io, input_write_idx, &num_output_samples, asrc_ctrl[5]))
            );
        break;
#endif
        default:
            xassert(0); // Too many jobs specified
        break;
    }

    // Note this value is written by all workers however all workers have the same ratio so will always be equal.
    return num_output_samples;
}


// Called from consumer side. Produces samples and returns channel count
int pull_samples(asrc_in_out_t * asrc_io, asynchronous_fifo_t * fifo, int32_t *samples, uint32_t output_frequency, int32_t consume_timestamp){
    asrc_io->output_frequency = output_frequency;
    if (asrc_io->ready_flag_configured){
        asynchronous_fifo_consumer_get(fifo, samples, consume_timestamp);
        return asrc_io->asrc_channel_count;
    }
    return 0;
}


// Consumer side FIFO reset and clear contents
void reset_asrc_fifo_consumer(asynchronous_fifo_t * fifo){
    asynchronous_fifo_reset_consumer(fifo);
    memset(fifo->buffer, 0, fifo->channel_count * fifo->max_fifo_depth * sizeof(int));
}

// Default implementation of receive (called from ASRC) which receives samples and config over a channel. This is overridable.
ASRC_TASK_ISR_CALLBACK_ATTR 
unsigned receive_asrc_input_samples_cb_default(chanend_t c_asrc_input, asrc_in_out_t *asrc_io, unsigned *new_input_rate){
    static unsigned asrc_in_counter = 0;

    // Get format and timing data from channel
    *new_input_rate = chanend_in_word(c_asrc_input);
    asrc_io->input_timestamp = chanend_in_word(c_asrc_input);
    asrc_io->input_channel_count = chanend_in_word(c_asrc_input);

    // Pack into array properly LRLRLRLR for 2ch or 123412341234 for 4ch etc.
    for(int i = 0; i < asrc_io->input_channel_count; i++){
        int idx = i + asrc_io->input_channel_count * asrc_in_counter;
        asrc_io->input_samples[asrc_io->input_write_idx][idx] = chanend_in_word(c_asrc_input);
    }

    if(++asrc_in_counter == SRC_N_IN_SAMPLES){
        asrc_in_counter = 0;
    }

    return asrc_in_counter;
}

// Structure used for holding the vars needed for the ASRC_TASK receive_asrc_input_samples() callback.
// This is needed because we can only pass a single pointer to an ISR.
typedef struct asrc_receive_samples_ctx_t{
    chanend_t c_asrc_input; // The chanend from which samples are received (streaming)
    chanend_t c_buff_idx;   // The chanend used to notify asrc_processor that a new block of samples is available
    asrc_in_out_t *asrc_io; // The ASRC IO state including buffers
} asrc_receive_samples_ctx_t;

// This is fired each time a sample is received (triggered by first channel token)
DEFINE_INTERRUPT_CALLBACK(ASRC_ISR_GRP, asrc_samples_rx_isr_handler, app_data){

    // Extract pointers and resource IDs and callback function pointer.
    asrc_receive_samples_ctx_t *asrc_receive_samples_ctx = app_data;
    chanend_t c_asrc_input = asrc_receive_samples_ctx->c_asrc_input;
    chanend_t c_buff_idx = asrc_receive_samples_ctx->c_buff_idx;
    asrc_in_out_t *asrc_io = asrc_receive_samples_ctx->asrc_io;
    ASRC_TASK_ISR_CALLBACK_ATTR asrc_task_produce_isr_cb_t receive_asrc_input_samples_cb = asrc_io->asrc_task_produce_cb;
    
    // Always consume samples so we don't apply backpressure to the producer
    // Call the user defined receive samples callback.
    ASRC_TASK_ISR_CALLBACK_ATTR
    unsigned asrc_in_counter = receive_asrc_input_samples_cb(c_asrc_input, asrc_io, &(asrc_io->input_frequency));

    // Only forward on to ASRC if it is ready (to avoid deadlock)
    if(asrc_in_counter == 0 && asrc_io->ready_flag_to_receive){
        // Note if you ever find the code has stopped here then this is due to the time required to ASRC process the input frame
        // is longer than the period of the frames coming in. To remedy this you need to increase ASRC processing resources or reduce
        // the processing requirement. If you are using XCORE-200, consider using xcore.ai for more than 2x the ASRC performance. 
        // Notify ASRC main loop of new frame
        chanend_out_byte(c_buff_idx, (uint8_t)asrc_io->input_write_idx);
        asrc_io->input_write_idx ^= 1; // Swap buffers
    }
}


// Keep receiving samples until input format is good
static inline void asrc_wait_for_valid_config(chanend_t c_buff_idx, uint32_t *input_frequency, uint32_t *output_frequency, asrc_in_out_t *asrc_io){
    asrc_io->ready_flag_to_receive = 1; // Signal we are ready to consume a frame of input samples

    do{
        chanend_in_byte(c_buff_idx); // Receive frame from source (receive_asrc_input_samples_cb)
        *input_frequency = asrc_io->input_frequency; // Extract input rate
        asrc_io->asrc_channel_count = asrc_io->input_channel_count; // Extract input channel count
        *output_frequency = asrc_io->output_frequency;
        delay_microseconds(2); // Hold off reading c_buff_idx for half of a minimum frame period. TODO: why is this needed?
    } while(*input_frequency == 0 ||
            *output_frequency == 0 ||
            asrc_io->asrc_channel_count == 0);

    xassert(asrc_io->asrc_channel_count <= MAX_ASRC_CHANNELS_TOTAL); // Too many channels requested
    frequency_to_fs_code(*input_frequency);  // This will assert if invalid
    frequency_to_fs_code(*output_frequency); // This will assert if invalid

    asrc_io->ready_flag_to_receive = 0;
}


// Check to see if input params have changed since last process
static inline bool asrc_detect_format_change(uint32_t input_frequency, uint32_t output_frequency, asrc_in_out_t *asrc_io){
    if( asrc_io->input_frequency != input_frequency || 
        asrc_io->input_channel_count != asrc_io->asrc_channel_count ||
        asrc_io->output_frequency != output_frequency){

        return true;
    } else {
        return false;
    }
}


// Main ASRC task. Defined as ISR friendly because we interrupt it receive samples
DEFINE_INTERRUPT_PERMITTED(ASRC_ISR_GRP, void, asrc_processor,
                            chanend_t c_asrc_input,
                            asrc_in_out_t *asrc_io,
                            chanend_t c_buff_idx,
                            asynchronous_fifo_t * fifo){
    
    uint32_t input_frequency = 0;   // Set to invalid for now. We will get rates supplied by producer and consumer.
    uint32_t output_frequency = 0;

    // Used for calculating the timestamp interpolation between major frequency conversions
    const int interpolation_ticks_2D[6][6] = {
        {  2268, 2268, 2268, 2268, 2268, 2268},
        {  2083, 2083, 2083, 2083, 2083, 2083},
        {  2268, 2268, 1134, 1134, 1134, 1134},
        {  2083, 2083, 1042, 1042, 1042, 1042},
        {  2268, 2268, 1134, 1134,  567,  567},
        {  2083, 2083, 1042, 1042,  521,  521}
    };
    
    // Setup a pointer to a struct so the ISR can access these elements
    asrc_receive_samples_ctx_t asrc_receive_samples_ctx = {c_asrc_input, c_buff_idx, asrc_io};

    // Enable interrupt on channel receive token (sent from ISR)
    triggerable_setup_interrupt_callback(c_asrc_input, &asrc_receive_samples_ctx, INTERRUPT_CALLBACK(asrc_samples_rx_isr_handler));
    triggerable_enable_trigger(c_asrc_input);
    interrupt_unmask_all();

    // This is a forever loop consisting of init -> forever process, until format change when we return to init
    while(1){
        asrc_wait_for_valid_config(c_buff_idx, &input_frequency, &output_frequency, asrc_io);

        //// Extract frequency info
        dprintf("Input fs: %lu Output fs: %lu\n", input_frequency, output_frequency);
        int inputFsCode = frequency_to_fs_code(input_frequency);
        int outputFsCode = frequency_to_fs_code(output_frequency);
        int interpolation_ticks = interpolation_ticks_2D[inputFsCode][outputFsCode];
        
        //// FIFO init
        dprintf("FIFO init channels: %d length: %ld\n", asrc_io->asrc_channel_count, fifo->max_fifo_depth);
        asynchronous_fifo_init(fifo, asrc_io->asrc_channel_count, fifo->max_fifo_depth);
        asynchronous_fifo_init_PID_fs_codes(fifo, inputFsCode, outputFsCode);

        //// Parallel scheduler init
        schedule_info_t schedule[MAX_ASRC_THREADS];
        int num_jobs = calculate_job_share(asrc_io->asrc_channel_count, schedule);
        dprintf("num_jobs: %d, MAX_ASRC_THREADS: %d, asrc_channel_count: %d\n", num_jobs, MAX_ASRC_THREADS, asrc_io->asrc_channel_count);
        for(int i = 0; i < num_jobs; i++){
            dprintf("schedule: %d, num_channels: %d, channel_start_idx: %d\n", i, schedule[i].num_channels, schedule[i].channel_start_idx);
        }
        int max_channels_per_instance = schedule[0].num_channels;
        dprintf("max_channels_per_instance: %d\n", max_channels_per_instance);

        //// ASRC init
        asrc_state_t sASRCState[MAX_ASRC_THREADS][SRC_MAX_SRC_CHANNELS_PER_INSTANCE];                                   // ASRC state machine state
        int iASRCStack[MAX_ASRC_THREADS][SRC_MAX_SRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES ];// Buffer between filter stages
        asrc_ctrl_t sASRCCtrl[MAX_ASRC_THREADS][SRC_MAX_SRC_CHANNELS_PER_INSTANCE];                                     // Control structure
        asrc_adfir_coefs_t asrc_adfir_coefs[MAX_ASRC_THREADS];                                                          // Adaptive filter coefficients

        uint64_t fs_ratio = 0;
        int ideal_fs_ratio = 0;
        int error = 0;

        for(int instance = 0; instance < num_jobs; instance++){
            for(int ch = 0; ch < max_channels_per_instance; ch++){
                // Set state, stack and coefs into ctrl structure
                sASRCCtrl[instance][ch].psState                   = &sASRCState[instance][ch];
                sASRCCtrl[instance][ch].piStack                   = iASRCStack[instance][ch];
                sASRCCtrl[instance][ch].piADCoefs                 = asrc_adfir_coefs[instance].iASRCADFIRCoefs;
            }
            fs_ratio = asrc_init(inputFsCode, outputFsCode, sASRCCtrl[instance], max_channels_per_instance, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);
            dprintf("ASRC init instance: %d ptr: %p\n", instance, sASRCCtrl[instance]);
        }

        //// Timing check vars. Includes ASRC, timestamp interpolation and FIFO push
        int32_t asrc_process_time_limit = (XS1_TIMER_HZ / input_frequency) * SRC_N_IN_SAMPLES;
        dprintf("ASRC process_time_limit: %ld\n", asrc_process_time_limit);
        int32_t asrc_peak_processing_time = 0;

        ideal_fs_ratio = (fs_ratio + (1<<31)) >> 32;
        dprintf("ideal_fs_ratio: %d\n", ideal_fs_ratio);

        const int xscope_used = 0; // Vestige of ASRC API. TODO - cleanup in future when lib_src is tidied

        asrc_io->ready_flag_to_receive = 1; // Signal we are ready to consume a frame of input samples
        asrc_io->ready_flag_configured = 1; // SIgnal we are ready to produce

        //// Run until format change detected
        while(1){
            // Wait for block of samples. We will get the buffer index of the newly written samples from receive_asrc_input_samples_cb
            unsigned input_write_idx = (unsigned)chanend_in_byte(c_buff_idx);

            // Check for format changes - do before we process in case things have changed
            if(asrc_detect_format_change(input_frequency, output_frequency, asrc_io)){
                asrc_io->ready_flag_configured = 0;
                break;
            }

            int32_t t0 = get_reference_time();
            int num_output_samples = par_asrc(num_jobs, schedule, fs_ratio, asrc_io, input_write_idx, sASRCCtrl);
            int ts = asrc_timestamp_interpolation(asrc_io->input_timestamp, sASRCCtrl[0], interpolation_ticks);
            // Only push to FIFO if we have samples (FIFO has a bug) otherwise hold last error value
            if(num_output_samples){
                error = asynchronous_fifo_producer_put(fifo, &asrc_io->output_samples[0], num_output_samples, ts, xscope_used);
            }

            fs_ratio = (((int64_t)ideal_fs_ratio) << 32) + (error * (int64_t) ideal_fs_ratio);

            // Watermark for ASRC peak execution time
            int32_t t1 = get_reference_time();
            if(t1 - t0 > asrc_peak_processing_time){
                asrc_peak_processing_time = t1 - t0;
                #ifdef DEBUG_ASRC_TASK
                printintln(asrc_peak_processing_time);
                #endif
                // xassert(asrc_peak_processing_time <= asrc_process_time_limit); // Optional assert on timing failure.
                (void)asrc_peak_processing_time; // Remove compiler warning if above not used.
                (void)asrc_process_time_limit; // Remove compiler warning
            }
        } // while !asrc_detect_format_change()
    } // while 1
}


// Wrapper to setup ISR->task signalling chanend and use ISR friendly call to function 
void asrc_task(chanend_t c_asrc_input, asrc_in_out_t *asrc_io, asynchronous_fifo_t *fifo, unsigned fifo_length){
    // Check callback is init'd. If not, use default implementation.
    if (asrc_io->asrc_task_produce_cb == NULL){
        asrc_io->asrc_task_produce_cb = receive_asrc_input_samples_cb_default;
    }
    // We use a single chanend to send the buffer IDX from the ISR of this task back to asrc task and sync
    chanend_t c_buff_idx = chanend_alloc();
    chanend_set_dest(c_buff_idx, c_buff_idx); // Loopback chanend to itself - we use this as a shallow event driven FIFO
    // This is a workaround where only 4 params can be sent to INTERRUPT_PERMITTED(). So set it struct and extract in asrc_processor_() init
    // http://bugzilla/show_bug.cgi?id=18745
    fifo->max_fifo_depth = fifo_length;
    // Run the ASRC task with stack set aside for an ISR
    INTERRUPT_PERMITTED(asrc_processor)(c_asrc_input, asrc_io, c_buff_idx, fifo);
}

// Register a custom rx function for ASRC task
void init_asrc_io_callback(asrc_in_out_t *asrc_io, asrc_task_produce_isr_cb_t asrc_rx_fp){
    asrc_io->asrc_task_produce_cb = asrc_rx_fp;
}

// Default send samples to ASRC function.
void send_asrc_input_samples_default(chanend_t c_asrc_input,
                                    unsigned input_frequency,
                                    int32_t input_timestamp,
                                    unsigned input_channel_count,
                                    int32_t *input_samples){
    // Send format info
    chanend_out_word(c_asrc_input, input_frequency);
    chanend_out_word(c_asrc_input, input_timestamp);
    chanend_out_word(c_asrc_input, input_channel_count);

    // Send samples
    for(int i = 0; i < input_channel_count; i++){
        chanend_out_word(c_asrc_input, input_samples[i]);
    }

}
