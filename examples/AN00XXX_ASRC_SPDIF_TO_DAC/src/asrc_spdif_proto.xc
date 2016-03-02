#include <xs1.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <platform.h>
#include <string.h>

#include <src.h>
#include <audio_codec.h>
#include <spdif.h>
#include <i2s.h>
#include <i2c.h>
#include <gpio.h>

#include <debug_print.h> //Enabled by -DDEBUG_PRINT_ENABLE=1 in Makefile
#include <xscope.h>

#define OUT_FIFO_SIZE           (SRC_N_OUT_IN_RATIO_MAX * SRC_N_IN_SAMPLES * 2)  //Size per channel of block2serial FIFO



#define DEFAULT_FREQ_HZ    48000
#define MCLK_FREQUENCY_48  24576000
#define MCLK_FREQUENCY_441 22579200


/* These port assignments all correspond to the Slicekit U16 Audio 8
   developement kit. Details can be found in the hardware manual
   for that board.

   The port assignments can be changed for a different port map.
*/

#define AUDIO_TILE                  0
#define SPDIF_TILE                  1

in buffered port:32  ports_i2s_adc[4]   = on tile[AUDIO_TILE]: {XS1_PORT_1I,
                                                     XS1_PORT_1J,
                                                     XS1_PORT_1K,
                                                     XS1_PORT_1L};
port port_i2s_mclk                       = on tile[AUDIO_TILE]: XS1_PORT_1F;
out buffered port:32 port_i2s_bclk       = on tile[AUDIO_TILE]: XS1_PORT_1H;
out buffered port:32 port_i2s_wclk       = on tile[AUDIO_TILE]: XS1_PORT_1G;
clock clk_i2s                            = on tile[AUDIO_TILE]: XS1_CLKBLK_1;

out buffered port:32 ports_i2s_dac[4]    = on tile[AUDIO_TILE]: {XS1_PORT_1M,
                                                     XS1_PORT_1N,
                                                     XS1_PORT_1O,
                                                     XS1_PORT_1P};
port port_i2c                            = on tile[AUDIO_TILE]: XS1_PORT_4A;

port port_audio_config                   = on tile[AUDIO_TILE]: XS1_PORT_8C;
/*
 * 0 DSD_MODE
 * 1 DAC_RST_N
 * 2 USB_SEL0
 * 3 USB_SEL1
 * 4 VBUS_OUT_EN
 * 5 PLL_SELECT
 * 6 ADC_RST_N
 * 7 MCLK_FSEL
 */

out buffered port:32 port_pll_ref        = on tile[AUDIO_TILE]: XS1_PORT_1A;
clock clk_mclk                           = on tile[AUDIO_TILE]: XS1_CLKBLK_2;
port port_spdif_rx                       = on tile[SPDIF_TILE]: XS1_PORT_1O;
clock clk_spdif_rx                       = on tile[SPDIF_TILE]: XS1_CLKBLK_1;
port port_debug                          = on tile[SPDIF_TILE]: XS1_PORT_1N;    //MIDI OUT

static const unsigned CODEC_I2C_DEVICE_ADDR = 0x18;
static const unsigned PLL_I2C_DEVICE_ADDR   = 0x4E;

static const enum codec_mode_t codec_mode = CODEC_IS_I2S_SLAVE;



typedef interface block_transfer_if {
    void push(int * movable &p_buffer, const unsigned n_samps);
} block_transfer_if;

typedef interface serial_transfer_push_if {
    void push(int samples[], const size_t n_samps);
} serial_transfer_push_if;

typedef interface serial_transfer_pull_if {
    [[notification]] slave void pull_ready();
    [[clears_notification]] void do_pull(int samples[], const size_t n_samps);
} serial_transfer_pull_if;

typedef interface sample_rate_enquiry_if {
    unsigned get_sample_count(int &elapsed_time_in_ticks);
    unsigned get_buffer_level(void);
} sample_rate_enquiry_if;

