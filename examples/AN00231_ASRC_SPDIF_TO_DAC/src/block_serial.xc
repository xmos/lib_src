#include <xs1.h>
#include <string.h>
#include <debug_print.h>
#include <xscope.h>
#include "block_serial.h"

[[distributable]]
#pragma unsafe arrays   //Performance optimisation. Removes bounds check
void serial2block(server serial_transfer_push_if i_serial_in, client block_transfer_if i_block_transfer[ASRC_N_INSTANCES], server sample_rate_enquiry_if i_input_rate)
{
    int buffer[ASRC_N_INSTANCES][ASRC_CHANNELS_PER_INSTANCE * ASRC_N_IN_SAMPLES]; //Half of the double buffer used for transferring blocks to src
    memset(buffer, 0, sizeof(buffer));

#if (ASRC_N_INSTANCES == 2)
    int * movable p_buffer[ASRC_N_INSTANCES] = {buffer[0], buffer[1]};    //One half of the double buffer
#else
    int * movable p_buffer[ASRC_N_INSTANCES] = {buffer[0]};
#endif


    unsigned samp_count = 0;            //Keeps track of samples processed since last query
    unsigned buff_idx = 0;

    timer t_tick;                       //100MHz timer for keeping track of sample time
    int t_last_count, t_this_count;     //Keeps track of time when querying sample count
    t_tick :> t_last_count;             //Get time for zero samples counted

    int t0, t1; //debug

    while(1){
        select{
            //Request to receive all channels of one sample period into double buffer
            case i_serial_in.push(int sample[], const unsigned n_chan):
                t_tick :> t_this_count;     //Grab timestamp of this sample group
                for(int i=0; i < n_chan / ASRC_N_INSTANCES; i++){
                    for(int j=0; j < ASRC_N_INSTANCES; j++){
                        p_buffer[j][buff_idx + i] = sample[i * ASRC_N_INSTANCES + j];
                    }
                }
                buff_idx += n_chan / ASRC_N_INSTANCES;  //Move index on by number of samples received
                samp_count++;                       //Keep track of samples received

                if(buff_idx == (ASRC_CHANNELS_PER_INSTANCE * ASRC_N_IN_SAMPLES)){  //When full..
                    buff_idx = 0;
                    t_tick :> t0;
                    for(int i=0; i < ASRC_N_INSTANCES; i++){
                        i_block_transfer[i].push(p_buffer[i], (ASRC_CHANNELS_PER_INSTANCE * ASRC_N_IN_SAMPLES)); //Exchange with src task
                    }
                    t_tick :> t1;
                    //debug_printf("interface call time = %d\n", t1 - t0);
                }
            break;

            //Request to report number of samples processed since last time
            case i_input_rate.get_sample_count(int &elapsed_time_in_ticks) -> unsigned count:
                elapsed_time_in_ticks = t_this_count - t_last_count;  //Set elapsed time in 10ns ticks
                t_last_count = t_this_count;                          //Store for next time around
                count = samp_count;
                samp_count = 0;
            break;

            //Request to report buffer level. Note this is always zero because we do not have a FIFO here
            //This method is more useful for block2serial, which does have a FIFO. This should never be called.
            case i_input_rate.get_buffer_level() -> unsigned level:
                level = 0;
            break;
        }
    }
}



//block2serial helper - sets FIFOs pointers to half and clears contents
static inline void init_fifos( int * unsafe * unsafe wr_ptr, int * unsafe * unsafe rd_ptr, int * unsafe fifo_base, static const unsigned fifo_size_entries)
{
    unsafe{
        *wr_ptr = fifo_base;
        *rd_ptr = fifo_base + (fifo_size_entries / 2);
        memset(fifo_base, 0, fifo_size_entries * sizeof(int));
    }
    //debug_printf("Init FIFO\n");
}

//Used by block2serial to push samples from ASRC output into FIFO
static inline unsigned push_sample_into_fifo(int samp, int * unsafe * unsafe wr_ptr, int * unsafe rd_ptr, int * unsafe fifo_base, static const unsigned fifo_size_entries){
    unsigned success = 1;
    unsafe{
        **wr_ptr = samp;    //write value into fifo

        (*wr_ptr)++; //increment write pointer (by reference)
        if (*wr_ptr >= fifo_base + fifo_size_entries) *wr_ptr = fifo_base; //wrap pointer
        if (*wr_ptr == rd_ptr) {
            success = 0;
        }
    }
    return success;
}

//Used by block2serial of samples to pull from output FIFO filled by ASRC
static inline unsigned pull_sample_from_fifo(int &samp, int * unsafe wr_ptr, int * unsafe * unsafe rd_ptr, int * unsafe fifo_base, static const unsigned fifo_size_entries){
    unsigned success = 1;
    unsafe{
        samp = **rd_ptr;    //read value from fifo
        (*rd_ptr)++; //increment write pointer (by reference)
        if (*rd_ptr >= fifo_base + fifo_size_entries) *rd_ptr = fifo_base; //wrap pointer
        if (*rd_ptr == wr_ptr){
            success = 0;
        }
    }
    return success;
}

//Used by block2serial to find out how many samples we have in FIFO
static inline unsigned get_fill_level(int * unsafe wr_ptr, int * unsafe rd_ptr, int * const unsafe fifo_base, const unsigned fifo_size_entries){
    unsigned fill_level;
    if (wr_ptr >= rd_ptr){
        fill_level = wr_ptr - rd_ptr;
    }
    else{
        fill_level = fifo_size_entries + wr_ptr - rd_ptr;
    }
    return fill_level;
}

