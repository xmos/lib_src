//Standard includes
#include <xs1.h>
#include <platform.h>
#include <string.h>

//Supporting libraries
#include <src.h>
#include <spdif.h>
#include <i2s.h>
#include <i2c.h>
#include <gpio.h>

//Application specific includes
#include "main.h"
#include "block_serial.h"
#include "cs4384_5368.h"
#include "app_config.h"     //General settings

//Debug includes
#include <debug_print.h> //Enabled by -DDEBUG_PRINT_ENABLE=1 in Makefile
#include <xscope.h>


/* These port assignments all correspond to XU216 multichannel audio board 2V0
   The port assignments can be changed for a different port map.
*/
#define AUDIO_TILE                      0
in buffered port:32  ports_i2s_adc[4]    = on tile[AUDIO_TILE]: {XS1_PORT_1I,
                                                     XS1_PORT_1J,
                                                     XS1_PORT_1K,
                                                     XS1_PORT_1L};
port port_i2s_mclk                       = on tile[AUDIO_TILE]: XS1_PORT_1F;
clock clk_mclk                           = on tile[AUDIO_TILE]: XS1_CLKBLK_2;
out buffered port:32 port_i2s_bclk       = on tile[AUDIO_TILE]: XS1_PORT_1H;
out buffered port:32 port_i2s_wclk       = on tile[AUDIO_TILE]: XS1_PORT_1G;
clock clk_i2s                            = on tile[AUDIO_TILE]: XS1_CLKBLK_1;
out buffered port:32 ports_i2s_dac[4]    = on tile[AUDIO_TILE]: {XS1_PORT_1M,
                                                     XS1_PORT_1N,
                                                     XS1_PORT_1O,
                                                     XS1_PORT_1P};


#define SPDIF_TILE                      1
port port_spdif_rx                       = on tile[SPDIF_TILE]: XS1_PORT_1O;
clock clk_spdif_rx                       = on tile[SPDIF_TILE]: XS1_CLKBLK_1;

out port p_leds_col                      = on tile[SPDIF_TILE]: XS1_PORT_4C;     //4x4 LED matrix
out port p_leds_row                      = on tile[SPDIF_TILE]: XS1_PORT_4D;

port port_i2c                            = on tile[AUDIO_TILE]: XS1_PORT_4A;     //I2C for CODEC configuration

port port_audio_config                   = on tile[AUDIO_TILE]: XS1_PORT_8C;
/* Bit map for XS1_PORT_8C
 * 0 DSD_MODE
 * 1 DAC_RST_N
 * 2 USB_SEL0
 * 3 USB_SEL1
 * 4 VBUS_OUT_EN
 * 5 PLL_SELECT
 * 6 ADC_RST_N
 * 7 MCLK_FSEL
 */

port p_buttons                           = on tile[AUDIO_TILE]: XS1_PORT_4D;     //Buttons and switch
char pin_map[1]                          = {0};                                  //Port map for buttons GPIO task. We are just interested in bit 0

out port port_debug_tile_1               = on tile[SPDIF_TILE]: XS1_PORT_1N;     //MIDI OUT. A good test point to probe..
out port port_debug_tile_0               = on tile[AUDIO_TILE]: XS1_PORT_1D;     //SPDIF COAX TX. A good test point to probe..


[[combinable]] void spdif_handler(streaming chanend c_spdif_rx, client serial_transfer_push_if i_serial_in);
void asrc(server block_transfer_if i_serial2block, client block_transfer_if i_block2serial, client fs_ratio_enquiry_if i_fs_ratio);
[[distributable]] void i2s_handler(server i2s_callback_if i2s, client serial_transfer_pull_if i_serial_out, client audio_codec_config_if i_codec, server buttons_if i_buttons);
[[combinable]]void rate_server(client sample_rate_enquiry_if i_spdif_rate, client sample_rate_enquiry_if i_output_rate, server fs_ratio_enquiry_if i_fs_ratio[ASRC_N_INSTANCES], client led_matrix_if i_leds);
[[combinable]]void led_driver(server led_matrix_if i_leds, out port p_leds_row, out port p_leds_col);
[[combinable]]void button_listener(client buttons_if i_buttons, client input_gpio_if i_button_port);