typedef interface fs_ratio_enquiry_if {
    unsigned get(unsigned nominal_fs_ratio);
} fs_ratio_enquiry_if;

typedef unsigned fs_ratio_t;

[[combinable]] void spdif_handler(streaming chanend c_spdif_rx, client serial_transfer_push_if i_serial_in);
[[distributable]] void serial2block(server serial_transfer_push_if i_serial_in, client block_transfer_if i_block_transfer, server sample_rate_enquiry_if i_input_rate);
void src(server block_transfer_if i_serial2block, client block_transfer_if i_block2serial, client fs_ratio_enquiry_if i_fs_ratio);
[[distributable]] unsafe void block2serial(server block_transfer_if i_block2serial, client serial_transfer_pull_if i_serial_out, server sample_rate_enquiry_if i_output_rate);
[[distributable]] void i2s_handler(server i2s_callback_if i2s, server serial_transfer_pull_if i_serial_out, client audio_codec_config_if i_codec);
[[combinable]]void rate_server(client sample_rate_enquiry_if i_spdif_rate, client sample_rate_enquiry_if i_output_rate, server fs_ratio_enquiry_if i_fs_ratio);

int main(void){
    serial_transfer_push_if i_serial_in;
    block_transfer_if i_serial2block, i_block2serial;
    serial_transfer_pull_if i_serial_out;
    sample_rate_enquiry_if i_sr_input, i_sr_i2s;
    fs_ratio_enquiry_if i_fs_ratio;
    interface audio_codec_config_if i_codec;
    interface i2c_master_if i_i2c[1];
    interface output_gpio_if i_gpio[8];    //See mapping of bits 0..7 above in port_audio_config
    interface i2s_callback_if i_i2s;

    streaming chan c_spdif_rx;

    par{
        on tile[SPDIF_TILE]: spdif_rx(c_spdif_rx, port_spdif_rx, clk_spdif_rx, DEFAULT_FREQ_HZ);
        on tile[SPDIF_TILE]: spdif_handler(c_spdif_rx, i_serial_in);
        on tile[SPDIF_TILE]: serial2block(i_serial_in, i_serial2block, i_sr_input);
        on tile[SPDIF_TILE]: src(i_serial2block, i_block2serial, i_fs_ratio);
        on tile[SPDIF_TILE]: unsafe {block2serial(i_block2serial, i_serial_out, i_sr_i2s);}

        on tile[AUDIO_TILE]: audio_codec_cs4384_cs5368(i_codec, i_i2c[0], codec_mode, i_gpio[0], i_gpio[1], i_gpio[6], i_gpio[7]);
        on tile[AUDIO_TILE]: i2c_master_single_port(i_i2c, 1, port_i2c, 10, 0 /*SCL*/, 1 /*SDA*/, 0);
        on tile[AUDIO_TILE]: output_gpio(i_gpio, 8, port_audio_config, null);
        on tile[AUDIO_TILE]: {
            i_gpio[5].output(0); //Select fixed local clock on MCLK mux
            i_gpio[2].output(0); //Output something to this interface (value is don't care) to avoid compiler warning of unused end
            i_gpio[3].output(0); //As above
            i_gpio[4].output(0); //As above
            configure_clock_src(clk_mclk, port_i2s_mclk); //Connect MCLK clock block to input pin
            start_clock(clk_mclk);
            debug_printf("Starting I2S\n");
            i2s_master(i_i2s, ports_i2s_dac, 1, ports_i2s_adc, 1, port_i2s_bclk, port_i2s_wclk, clk_i2s, clk_mclk);
        }
        on tile[AUDIO_TILE]: i2s_handler(i_i2s, i_serial_out, i_codec);
        on tile[AUDIO_TILE]: rate_server(i_sr_input, i_sr_i2s, i_fs_ratio);
    }
    return 0;
}


