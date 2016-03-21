#include <xs1.h>
#include <string.h>
#include <debug_print.h>
#include <xscope.h>
#include "block_serial.h"

extern out port port_debug_tile_0;

[[distributable]]
#pragma unsafe arrays   //Performance optimisation for serial2block. Removes bounds check
void serial2block(server serial_transfer_push_if i_serial_in, client block_transfer_if i_block_transfer[ASRC_N_INSTANCES], server sample_rate_enquiry_if i_input_rate)
{

    int * unsafe buff_ptr[ASRC_N_INSTANCES];//Array of buffer pointers. These point to the input double buffers of ASRC instances

    unsigned samp_count = 0;            //Keeps track of samples processed since last query
    unsigned buff_idx = 0;              //Index keeping track of input samples per block

    timer t_tick;                       //100MHz timer for keeping track of sample time
    int t_last_count, t_this_count;     //Keeps track of time when querying sample count
    t_tick :> t_last_count;             //Get time for zero samples counted

    for (int i=0; i<ASRC_N_INSTANCES; i++) {
        buff_ptr[i] = i_block_transfer[i].push(0);  //Get initial buffers to write to
    }

    int t0, t1; //debug

    while(1){
        select{
            //Request to receive one channels of one sample period into double buffer
            case i_serial_in.push(int sample, const unsigned chan_idx):
                if (chan_idx == 0 ) {
                    t_tick :> t_this_count;     //Grab timestamp of this sample group
                    samp_count++;                       //Keep track of samples received
                }

                unsafe{
                    *(buff_ptr[chan_idx % ASRC_N_INSTANCES] + buff_idx) = sample;
                }
                if (chan_idx == ASRC_CHANNELS_PER_INSTANCE - 1) buff_idx++;  //Move index when all channels received

                if(buff_idx == ASRC_N_IN_SAMPLES){  //When full..
                    buff_idx = 0;
                    t_tick :> t0;
                    for(int i=0; i < ASRC_N_INSTANCES; i++) unsafe{//Get new buffer pointers
                        buff_ptr[i] = i_block_transfer[i].push(0);
                    }
                    t_tick :> t1;
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
            case i_input_rate.get_buffer_level() -> {unsigned curr_size, unsigned fill_level}:
                curr_size = 0;
                fill_level = 0;
            break;
        }
    }
}




unsafe{
    //block2serial helper - sets FIFOs pointers to half and clears contents
    static inline void init_fifo(b2s_fifo_t *fifo, int * fifo_base, int size){

        fifo->ptr_base      = fifo_base;
        fifo->ptr_top_max   = fifo_base + size;
        fifo->ptr_top_curr  = fifo_base + size;
        fifo->ptr_rd        = fifo_base + (size >> 1);
        fifo->ptr_wr        = fifo_base;

        memset(fifo_base, 0, size * sizeof(int));
    }

    //block2serial helper - returns size and fill level
    {int, int} static get_fill_level(b2s_fifo_t *fifo){

        int size, fill_level;
        size = fifo->ptr_top_curr - fifo->ptr_base;                             //Total current size if FIFO
        if (fifo->ptr_wr >= fifo->ptr_rd){
            fill_level = fifo->ptr_wr - fifo->ptr_rd;                           //Fill level if write pointer ahead of read pointer
        }
        else {                                                                  //Fill level if read pointer ahead of write pointer
            fill_level = (fifo->ptr_top_curr - fifo->ptr_rd) + (fifo->ptr_wr - fifo->ptr_base);
        }
        return {size, fill_level};
    }

    //block2serial helper - checks supplied wr_ptr and modifies if necessary
    static inline unsigned confirm_wr_block_address(b2s_fifo_t *fifo){

        if (fifo->ptr_wr >= fifo->ptr_rd){                                      //rd_ptr behind, need to check for hitting top
            if ((fifo->ptr_wr + ASRC_MAX_BLOCK_SIZE) >= fifo->ptr_top_max) {    //cannot fit next block in top, need to wrap
                if((fifo->ptr_base + ASRC_MAX_BLOCK_SIZE) > fifo->ptr_rd) {     //No space at bottom either
                    return FAIL;                                                //Overflow. We expect to init FIFOs so leave vals as is
                }
                                                                                //Space available at bottom
                fifo->ptr_top_curr = fifo->ptr_wr;                              //Set new top of FIFO at last write pointer location
                fifo->size_curr = fifo->ptr_top_curr - fifo->ptr_base;          //Shrink size to current top - base
                fifo->ptr_wr = fifo->ptr_base;                                  //Start writing at base

            }
            else {                                                              //write space can fit at top
            }
        }

        else {                                                                  //rd_ptr ahead, need to check for hitting rd_ptr
            if((fifo->ptr_wr + ASRC_MAX_BLOCK_SIZE) >= fifo->ptr_rd) {          //We hit rd_ptr
                return FAIL;

            }                                                                   //We have room
        }
        return PASS;
    }


    //block2serial helper - pulls a single sample from the FIFO
    static unsigned pull_sample_from_fifo(b2s_fifo_t *fifo, int &samp) {

        samp = *fifo->ptr_rd;                                                   //read value from fifo
        (fifo->ptr_rd)++;                                                       //increment write pointer
        if (fifo->ptr_rd >= fifo->ptr_top_curr) {
            fifo->ptr_rd = fifo->ptr_base;                                      //wrap pointer
        }

        if (fifo->ptr_rd == fifo->ptr_wr){                                      //We have hit write pointer
            return FAIL;                                                        //Underflow - assume will init after this so leave as is
        }
        return PASS;
    }
} //unsafe region


//Task that takes blocks of samples from SRC, buffers them in a FIFO and serves them up as a stream
//This task is marked as unsafe keep pointers in scope throughout function
[[distributable]]
#pragma unsafe arrays   //Performance optimisation for block2serial. Removes bounds check
unsafe void block2serial(server block_transfer_if i_block2serial[ASRC_N_INSTANCES], server serial_transfer_pull_if i_serial_out, server sample_rate_enquiry_if i_output_rate)
{

    int samps_b2s[ASRC_N_CHANNELS][OUT_FIFO_SIZE];   //FIFO buffer storage

    b2s_fifo_t b2s_fifo[ASRC_N_CHANNELS];            //Declare FIFO control stucts


    unsigned samp_count = 0;                    //Keeps track of number of samples passed through

    timer t_tick;                               //100MHz timer for keeping track of sample ime
    int t_last_count, t_this_count;             //Keeps track of time when querying sample count
    t_tick :> t_last_count;                     //Get time for zero samples counted

    for (unsigned i=0; i<ASRC_N_CHANNELS; i++) { //Initialise FIFOs
        init_fifo(&b2s_fifo[i], samps_b2s[i], OUT_FIFO_SIZE);
    }

    while(1){
        select{
            //Request to pull one channel of a sample over serial
            case i_serial_out.pull(const unsigned chan_idx) -> int samp:
                if(chan_idx == 0){
                    t_tick :> t_this_count;         //Grab timestamp of request for channel 0 only
                    samp_count ++;                  //Keep track of number of samples served
                }

                unsigned success = pull_sample_from_fifo(&b2s_fifo[chan_idx], samp);
                if (success == FAIL) {  //FIFO empty
                    //debug_printf("-");
                    for (int i=0; i<ASRC_N_CHANNELS; i++){
                        init_fifo(&b2s_fifo[i], samps_b2s[i], OUT_FIFO_SIZE);
                    }
                }
            break;

            //Request to push block of samples from SRC
            //selects over the entire array of interfaces
            case i_block2serial[int if_index].push(const unsigned n_samps) -> int * unsafe p_buffer_wr:
                b2s_fifo[if_index].ptr_wr += n_samps;                               //Move on write pointer. We alreaady know we have space
                unsigned result = confirm_wr_block_address(&b2s_fifo[if_index]);    //Get next available block for write pointer
                    if (result == FAIL) {                                           //FIFO full
                        //debug_printf("+");
                        for (int i=0; i<ASRC_N_CHANNELS; i++){
                            init_fifo(&b2s_fifo[i], samps_b2s[i], OUT_FIFO_SIZE);
                        }
                    }
                p_buffer_wr = b2s_fifo[if_index].ptr_wr;
            break;

            //Request to report number of samples processed since last request
            case i_output_rate.get_sample_count(int &elapsed_time_in_ticks) -> unsigned count:
                elapsed_time_in_ticks = t_this_count - t_last_count;  //Set elapsed time in 10ns ticks
                t_last_count = t_this_count;                          //Store for next time around
                count = samp_count;
                samp_count = 0;
            break;

            //Request to report on the current buffer level
            case i_output_rate.get_buffer_level() -> {unsigned curr_size, unsigned fill_level}:
                //Currently just reports the level of first FIFO. Each FIFO should be the same
                {curr_size, fill_level} = get_fill_level(&b2s_fifo[0]);
            break;
        }
    }
}