int main(void){
    serial_transfer_push_if i_serial_in;
    block_transfer_if i_serial2block[ASRC_N_INSTANCES];
    block_transfer_if i_block2serial[ASRC_N_INSTANCES];
    serial_transfer_pull_if i_serial_out;
    sample_rate_enquiry_if i_sr_input, i_sr_output;
    fs_ratio_enquiry_if i_fs_ratio[ASRC_N_INSTANCES];
    interface audio_codec_config_if i_codec;
    interface i2c_master_if i_i2c[1];
    interface output_gpio_if i_gpio[8];    //See mapping of bits 0..7 above in port_audio_config
    interface i2s_callback_if i_i2s;
    led_matrix_if i_leds;
    buttons_if i_buttons;
    input_gpio_if i_button_gpio[1];
    streaming chan c_spdif_rx;

    par{
        on tile[SPDIF_TILE]: spdif_rx(c_spdif_rx, port_spdif_rx, clk_spdif_rx, DEFAULT_FREQ_HZ_SPDIF);
        on tile[AUDIO_TILE]: [[combine, ordered]] par{
            rate_server(i_sr_input, i_sr_output, i_fs_ratio, i_leds);
            spdif_handler(c_spdif_rx, i_serial_in);
            button_listener(i_buttons, i_button_gpio[0]);
            input_gpio_with_events(i_button_gpio, 1, p_buttons, pin_map);
        }

        on tile[AUDIO_TILE]: serial2block(i_serial_in, i_serial2block, i_sr_input);
        on tile[AUDIO_TILE]: par (int i=0; i<ASRC_N_INSTANCES; i++) asrc(i_serial2block[i], i_block2serial[i], i_fs_ratio[i]);
        on tile[AUDIO_TILE]: unsafe { par{[[distribute]] block2serial(i_block2serial, i_serial_out, i_sr_output);}}

        on tile[AUDIO_TILE]: audio_codec_cs4384_cs5368(i_codec, i_i2c[0], CODEC_IS_I2S_SLAVE, i_gpio[0], i_gpio[1], i_gpio[6], i_gpio[7]);
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
        on tile[AUDIO_TILE]: [[distribute]] i2s_handler(i_i2s, i_serial_out, i_codec, i_buttons);
        on tile[SPDIF_TILE]: led_driver(i_leds, p_leds_row, p_leds_col);

    }
    return 0;
}


//Shim task to handle setup and streaming of SPDIF samples from the streaming channel to the interface of serial2block
[[combinable]] //Run on same core as other tasks if possible
void spdif_handler(streaming chanend c_spdif_rx, client serial_transfer_push_if i_serial_in)
{
    unsigned index;                             //Channel index
    signed long sample;                         //Sample received from SPDIF

    delay_microseconds(1000);                   //Bug 17263 workaround (race condition in distributable task init)
    while (1) {
        select {
            case spdif_receive_sample(c_spdif_rx, sample, index):
                i_serial_in.push(sample, index);   //Push them into serial to block
            break;
        }
    }
}


//Helper function for converting sample to fs index value
static fs_code_t samp_rate_to_code(unsigned samp_rate){
    unsigned samp_code = 0xdead;
    switch (samp_rate){
    case 44100:
        samp_code = FS_CODE_44100;
        break;
    case 48000:
        samp_code = FS_CODE_48000;
        break;
    case 88200:
        samp_code = FS_CODE_88200;
        break;
    case 96000:
        samp_code = FS_CODE_96000;
        break;
    case 176400:
        samp_code = FS_CODE_176400;
        break;
    case 192000:
        samp_code = FS_CODE_192000;
        break;
    }
    return samp_code;
}

