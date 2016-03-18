/*
 * variable_buffer_size.xc
 *
 *  Created on: Mar 17, 2016
 *      Author: Ed
 */
#include <xs1.h>
#include <stddef.h>
#include <string.h>
#include <debug_print.h>
#include <stdio.h>
#include <stdlib.h>


#define     ASRC_N_CHANNELS                 1  //Total number of audio channels to be processed by SRC (minimum 1)
#define     ASRC_N_INSTANCES                1  //Number of instances (each usually run a logical core) used to process audio (minimum 1)
#define     ASRC_CHANNELS_PER_INSTANCE      (ASRC_N_CHANNELS/ASRC_N_INSTANCES)
                                               //Calcualted number of audio channels processed by each core
#define     ASRC_N_IN_SAMPLES               4 //Number of samples per channel in each block passed into SRC each call
                                               //Must be a power of 2 and minimum value is 4 (due to two /2 decimation stages)
#define     ASRC_N_OUT_IN_RATIO_MAX         2  //Max ratio between samples out:in per processing step (44.1->192 is worst case)
#define     ASRC_DITHER_SETTING             0  //No output dithering of samples from 32b to 24b
#define     ASRC_MAX_BLOCK_SIZE             (ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX)
#define     OUT_FIFO_SIZE                   (ASRC_MAX_BLOCK_SIZE * 4)  //Size per channel of block2serial output FIFO


typedef interface block_transfer_if {
    int * unsafe push(int * unsafe p_next, const unsigned n_samps);
} block_transfer_if;

typedef interface serial_transfer_pull_if {
    int pull(const size_t chan_idx);
} serial_transfer_pull_if;



//Interface for obtaining timing information about the serial stream
typedef interface sample_rate_enquiry_if {
    unsigned get_sample_count(int &elapsed_time_in_ticks); //Returns sample count and elapsed time in 10ns ticks since last query
    unsigned get_buffer_level(void); //Returns the buffer level. Note this will return zero for the serial2block function which uses a double-buffer
} sample_rate_enquiry_if;



typedef struct b2s_fifo_t{
    int * unsafe ptr_base;
    int * unsafe ptr_top_max;
    int * unsafe ptr_top_curr;
    int * unsafe ptr_rd;
    int * unsafe ptr_wr;
    int size_curr;
    int fill_level;
} b2s_fifo_t;

#define PASS    1
#define FAIL    0