//Shim task to handle setup and streaming of I2S samples from block2serial to the I2S module
[[combinable]]
void spdif_handler(streaming chanend c_spdif_rx, client serial_transfer_push_if i_serial_in)
{

    int samples[2] = {0,0};                     //Array of input samples for SPDIF (L/R)
    size_t index = 0;                           //Index for above
    signed long sample;                         //Sample received from SPDIF


    while (1) {
        select {
            case spdif_receive_sample(c_spdif_rx, sample, index):
                samples[index] = sample;
                if (index == 1){                    //We have completed a L/R pair
                    i_serial_in.push(samples, 2);   //Push them into serial to block
                }
             break;
        }
    }
}

[[distributable]]
void serial2block(server serial_transfer_push_if i_serial_in, client block_transfer_if i_block_transfer, server sample_rate_enquiry_if i_input_rate)
{
    int buffer[SRC_N_CHANNELS * SRC_N_IN_SAMPLES];              //Half of the double buffer used for transferring blocks to src
    int * movable p_buffer = buffer;    //One half of the double buffer
    memset(p_buffer, 0, sizeof(buffer));

    unsigned samp_count = 0;            //Keeps track of samples processed since last query
    unsigned buff_idx = 0;

    timer t_tick;                       //100MHz timer for keeping track of sample ime
    int t_last_count, t_this_count;     //Keeps track of time when querying sample count
    t_tick :> t_last_count;             //Get time for zero samples counted

    int t0, t1; //debug

    while(1){
        select{
            //Request to receive group of samples into double buffer
            case i_serial_in.push(int sample[], const unsigned count):
                t_tick :> t_this_count;     //Grab timestamp of this sample
                for(int i=0; i<count; i++){
                    p_buffer[buff_idx + i] = sample[i];
                }
                buff_idx += count;          //Move index on by number of samples received
                samp_count++;               //Keep track of samples received

                if(buff_idx == (SRC_N_CHANNELS * SRC_N_IN_SAMPLES)){  //When full..
                    buff_idx = 0;
                    //for (int i=0; i<BUFF_SIZE; i++) debug_printf("buf[%d]=%d\n", i, p_buffer[i]);
                    t_tick :> t0;
                    i_block_transfer.push(p_buffer, (SRC_N_CHANNELS * SRC_N_IN_SAMPLES)); //Exchange with src task
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

int from_spdif[SRC_N_CHANNELS * SRC_N_IN_SAMPLES];                  //Double buffers for to block/serial tasks on each side
int to_i2s[SRC_N_CHANNELS * SRC_N_IN_SAMPLES * SRC_N_OUT_IN_RATIO_MAX];

void src(server block_transfer_if i_serial2block, client block_transfer_if i_block2serial, client fs_ratio_enquiry_if i_fs_ratio)
{
    int * movable p_from_spdif = from_spdif;    //Movable pointers for swapping ownership
    int * movable p_to_i2s = to_i2s;

    ASRCState_t     sASRCState[ASRC_CHANNELS_PER_CORE]; //ASRC state machine state
    int             iASRCStack[ASRC_CHANNELS_PER_CORE][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES]; //Buffer between filter stages
    ASRCCtrl_t      sASRCCtrl[ASRC_CHANNELS_PER_CORE];  //Control structure
    iASRCADFIRCoefs_t SiASRCADFIRCoefs;                 //Adaptive filter coefficients

    set_core_high_priority_on();                //Give me guarranteed 100MHz

    printf("ASRC_CHANNELS_PER_CORE=%d\n", ASRC_CHANNELS_PER_CORE);

    for(int ui = 0; ui < ASRC_CHANNELS_PER_CORE; ui++)
    unsafe {
        // Set state, stack and coefs into ctrl structure
        sASRCCtrl[ui].psState                   = &sASRCState[ui];
        sASRCCtrl[ui].piStack                   = iASRCStack[ui];
        sASRCCtrl[ui].piADCoefs                 = SiASRCADFIRCoefs.iASRCADFIRCoefs;
    }

    unsigned nominal_fs_ratio = asrc_init(1, 1, sASRCCtrl);

    printf("nominal_fs_ratio=0x%x\n", nominal_fs_ratio);


    unsigned do_dsp_flag = 0;                   //Flag to indiciate we are ready to process. Minimises blocking on pushg case below
    while(1){
        select{
            case i_serial2block.push(int * movable &p_buffer_other, const unsigned n_samps):
                int * movable tmp;
                tmp = move(p_buffer_other);
                p_buffer_other = move(p_from_spdif);    //Swap buffer ownership between tasks
                p_from_spdif = move(tmp);
                do_dsp_flag = 1;                        //We have a fresh buffer to process
            break;

            default:
                if (do_dsp_flag){                        //Do the sample rate conversion
                    port_debug <: 1;                     //debug
                    unsigned n_samps_out;
                    fs_ratio_t fs_ratio = i_fs_ratio.get(nominal_fs_ratio); //Find out how many samples to produce
                    //debug_printf("Using fs_ratio=0x%x\n",fs_ratio);
                    //xscope_int(LEFT, p_to_i2s[0]);

#if 1
                   n_samps_out = asrc_process(p_from_spdif, p_to_i2s, fs_ratio, sASRCCtrl);
                   xscope_int(2, fs_ratio);
#else
                    for(int i=0;i<SRC_N_IN_SAMPLES;i++){
                        p_to_i2s[2*i] = p_from_spdif[2*i];
                        p_to_i2s[2*i + 1] = p_from_spdif[2*i + 1];
                    }
                    n_samps_out = SRC_N_IN_SAMPLES;
#endif
                    i_block2serial.push(p_to_i2s, n_samps_out); //Push result to serialiser output
                    //xscope_int(LEFT, p_to_i2s[0]);
                    do_dsp_flag = 0;                        //Clear flag and wait for next input block
                    port_debug <: 0;                     //debug
                    //debug_printf("n_samps_out=%d\n",n_samps_out);
                }
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
unsafe void block2serial(server block_transfer_if i_block2serial, client serial_transfer_pull_if i_serial_out, server sample_rate_enquiry_if i_output_rate)
{
    int samps_to_i2s[2][OUT_FIFO_SIZE];         //Circular buffers and pointers for output from block2serial
    int * unsafe ptr_base_samps_to_i2s[2];
    int * unsafe ptr_rd_samps_to_i2s[2];
    int * unsafe ptr_wr_samps_to_i2s[2];

    //Double buffer from output from SRC
    int to_i2s[SRC_N_CHANNELS * SRC_N_IN_SAMPLES * SRC_N_OUT_IN_RATIO_MAX];
    int * movable p_to_i2s = to_i2s;            //Movable pointer for swapping with SRC

    unsigned samp_count = 0;                    //Keeps track of number of samples passed through

    timer t_tick;                               //100MHz timer for keeping track of sample ime
    int t_last_count, t_this_count;             //Keeps track of time when querying sample count
    t_tick :> t_last_count;                     //Get time for zero samples counted

    for (unsigned i=0; i<2; i++)                //Initialise FIFOs
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
                int samp[2];
                for (int i=0; i<2; i++) {
                    success &= pull_sample_from_fifo(samp[i], ptr_wr_samps_to_i2s[i], &ptr_rd_samps_to_i2s[i], ptr_base_samps_to_i2s[i], OUT_FIFO_SIZE);
                }
                //xscope_int(1, samp[0]);
                i_serial_out.do_pull(samp, 2);
                if (!success) {
                    for (int i=0; i<2; i++) {   //Init all FIFOs if any of them have under/overflowed
                        debug_printf("-");      //FIFO empty
                        init_fifos(&ptr_wr_samps_to_i2s[i], &ptr_rd_samps_to_i2s[i], ptr_base_samps_to_i2s[i], OUT_FIFO_SIZE);
                    }
                }
                samp_count ++;                  //Keep track of number of samples served to I2S
            break;

            //Request to push block of samples from SRC
            case i_block2serial.push(int * movable &p_buffer_other, const unsigned n_samps):
                int * movable tmp;
                tmp = move(p_buffer_other);         //First swap buffer pointers
                p_buffer_other = move(p_to_i2s);
                p_to_i2s = move(tmp);

                for(int i=0; i<n_samps; i++) {     //Get entire buffer
                    unsigned success = 1;           //Keep track of status of FIFO operations
                    for (int j=0; j<2; j++) {       //Push samples into FIFO
                        int samp = p_to_i2s[2*i + j];
                        success &= push_sample_into_fifo(samp, &ptr_wr_samps_to_i2s[j], ptr_rd_samps_to_i2s[j], ptr_base_samps_to_i2s[j], OUT_FIFO_SIZE);
                    }

                    //xscope_int(LEFT, p_to_i2s[0]);

                    if (!success) {                 //One of the FIFOs has overflowed
                        for (int i=0; i<2; i++){
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

//Shim task to handle setup and streaming of I2S samples from block2serial to the I2S module
[[distributable]]
void i2s_handler(server i2s_callback_if i2s, server serial_transfer_pull_if i_serial_out, client audio_codec_config_if i_codec)
    {
    int samples[2] = {0,0};
    unsigned sample_rate = DEFAULT_FREQ_HZ;
    unsigned mclk_rate = MCLK_FREQUENCY_48;


    while (1) {
        select {
            case i2s.init(i2s_config_t &?i2s_config, tdm_config_t &?tdm_config):
            i2s_config.mclk_bclk_ratio = mclk_rate / (sample_rate << 6);
            i2s_config.mode = I2S_MODE_I2S;
            i_codec.reset(sample_rate, mclk_rate);
            debug_printf("Initializing I2S to %dHz and MCLK to %dHz\n", sample_rate, mclk_rate);
            break;

            //Start of I2S frame
            case i2s.restart_check() -> i2s_restart_t ret:
            ret = I2S_NO_RESTART;
            break;

            //Get samples from ADC
            case i2s.receive(size_t index, int32_t sample):
            break;

            //Send samples to DAC
            case i2s.send(size_t index) -> int32_t sample:
            sample = samples[index];
            if (index == 1){
                i_serial_out.pull_ready();
            }
            break;

            case i_serial_out.do_pull(int new_samples[], const size_t n_samps):
            memcpy(samples, new_samples, n_samps * sizeof(int));
            break;
        }
    }
}

//Type which tells us the current status of the detected sample rate - is it supported?
typedef enum sample_rate_status_t{
    INVALID,
    VALID }
    sample_rate_status_t;

#define SR_TOLERANCE_PPM    1000
#define LOWER_LIMIT(freq) (freq - (((long long) freq * SR_TOLERANCE_PPM) / 1000000))
#define UPPER_LIMIT(freq) (freq + (((long long) freq * SR_TOLERANCE_PPM) / 1000000))

static const unsigned sr_range[6 * 3] = {
        44100, LOWER_LIMIT(44100), UPPER_LIMIT(44100),
        48000, LOWER_LIMIT(48000), UPPER_LIMIT(48000),
        88200, LOWER_LIMIT(88200), UPPER_LIMIT(88200),
        96000, LOWER_LIMIT(96000), UPPER_LIMIT(96000),
        176400, LOWER_LIMIT(176400), UPPER_LIMIT(176400),
        192000, LOWER_LIMIT(192000), UPPER_LIMIT(192000) };

//Helper function for rate_server to check for validity of detected sample rate. Takes sample rate as integer
static sample_rate_status_t detect_frequency(unsigned sample_rate, unsigned &nominal_sample_rate)
{
    sample_rate_status_t result = INVALID;
    nominal_sample_rate = 0;
    for (int i = 0; i < 6 * 3; i+=3) {
        if ((sr_range[i + 1] < sample_rate) && (sample_rate < sr_range[i + 2])){
            nominal_sample_rate = sr_range[i];
            result = VALID;
        }
    }
    return result;
}

//Task that queires the de/serialisers periodically and calculates the number of samples for the SRC
//to produce to keep the output FIFO in block2serial rougly centered. Uses an average of averages to
//filter the sample counts requested from serial2block and block2serial

#define SR_CALC_PERIOD  10000000    //100ms The period over which we count samples to find the rate
#define REPORT_PERIOD   50000000    //500ms How often we print the rates to the screen for debug
#define SR_FRAC_BITS    12          //Number of fractional bits used to store sample rate
                                    //Using 12 gives us 20 bits of integer - up to 1.048MHz SR
//This multiplier is used to work out SR in 20.12 representation. There is enough headroom in a long long calc
//to support a measurement period of 1s at 192KHz with over 2 order of magnitude margin
#define SR_MULTIPLIER   ((1<<SR_FRAC_BITS) * (unsigned long long) XS1_TIMER_HZ)

typedef struct rate_info_t{
    unsigned samp_count;    //Sample count over last period
    unsigned time_ticks;    //Time in ticks for last count
    unsigned current_rate;  //Current average rate in 20.12 fixed point format
    unsigned last_rate;     //Average rate from last period in 20.12 fixed point
    sample_rate_status_t status;    //Lock status
    unsigned nominal_rate;  //Snapped-to nominal rate
} rate_info_t;


[[combinable]]
void rate_server(client sample_rate_enquiry_if i_spdif_rate, client sample_rate_enquiry_if i_i2s_rate,
        server fs_ratio_enquiry_if i_fs_ratio)
{
    rate_info_t spdif_info = {  //Initialise to nominal values for default frequency
            ((DEFAULT_FREQ_HZ * 10000000ULL) / XS1_TIMER_HZ),
            SR_CALC_PERIOD,
            DEFAULT_FREQ_HZ << SR_FRAC_BITS,
            DEFAULT_FREQ_HZ << SR_FRAC_BITS,
            INVALID,
            DEFAULT_FREQ_HZ};

    rate_info_t i2s_info = {    //Initialise to nominal values for default frequency
            ((DEFAULT_FREQ_HZ * 10000000ULL) / XS1_TIMER_HZ),
            SR_CALC_PERIOD,
            DEFAULT_FREQ_HZ << SR_FRAC_BITS,
            DEFAULT_FREQ_HZ << SR_FRAC_BITS,
            INVALID,
            DEFAULT_FREQ_HZ};

    unsigned i2s_buff_level = 0;                //Buffer fill level. Initialise to empty.

    timer t_print;
    int t_print_trigger;

    t_print :> t_print_trigger;
    t_print_trigger += REPORT_PERIOD;

    fs_ratio_t fs_ratio;                        //4.28 fixed point value of how many samples we want SRC to produce
                                                //input fs/output fs. ie. below 1 means inoput faster than output
    fs_ratio_t fs_ratio_old;                    //Last time round value for filtering
    timer t_period_calc;                        //Timer to govern sample count periods
    int t_calc_trigger;                         //Trigger comparison for above

    int sample_time_spdif;                      //Used for passing to get_sample_count method by refrence
    int sample_time_i2s;                        //Used for passing to get_sample_count method by refrence

    t_period_calc :> t_calc_trigger;            //Get current time and set trigger for the future
    t_calc_trigger += SR_CALC_PERIOD;

    while(1){
        select{
            //Serve up latest sample count value when required
            case i_fs_ratio.get(unsigned nominal_fs_ratio) -> fs_ratio_t fs_ratio_ret:
                if (spdif_info.status == VALID && i2s_info.status == VALID){
                    fs_ratio_ret = fs_ratio;    //Pass back calculated value
                }
                else {
                    fs_ratio = nominal_fs_ratio; //Pass back nominal until we have valid rate data
                }
            break;

            //Timeout to trigger calculation of new fs_ratio
            case t_period_calc when timerafter(t_calc_trigger) :> int _:
                t_calc_trigger += SR_CALC_PERIOD;

                unsigned samp_count_spdif = i_spdif_rate.get_sample_count(sample_time_spdif); //get spdif sample count;
                unsigned samp_count_i2s   = i_i2s_rate.get_sample_count(sample_time_i2s);     //And I2S
                i2s_buff_level = i_i2s_rate.get_buffer_level();

                //debug_printf("samp_count_spdif=%d, sample_time_spdif=%d, samp_count_i2s=%d, sample_time_i2s=%d\n", samp_count_spdif, sample_time_spdif, samp_count_i2s, sample_time_i2s);

                if (sample_time_spdif){
                    spdif_info.current_rate = (((unsigned long long)samp_count_spdif * SR_MULTIPLIER) / sample_time_spdif);
                }
                if (sample_time_i2s){
                    i2s_info.current_rate   = (((unsigned long long)samp_count_i2s * SR_MULTIPLIER) / sample_time_i2s);
                }

                //debug_printf("spdif_info.current_rate=%d, i2s_info.current_rate=%d\n", spdif_info.current_rate, i2s_info.current_rate);

                spdif_info.status = detect_frequency(spdif_info.current_rate >> SR_FRAC_BITS, spdif_info.nominal_rate);
                i2s_info.status   = detect_frequency(i2s_info.current_rate >> SR_FRAC_BITS, i2s_info.nominal_rate);


                //Calculate fs_ratio to tell src how many samples to produce
                if (spdif_info.status == VALID && i2s_info.status == VALID) {
                    fs_ratio_old = fs_ratio;        //Save old value
                    fs_ratio = (unsigned) ((i2s_info.current_rate * 0x10000000ULL) / spdif_info.current_rate);
                    int i2s_buffer_level_from_half = (signed)i2s_buff_level - (OUT_FIFO_SIZE / 2);    //Level w.r.t. half full
#define BUFFER_LEVEL_TERM   10000   //How much apply the buffer level feedback term
                    //If buffer is negative, we need to produce more samples so fs_ratio needs to be < 1
                    //If positive, we need to back off a bit so fs_ratio needs to be over unity to get more samples from asrc
                    fs_ratio = (unsigned) (((BUFFER_LEVEL_TERM + i2s_buffer_level_from_half) * (unsigned long long)fs_ratio) / BUFFER_LEVEL_TERM);
                    debug_printf("sp=%d\ti2s=%d\tbuff=%d\tfs_raw=0x%x\tfs_av=0x%x\n", spdif_info.current_rate, i2s_info.current_rate, i2s_buffer_level_from_half, fs_ratio, fs_ratio_old);
#define OLD_VAL_WEIGHTING   100
                    //Apply simple low pass filter
                    fs_ratio = (unsigned) (((unsigned long long)(fs_ratio_old) * OLD_VAL_WEIGHTING + (unsigned long long)(fs_ratio) ) /
                            (1 + OLD_VAL_WEIGHTING));
                }

                if (spdif_info.status == VALID) spdif_info.last_rate = spdif_info.current_rate;
                if (i2s_info.status == VALID)   i2s_info.last_rate = i2s_info.current_rate;
            break;

            case t_print when timerafter(t_print_trigger) :> int _:
                t_print_trigger += REPORT_PERIOD;
                //Calculate sample rates in Hz for human readability
#if 0
                debug_printf("spdif rate ave=%d, valid=%d, i2s rate=%d, valid=%d, i2s_buff=%d, fs_ratio=0x%x\n",
                        spdif_info.current_rate >> SR_FRAC_BITS, spdif_info.status,
                        i2s_info.current_rate >> SR_FRAC_BITS, i2s_info.status,
                        i2s_buff_level, fs_ratio);
#endif
            break;


        }
    }
}