//The ASRC processing task - has it's own logical core to reserve processing MHz
unsafe{
    void asrc(server block_transfer_if i_serial2block, client block_transfer_if i_block2serial, client fs_ratio_enquiry_if i_fs_ratio)
    {
        int input_dbl_buf[2][ASRC_CHANNELS_PER_INSTANCE * ASRC_N_IN_SAMPLES];  //Double buffers for to block/serial tasks
        unsigned buff_idx = 0;
        int * unsafe asrc_input = input_dbl_buf[0]; //pointer for ASRC input buffer
        int * unsafe p_out_fifo;                    //C-style pointer for output FIFO

        p_out_fifo = i_block2serial.push(0);        //Get pointer to initial write buffer

        set_core_high_priority_on();                //Give me guarranteed 1/5 of the processor clock i.e. 100MHz

        fs_code_t in_fs_code = samp_rate_to_code(DEFAULT_FREQ_HZ_SPDIF);  //Sample rate code 0..5
        fs_code_t out_fs_code = samp_rate_to_code(DEFAULT_FREQ_HZ_I2S);

        ASRCState_t     sASRCState[ASRC_CHANNELS_PER_INSTANCE]; //ASRC state machine state
        int             iASRCStack[ASRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * ASRC_N_IN_SAMPLES]; //Buffer between filter stages
        ASRCCtrl_t      sASRCCtrl[ASRC_CHANNELS_PER_INSTANCE];  //Control structure
        iASRCADFIRCoefs_t SiASRCADFIRCoefs;                     //Adaptive filter coefficients

        for(int ui = 0; ui < ASRC_CHANNELS_PER_INSTANCE; ui++)
        unsafe {
            //Set state, stack and coefs into ctrl structure
            sASRCCtrl[ui].psState                   = &sASRCState[ui];
            sASRCCtrl[ui].piStack                   = iASRCStack[ui];
            sASRCCtrl[ui].piADCoefs                 = SiASRCADFIRCoefs.iASRCADFIRCoefs;
        }

        //Initialise ASRC
        unsigned nominal_fs_ratio = asrc_init(in_fs_code, out_fs_code, sASRCCtrl, ASRC_CHANNELS_PER_INSTANCE, ASRC_N_IN_SAMPLES, ASRC_DITHER_SETTING);

        unsigned do_dsp_flag = 0;                   //Flag to indiciate we are ready to process. Minimises blocking on push case below
        timer t_do_dsp;                             //Used to trigger do_dsp event

        while(1){
            select{
                case i_serial2block.push(const unsigned n_samps) -> int * unsafe new_buff_ptr:
                    asrc_input = input_dbl_buf[buff_idx];   //Grab address of freshly filled buffer
                    do_dsp_flag = 1;                        //We have a fresh buffer to process
                    buff_idx ^= 1;                          //Flip double buffer for filling
                    new_buff_ptr = input_dbl_buf[buff_idx]; //Return pointer for serial2block to fill
                break;

                case i_fs_ratio.new_sr_notify():            //Notification from SR manager that we need to initialise ASRC
                    in_fs_code = samp_rate_to_code(i_fs_ratio.get_in_fs());         //Get the new SRs
                    out_fs_code = samp_rate_to_code(i_fs_ratio.get_out_fs());
                    debug_printf("New rate in SRC in=%d, out=%d\n", in_fs_code, out_fs_code);
                    nominal_fs_ratio = asrc_init(in_fs_code, out_fs_code, sASRCCtrl, ASRC_CHANNELS_PER_INSTANCE, ASRC_N_IN_SAMPLES, ASRC_DITHER_SETTING);
                break;

                case do_dsp_flag => t_do_dsp :> void:      //Do the sample rate conversion
                    //port_debug <: 1;                     //debug
                    unsigned n_samps_out;
                    fs_ratio_t fs_ratio = i_fs_ratio.get_ratio(nominal_fs_ratio); //Find out how many samples to produce

                    //Run the ASRC and pass pointer of output to block2serial
                    n_samps_out = asrc_process((int *)asrc_input, (int *)p_out_fifo, fs_ratio, sASRCCtrl);
                    p_out_fifo = i_block2serial.push(n_samps_out);   //Get pointer to next write buffer


                    do_dsp_flag = 0;                        //Clear flag and wait for next input block
                    //port_debug <: 0;                     //debug
                    break;
            }
        }//While 1
    }//asrc
} //unsafe

#define MUTE_MS_AFTER_SR_CHANGE   250    //250ms. Avoids incorrect rate playing momentarily while new rate is detected