//Task that takes blocks of samples from SRC, buffers them in a FIFO and serves them up as a stream
//This task is marked as unsafe keep pointers in scope throughout function
[[distributable]]
#pragma unsafe arrays   //Performance optimisation
unsafe void block2serial(server block_transfer_if i_block2serial[ASRC_N_INSTANCES], client serial_transfer_pull_if i_serial_out, server sample_rate_enquiry_if i_output_rate)
{
    int samps_to_i2s[ASRC_N_CHANNELS][OUT_FIFO_SIZE];   //Circular buffers and pointers for output from block2serial
    int * unsafe ptr_base_samps_to_i2s[ASRC_N_CHANNELS];
    int * unsafe ptr_rd_samps_to_i2s[ASRC_N_CHANNELS];
    int * unsafe ptr_wr_samps_to_i2s[ASRC_N_CHANNELS];

    //Double buffer from output from SRC
    int to_i2s[ASRC_N_INSTANCES][ASRC_CHANNELS_PER_INSTANCE * ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX];
    memset(to_i2s, 0, sizeof(to_i2s));

#if (ASRC_N_INSTANCES == 2)
    int * movable p_to_i2s[ASRC_N_INSTANCES] = {to_i2s[0], to_i2s[1]}; //Movable pointer for swapping with SRC
#else
    int * movable p_to_i2s[ASRC_N_INSTANCES] = {to_i2s[0]};            //Movable pointer for swapping with SRC
#endif

    unsigned samp_count = 0;                    //Keeps track of number of samples passed through

    timer t_tick;                               //100MHz timer for keeping track of sample ime
    int t_last_count, t_this_count;             //Keeps track of time when querying sample count
    t_tick :> t_last_count;                     //Get time for zero samples counted

    for (unsigned i=0; i<ASRC_N_CHANNELS; i++)  //Initialise FIFOs
    {
        ptr_base_samps_to_i2s[i] = samps_to_i2s[i];
        init_fifos(&ptr_wr_samps_to_i2s[i], &ptr_rd_samps_to_i2s[i], ptr_base_samps_to_i2s[i], OUT_FIFO_SIZE);
    }

    while(1){
        select{
            //Request for pair of samples from I2S
            case i_serial_out.pull_ready():
                t_tick :> t_this_count;         //Grab timestamp of request from I2S

                unsigned success = 1;
                int samp[ASRC_N_CHANNELS];
                for (int i=0; i<ASRC_N_CHANNELS; i++) {
                    success &= pull_sample_from_fifo(samp[i], ptr_wr_samps_to_i2s[i], &ptr_rd_samps_to_i2s[i], ptr_base_samps_to_i2s[i], OUT_FIFO_SIZE);
                }
                //xscope_int(0, samp[0]);
                i_serial_out.do_pull(samp, ASRC_N_CHANNELS);
                if (!success) {
                    for (int i=0; i<ASRC_N_CHANNELS; i++) {   //Init all FIFOs if any of them have under/overflowed
                        debug_printf("-");                    //FIFO empty
                        init_fifos(&ptr_wr_samps_to_i2s[i], &ptr_rd_samps_to_i2s[i], ptr_base_samps_to_i2s[i], OUT_FIFO_SIZE);
                    }
                }
                samp_count ++;                  //Keep track of number of samples served to I2S
            break;

            //Request to push block of samples from SRC
            //selects over the entire array of interfaces
            case i_block2serial[int if_index].push(int * movable &p_buffer_other, const unsigned n_samps):
                if (if_index == 0) t_tick :> t_this_count;  //Grab time of sample
                int * movable tmp;
                tmp = move(p_buffer_other);         //First swap buffer pointers
                p_buffer_other = move(p_to_i2s[if_index]);
                p_to_i2s[if_index] = move(tmp);
                for(int i=0; i < n_samps; i++) {   //Get entire buffer
                    unsigned success = 1;                                   //Keep track of status of FIFO operations
                    for (int j=0; j < ASRC_CHANNELS_PER_INSTANCE; j++) {        //Push samples into FIFO
                        int samp = p_to_i2s[if_index][ASRC_CHANNELS_PER_INSTANCE * i + j];
                        int channel_num = if_index + j;
                        success &= push_sample_into_fifo(samp, &ptr_wr_samps_to_i2s[channel_num], ptr_rd_samps_to_i2s[channel_num],
                                ptr_base_samps_to_i2s[channel_num], OUT_FIFO_SIZE);
                    }

                    //xscope_int(LEFT, p_to_i2s[0]);

                    if (!success) {                 //One of the FIFOs has overflowed
                        for (int i=0; i<ASRC_N_CHANNELS; i++){
                            debug_printf("+");  //FIFO full
                            //debug_printf("push fail - buffer fill=%d, ", get_fill_level(ptr_wr_samps_to_i2s[i], ptr_rd_samps_to_i2s[i], ptr_base_samps_to_i2s[i], OUT_FIFO_SIZE));
                            init_fifos(&ptr_wr_samps_to_i2s[i], &ptr_rd_samps_to_i2s[i], ptr_base_samps_to_i2s[i], OUT_FIFO_SIZE);
                        }
                    }
                }
            break;

            //Request to report number of samples processed since last request
            case i_output_rate.get_sample_count(int &elapsed_time_in_ticks) -> unsigned count:
                elapsed_time_in_ticks = t_this_count - t_last_count;  //Set elapsed time in 10ns ticks
                t_last_count = t_this_count;                          //Store for next time around
                count = samp_count;
                samp_count = 0;
            break;

            //Request to report on the current buffer level
            case i_output_rate.get_buffer_level() -> unsigned fill_level:
                //Currently just reports the level of first FIFO. Each FIFO should be the same
                fill_level = get_fill_level(ptr_wr_samps_to_i2s[0], ptr_rd_samps_to_i2s[0], ptr_base_samps_to_i2s[0], OUT_FIFO_SIZE);
            break;
        }
    }
}