//block2serial helper - sets FIFOs pointers to half and clears contents
unsafe{
    static inline void init_fifo(b2s_fifo_t *fifo, int * fifo_base, int size){

        fifo->ptr_base      = fifo_base;
        fifo->ptr_top_max   = fifo_base + size;
        fifo->ptr_top_curr  = fifo_base + size;
        fifo->ptr_rd        = fifo_base + (size >> 1);
        fifo->ptr_wr        = fifo_base;

        memset(fifo_base, 0, size * sizeof(int));
    }

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

    static inline unsigned confirm_wr_block_address(b2s_fifo_t *fifo){

        if (fifo->ptr_wr >= fifo->ptr_rd){                                      //rd_ptr behind, need to check for hitting top
            //printf("Fill above\n");

            if ((fifo->ptr_wr + ASRC_MAX_BLOCK_SIZE) >= fifo->ptr_top_max) {    //cannot fit next block in top, need to wrap
                printf("Write wrap\n");
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
            //printf("Fill below\n");
            //printf("wr + ASRC_MAX_BLOCK_SIZE=%p, rd=%p\n", fifo->ptr_wr + ASRC_MAX_BLOCK_SIZE, fifo->ptr_rd);
            if((fifo->ptr_wr + ASRC_MAX_BLOCK_SIZE) >= fifo->ptr_rd) {          //We hit rd_ptr
                return FAIL;

            }                                                                   //We have room
        }
        return PASS;
    }


//Used by block2serial of samples to pull from output FIFO filled by ASRC
    static unsigned pull_sample_from_fifo(b2s_fifo_t *fifo, int &samp) {

        samp = *fifo->ptr_rd;                                                   //read value from fifo
        (fifo->ptr_rd)++;                                                       //increment write pointer
        if (fifo->ptr_rd >= fifo->ptr_top_curr) {
            fifo->ptr_rd = fifo->ptr_base;                                      //wrap pointer
            printf("Read wrap\n");
        }
        if (fifo->ptr_rd == fifo->ptr_wr){                                      //We have hit write pointer
            return FAIL;                                                        //Underflow - assume will init after this so leave as is
        }
        return PASS;
    }


} //unsafe


static void print_fifo(b2s_fifo_t fifo){
    int curr_size, fill_level;
    {curr_size, fill_level} = get_fill_level(&fifo);

    printf("ptr_base=%p, ptr_top_max=%p, ptr_top_curr=%p, ptr_rd=%p. ptr_wr=%p, size_curr=0x%x, fill_level=0x%x\n",
            fifo.ptr_base, fifo.ptr_top_max, fifo.ptr_top_curr, fifo.ptr_rd, fifo.ptr_wr, curr_size, fill_level);
}


//Task that takes blocks of samples from SRC, buffers them in a FIFO and serves them up as a stream
//This task is marked as unsafe keep pointers in scope throughout function
[[distributable]]
#pragma unsafe arrays   //Performance optimisation for block2serial. Removes bounds check
unsafe void block2serial(server block_transfer_if i_block2serial[ASRC_N_INSTANCES], server serial_transfer_pull_if i_serial_out, server sample_rate_enquiry_if i_output_rate)
{

    int samps_b2s[ASRC_N_CHANNELS][OUT_FIFO_SIZE];   //FIFO buffer storag

    b2s_fifo_t b2s_fifo[ASRC_N_CHANNELS];            //Declare FIFO control stucts


    unsigned samp_count = 0;                    //Keeps track of number of samples passed through

    timer t_tick;                               //100MHz timer for keeping track of sample ime
    int t_last_count, t_this_count;             //Keeps track of time when querying sample count
    t_tick :> t_last_count;                     //Get time for zero samples counted


    for (unsigned i=0; i<ASRC_N_CHANNELS; i++) print_fifo(b2s_fifo[i]);

    for (unsigned i=0; i<ASRC_N_CHANNELS; i++)  //Initialise FIFOs
    {
        init_fifo(&b2s_fifo[i], samps_b2s[i], OUT_FIFO_SIZE);
    }
    for (unsigned i=0; i<ASRC_N_CHANNELS; i++) print_fifo(b2s_fifo[i]);

    while(1){
        select{
            //Request to pull one channel of a sample over serial
            case i_serial_out.pull(const unsigned chan_idx) -> int samp:
                if(chan_idx == 0){
                    t_tick :> t_this_count;         //Grab timestamp of request for channel 0 only
                    samp_count ++;                  //Keep track of number of samples served
                }

                unsigned success = 1;
                for (int i=0; i<ASRC_N_CHANNELS; i++){
                    success &= pull_sample_from_fifo(b2s_fifo, samp);
                }

                if (!success) {
                    printf("UNDERFLOW\n");                    //FIFO empty
                    for (int i=0; i<ASRC_N_CHANNELS; i++){
                        init_fifo(&b2s_fifo[i], samps_b2s[i], OUT_FIFO_SIZE);
                    }
                }
                //for (unsigned i=0; i<ASRC_N_CHANNELS; i++) {printf("pull "); print_fifo(b2s_fifo[i]);}

            break;

            //Request to push block of samples from SRC
            //selects over the entire array of interfaces
            case i_block2serial[int if_index].push(int * unsafe p_buffer_next, const unsigned n_samps) -> int * unsafe p_buffer_wr:

                //printf("wr=%p\n", b2s_fifo[if_index].ptr_wr);
                b2s_fifo[if_index].ptr_wr += n_samps;                             //Move on write pointer. We alreaady know we have space
                //printf("wr=%p\n", b2s_fifo[if_index].ptr_wr);
                unsigned result = confirm_wr_block_address(&b2s_fifo[if_index]); //Get next available block for write pointer
                    if (result == FAIL) {
                        printf("OVERFLOW\n");                    //FIFO full
                        for (int i=0; i<ASRC_N_CHANNELS; i++){
                            init_fifo(&b2s_fifo[i], samps_b2s[i], OUT_FIFO_SIZE);
                        }
                    }
                //printf("wr=%p\n", b2s_fifo[if_index].ptr_wr);

                for (unsigned i=0; i<ASRC_N_CHANNELS; i++) {printf("push "); print_fifo(b2s_fifo[i]);}
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
            case i_output_rate.get_buffer_level() -> unsigned fill_level:
                //Currently just reports the level of first FIFO. Each FIFO should be the same
            break;
        }
    }
}

unsafe{
void producer(client block_transfer_if i_block, chanend trig){
    int counter=0;
    int block_size[] = {4, 3, 4, 3, 4, 3, 7, 4, 3, 1, 8, 3 ,6};
    int * unsafe ptr_wr;

    printf("ASRC_MAX_BLOCK_SIZE=%d\n", ASRC_MAX_BLOCK_SIZE);

    ptr_wr = i_block.push(ptr_wr, 0);   //Get pointer to initial write buffer

    for (int i=0; i<sizeof(block_size) / sizeof(int); i++){
        int n = block_size[i];
        //printf("producer requesting to push %d samps = +0x%x\n", n, n*4);
        for (int j=0; j < n; j++){
            printf("producer ptr_wr=%p, val=%d\n", ptr_wr, 1000000 + counter);
            *ptr_wr = (1000000 + counter);
            ptr_wr++;
            counter++;
        }
        ptr_wr = i_block.push(ptr_wr, n);   //Get pointer to next write buffer
        delay_microseconds(600);
    }
    trig <: 0;
    trig :> int _;
    _Exit(0);
}
} //unsafe

void consumer(client serial_transfer_pull_if i_serial, chanend trig){
    int count = 0;
    delay_microseconds(1000);
    while (1){
        printf("consumer pull sample=%d\n", i_serial.pull(0));
        delay_microseconds(100);
        count+=1;
        if (count == 1000) _Exit(0);
    }
}


int main(void){

    block_transfer_if i_block[2];
    serial_transfer_pull_if i_serial;
    sample_rate_enquiry_if i_rate;
    chan trig;
    par{
        producer(i_block[0], trig);
        unsafe {block2serial(i_block, i_serial, i_rate);}
        consumer(i_serial, trig);
    }
    return 0;
}