//Shim task to handle setup and streaming of I2S samples from block2serial to the I2S module
[[distributable]]
#pragma unsafe arrays   //Performance optimisation of i2s_handler task
void i2s_handler(server i2s_callback_if i2s, client serial_transfer_pull_if i_serial_out, client audio_codec_config_if i_codec, server buttons_if i_buttons)
    {
    unsigned sample_rate = DEFAULT_FREQ_HZ_I2S;
    unsigned mclk_rate;
    unsigned restart_status = I2S_NO_RESTART;
    unsigned mute_counter; //Non zero indicates mute. Initialised on I2S init SR change

    while (1) {
        select {
            case i2s.init(i2s_config_t &?i2s_config, tdm_config_t &?tdm_config):
                if (!(sample_rate % 48000)) mclk_rate = MCLK_FREQUENCY_48;  //Initialise MCLK to appropriate multiple of sample_rate
                else mclk_rate  = MCLK_FREQUENCY_44;
                i2s_config.mclk_bclk_ratio = mclk_rate / (sample_rate << 6);
                i2s_config.mode = I2S_MODE_I2S;
                i_codec.reset(sample_rate, mclk_rate);
                debug_printf("Initializing I2S to %dHz and MCLK to %dHz\n", sample_rate, mclk_rate);
                restart_status = I2S_NO_RESTART;
                mute_counter = (sample_rate * MUTE_MS_AFTER_SR_CHANGE) / 1000; //Initialise to a number of milliseconds
            break;

            //Start of I2S frame
            case i2s.restart_check() -> i2s_restart_t ret:
                ret = restart_status;
            break;

            //Get samples from ADC
            case i2s.receive(size_t index, int32_t sample):
            break;

            //Send samples to DAC
            case i2s.send(size_t index) -> int32_t sample:
                if (mute_counter){
                    sample = 0;
                    mute_counter --;
                }
                else{
                    sample = i_serial_out.pull(index);
                }
            break;


            //Cycle through sample rates of I2S on button press
            case i_buttons.pressed():
                switch (sample_rate) {
                case 44100:
                    sample_rate = 48000;
                    break;
                case 48000:
                    sample_rate = 88200;
                    break;
                case 88200:
                    sample_rate = 96000;
                    break;
                case 96000:
                    sample_rate = 44100;
                    break;
                }
                restart_status = I2S_RESTART;
            break;

        }
    }
}


#define SR_TOLERANCE_PPM    1000    //How far the detect_frequency function will allow before declaring invalid in p.p.m.
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


#define SR_CALC_PERIOD  10000000    //100ms The period over which we count samples to find the rate
                                    //Because we timestamp at 10ns resolution, we get 10000000/10 = 20bits of precision
#define REPORT_PERIOD   100100000   //1001ms How often we print the rates to the screen for debug. Chosen to not clash with above
#define SR_FRAC_BITS    12          //Number of fractional bits used to store sample rate
                                    //Using 12 gives us 20 bits of integer - up to 1.048MHz SR before overflow
//Below is the multiplier is used to work out SR in 20.12 representation. There is enough headroom in a long long calc
//to support a measurement period of 1s at 192KHz with over 2 order of magnitude margin against overflow
#define SR_MULTIPLIER   ((1<<SR_FRAC_BITS) * (unsigned long long) XS1_TIMER_HZ)
#define SETTLE_CYCLES   3           //Number of measurement periods to skip after SR change (SR change blocks spdif momentarily so corrupts SR calc)

typedef struct rate_info_t{
    unsigned samp_count;            //Sample count over last period
    unsigned time_ticks;            //Time in ticks for last count
    unsigned current_rate;          //Current average rate in 20.12 fixed point format
    sample_rate_status_t status;    //Lock status
    unsigned nominal_rate;          //Snapped-to nominal rate as unsigned integer
} rate_info_t;

//Task that queires the de/serialisers periodically and calculates the number of samples for the SRC
//to produce to keep the output FIFO in block2serial rougly centered. Uses the timestamped sample counts
//requested from serial2block and block2serial and FIFO level as P and I terms
[[combinable]]
#pragma unsafe arrays   //Performance optimisation
void rate_server(client sample_rate_enquiry_if i_spdif_rate, client sample_rate_enquiry_if i_i2s_rate,
        server fs_ratio_enquiry_if i_fs_ratio[ASRC_N_INSTANCES], client led_matrix_if i_leds)
{
    rate_info_t spdif_info = {  //Initialise to nominal values for default frequency
            ((DEFAULT_FREQ_HZ_SPDIF * 10000000ULL) / XS1_TIMER_HZ),
            SR_CALC_PERIOD,
            DEFAULT_FREQ_HZ_SPDIF << SR_FRAC_BITS,
            INVALID,
            DEFAULT_FREQ_HZ_SPDIF};

    rate_info_t i2s_info = {    //Initialise to nominal values for default frequency
            ((DEFAULT_FREQ_HZ_I2S * 10000000ULL) / XS1_TIMER_HZ),
            SR_CALC_PERIOD,
            DEFAULT_FREQ_HZ_I2S << SR_FRAC_BITS,
            INVALID,
            DEFAULT_FREQ_HZ_I2S};

    unsigned i2s_buff_level = 0;                //Buffer fill level. Initialise to empty.
    unsigned i2s_buff_size = OUT_FIFO_SIZE;
    unsigned skip_validity = 0;                 //Do SR validity check - need this to allow SR to settle after SR change

    timer t_print;                              //Debug print timers
    int t_print_trigger;

    t_print :> t_print_trigger;
    t_print_trigger += REPORT_PERIOD;

    fs_ratio_t fs_ratio;                        //4.28 fixed point value of how many samples we want SRC to produce
                                                //input fs/output fs. ie. below 1 means inoput faster than output
    fs_ratio_t fs_ratio_old;                    //Last time round value for filtering
    fs_ratio_t fs_ratio_nominal;                //Nominal fs ratio reported by SRC
    timer t_period_calc;                        //Timer to govern sample count periods
    int t_calc_trigger;                         //Trigger comparison for above

    int sample_time_spdif;                      //Used for passing to get_sample_count method by refrence
    int sample_time_i2s;                        //Used for passing to get_sample_count method by refrence

    t_period_calc :> t_calc_trigger;            //Get current time and set trigger for the future
    t_calc_trigger += SR_CALC_PERIOD;

    while(1){
        select{
            //Serve up latest sample count value when required. Note selects over array of interfaces
            case i_fs_ratio[int if_index].get_ratio(unsigned nominal_fs_ratio) -> fs_ratio_t fs_ratio_ret:
                fs_ratio_nominal =  nominal_fs_ratio;   //Allow use outside of this case
                if ((spdif_info.status == VALID) && (i2s_info.status == VALID)){
                    fs_ratio_ret = fs_ratio;    //Pass back calculated value
                }
                else {
                    fs_ratio = nominal_fs_ratio; //Pass back nominal until we have valid rate data
                }

            break;

            //Serve up the input sample rate
            case i_fs_ratio[int if_index].get_in_fs(void) -> unsigned fs:
                fs = spdif_info.nominal_rate;
            break;

            //Serve up the output sample rate
            case i_fs_ratio[int if_index].get_out_fs(void) -> unsigned fs:
                fs = i2s_info.nominal_rate;
            break;

            //Timeout to trigger calculation of new fs_ratio
            case t_period_calc when timerafter(t_calc_trigger) :> int _:
                t_calc_trigger += SR_CALC_PERIOD;

                unsigned samp_count_spdif = i_spdif_rate.get_sample_count(sample_time_spdif); //get spdif sample count;
                unsigned samp_count_i2s   = i_i2s_rate.get_sample_count(sample_time_i2s);     //And I2S
                {i2s_buff_size, i2s_buff_level} = i_i2s_rate.get_buffer_level();

                if (sample_time_spdif){ //If time is non-zero - avoids divide by zero if no input
                    spdif_info.current_rate = (((unsigned long long)samp_count_spdif * SR_MULTIPLIER) / sample_time_spdif);
                }
                else spdif_info.current_rate = 0;
                if (sample_time_i2s){
                    i2s_info.current_rate   = (((unsigned long long)samp_count_i2s * SR_MULTIPLIER) / sample_time_i2s);
                }
                else i2s_info.current_rate = 0;

                //Find lock status of input/output sample rates
                sample_rate_status_t spdif_status_new = detect_frequency(spdif_info.current_rate >> SR_FRAC_BITS, spdif_info.nominal_rate);
                sample_rate_status_t i2s_status_new  = detect_frequency(i2s_info.current_rate >> SR_FRAC_BITS, i2s_info.nominal_rate);

                //If either has changed from invalid to valid, send message to SRC to initialise
                if ((spdif_status_new == VALID && i2s_status_new == VALID) && ((spdif_info.status == INVALID || i2s_info.status == INVALID)) && !skip_validity){
                    for(int i = 0; i < ASRC_N_INSTANCES; i++){
                        i_fs_ratio[i].new_sr_notify();
                    }
                    skip_validity =  SETTLE_CYCLES;  //Don't check on validity for a few cycles as will be corrupted by SR change and SRC init
                    fs_ratio = (unsigned) ((spdif_info.nominal_rate * 0x10000000ULL) / i2s_info.nominal_rate); //Initialise rate to nominal
                }

                if (skip_validity) skip_validity--;

                //Update current sample rate status flags for input and output
                spdif_info.status = spdif_status_new;
                i2s_info.status   = i2s_status_new;

#define BUFFER_LEVEL_TERM   20000   //How much to apply the buffer level feedback term (effectively 1/I term)
#define OLD_VAL_WEIGHTING   5      //Simple low pass filter. Set proportion of old value to carry over


                //Calculate fs_ratio to tell asrc how many samples to produce in 4.28 fixed point format
                int i2s_buffer_level_from_half = (signed)i2s_buff_level - (i2s_buff_size / 2);    //Level w.r.t. half full
                if (spdif_info.status == VALID && i2s_info.status == VALID) {
                    fs_ratio_old = fs_ratio;        //Save old value
                    fs_ratio = (unsigned) ((spdif_info.current_rate * 0x10000000ULL) / i2s_info.current_rate);

                    //If buffer is negative, we need to produce more samples so fs_ratio needs to be < 1
                    //If positive, we need to back off a bit so fs_ratio needs to be over unity to get more samples from asrc
                    fs_ratio = (unsigned) (((BUFFER_LEVEL_TERM + i2s_buffer_level_from_half) * (unsigned long long)fs_ratio) / BUFFER_LEVEL_TERM);
                    //debug_printf("sp=%d\ti2s=%d\tbuff=%d\tfs_raw=0x%x\tfs_av=0x%x\n", spdif_info.current_rate, i2s_info.current_rate, i2s_buffer_level_from_half, fs_ratio, fs_ratio_old);
                    //Apply simple low pass filter
                    fs_ratio = (unsigned) (((unsigned long long)(fs_ratio_old) * OLD_VAL_WEIGHTING + (unsigned long long)(fs_ratio) ) /
                            (1 + OLD_VAL_WEIGHTING));
                }

                //Set Sample rate LEDs
                unsigned spdif_fs_code = samp_rate_to_code(spdif_info.nominal_rate) + 1;
                unsigned i2s_fs_code = samp_rate_to_code(i2s_info.nominal_rate) + 1;
                if (spdif_info.status == INVALID) spdif_fs_code = 0;
                if (i2s_info.status  == INVALID) i2s_fs_code = 0;

                for (int i = 0; i< 4; i++){
                    if (spdif_fs_code > i) i_leds.set(3, i, 1);
                    else i_leds.set(3, i, 0);
                    if (i2s_fs_code > i) i_leds.set(0, i, 1);
                    else i_leds.set(0, i, 0);
                }

#define THRESH_0    6   //First led comes on when non-zero. Second when > THRESH_0
#define THRESH_1    12  //Third led comes on when > THRESH_1

                //Show buffer level in column 3
                if (i2s_buffer_level_from_half > 0) i_leds.set(2, 2, 1);
                else i_leds.set(2, 2, 0);
                if (i2s_buffer_level_from_half > THRESH_0) i_leds.set(2, 3, 1);
                else i_leds.set(2, 3, 0);
                if (i2s_buffer_level_from_half > THRESH_1) i_leds.set(1, 3, 1);
                else i_leds.set(1, 3, 0);

                if (i2s_buffer_level_from_half < 0) i_leds.set(2, 1, 1);
                else i_leds.set(2, 1, 0);
                if (i2s_buffer_level_from_half < -THRESH_0) i_leds.set(2, 0, 1);
                else i_leds.set(2, 0, 0);
                if (i2s_buffer_level_from_half < -THRESH_1) i_leds.set(1, 0, 1);
                else i_leds.set(1, 0, 0);

            break;

            case t_print when timerafter(t_print_trigger) :> int _:
                t_print_trigger += REPORT_PERIOD;
                //Calculate sample rates in Hz for human readability
#if 1
                debug_printf("spdif rate ave=%d, valid=%d, i2s rate=%d, valid=%d, i2s_buff=%d, fs_ratio=0x%x, nom_fs=0x%x\n",
                        spdif_info.current_rate >> SR_FRAC_BITS, spdif_info.status,
                        i2s_info.current_rate >> SR_FRAC_BITS, i2s_info.status,
                        (signed)i2s_buff_level - (OUT_FIFO_SIZE / 2), fs_ratio, fs_ratio_nominal);
#endif
            break;
        }
    }
}


//Task that drives the multiplexed 4x4 display on the xCORE-200 MC AUDIO board. Very low performance requirements so can be combined
#define LED_SCAN_TIME   200100   //2ms - How long each column is displayed. Any more than this and you start to see flicker
[[combinable]]void led_driver(server led_matrix_if i_leds, out port p_leds_col, out port p_leds_row){
    unsigned col_frame_buffer[4] = {0xf, 0xf, 0xf, 0xf};  //4 x 4 bitmap frame buffer scanning from left to right
                                                          //Active low drive hence initialise to 0b1111
    unsigned col_idx = 0;                                 //Index for above
    unsigned col_sel = 0x1;                               //Column select 0x8 -> 0x4 -> 0x2 -> 0x1
    timer t_scan;
    int scan_time_trigger;

    t_scan :> scan_time_trigger;                          //Get current time

    while(1){
        select{
            //Scan through 4 columns and output bitmap for each
            case t_scan when timerafter(scan_time_trigger + LED_SCAN_TIME) :> scan_time_trigger:
                p_leds_col <: col_sel;
                p_leds_row <: col_frame_buffer[col_idx];
                col_idx = (col_idx + 1) & 0x3;
                col_sel = col_sel << 1;
                if(col_sel > 0x8) col_sel = 0x1;
            break;

            //Sets a pixel at col, row (origin bottom left) to 0 or on
            case i_leds.set(unsigned col, unsigned row, unsigned val):
                row = row & 0x3;  //Prevent out of bounds access
                col = col & 0x3;
                if (val) {  //Need to clear corresponding bit (active low)
                    col_frame_buffer[col] &= ~(0x8 >> row);
                }
                else {      ///Set bit to turn off (active low)
                    col_frame_buffer[col] |= (0x8 >> row);
                }
            break;
        }
    }
}

#define DEBOUNCE_PERIOD        2000000  //20ms
#define DEBOUNCE_SAMPLES       5        //Sample 5 times in this period to ensure we have a true value
#define BUTTON_PRESSED_VAL     0
#define BUTTON_NOT_PRESSED_VAL 1

//Button listener task. Applies a debounce function by checking several times for the same value
[[combinable]]void button_listener(client buttons_if i_buttons, client input_gpio_if i_button_port){
    timer t_debounce;
    int t_debounce_time;
    unsigned debounce_counter = 0;  //Counts debounce sequence has started
    unsigned button_release = 0;    //Flag showing we hav just had a press and aree waiting to start again

    i_button_port.event_when_pins_eq(BUTTON_PRESSED_VAL); //setup button event

    while(1){
        select{
            case i_button_port.event(): //The button has reached the expected value
                if (button_release){    //If being released
                    i_button_port.event_when_pins_eq(BUTTON_PRESSED_VAL); //Setup event for being pressed
                    button_release = 0;
                    break;
                }
                debounce_counter = DEBOUNCE_SAMPLES;    //Kick off debounce sequence
                t_debounce :> t_debounce_time;          //Get current time
            break;

            case debounce_counter => t_debounce when timerafter(t_debounce_time + (DEBOUNCE_PERIOD/DEBOUNCE_SAMPLES)) :> t_debounce_time:
                unsigned port_val = i_button_port.input();
                if (port_val != BUTTON_PRESSED_VAL) {       //Read port n times. If not what we want, start over
                    debounce_counter = 0;
                    i_button_port.event_when_pins_eq(BUTTON_PRESSED_VAL);
                    break;
                }
                debounce_counter--;
                if (debounce_counter == 0){                 //We have seen n samples the same, so button is pressed
                    button_release = 1;
                    i_button_port.event_when_pins_eq(BUTTON_NOT_PRESSED_VAL);   //setup event for button released
                    i_buttons.pressed();                                        //Send button pressed message
                }
            break;
        }
    }
}

