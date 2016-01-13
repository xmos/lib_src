// General includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>


// SSRC includes
#include "src.h"

#define			INPUT_FILE_CHANNEL_0_DEFAULT					"../input_sines/s1k_0dB_192.dat"
#define         INPUT_FILE_CHANNEL_1_DEFAULT                    "../input_sines/s1k_0dB_176.dat"
#define			OUTPUT_FILE_CHANNEL_0_DEFAULT					"output_ch0.dat"
#define         OUTPUT_FILE_CHANNEL_1_DEFAULT                   "output_ch1.dat"


#define			INPUT_FS_DEFAULT								5 //See ssrc.h SSRCFs_t for these codes. 0=44.1 - 5=192
#define			OUTPUT_FS_DEFAULT								1

#define         SAMP_IN_LIMIT                                   512
#define         SAMP_IN_CHANGE_SR                               SAMP_IN_LIMIT + 1

void dsp_slave(chanend c_dsp)
{
    SSRCState_t     sSSRCState;                                              //State of SSRC module
    int             iSSRCStack[SSRC_STACK_LENGTH_MULT * SSRC_N_IN_SAMPLES];  //Buffers between processing stages
    SSRCCtrl_t      sSSRCCtrl;                                               //SSRC Control structure

    // Set state, stack and coefs into ctrl structure
    unsafe{
      sSSRCCtrl.psState                   = &sSSRCState;
      sSSRCCtrl.piStack                   = iSSRCStack;
    }

    unsigned int    sr_in_out = 99999; //Invalid SR code to force initialisation on first run
    unsigned int    sr_in_out_new;

    const int sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000, 352800, 384000};

    int             in_buff[SSRC_N_IN_SAMPLES * SSRC_CHANNELS_PER_CORE];
    int             out_buff[SSRC_N_IN_SAMPLES * SSRC_N_OUT_IN_RATIO_MAX * SSRC_CHANNELS_PER_CORE];

    timer t;
    unsigned t1=0,t2=0,t_dsp=0;

    unsigned int    n_samps_out = 0;	//number of samples produced by last call to SSRC
    unsigned int    n_samps_in_tot = 0; //Total number of input samples through SSRC

    memset(out_buff, 0, SSRC_N_IN_SAMPLES * SSRC_N_OUT_IN_RATIO_MAX * SSRC_CHANNELS_PER_CORE * 4);

    while(1){
        t :> t2;
        t_dsp = (t2 - t1);
	int sample_time = 100000000 / sample_rates[sr_in_out >> 16];
        if (n_samps_in_tot) printf("Process time per chan=%d, Input sample period=%d, Thread utilisation=%d%%, Tot samp in count=%d\n", 
	 	(t_dsp / (SSRC_CHANNELS_PER_CORE * SSRC_N_IN_SAMPLES)), sample_time, (100 * (t_dsp / ( SSRC_N_IN_SAMPLES))) / sample_time, n_samps_in_tot);
        c_dsp :> sr_in_out_new;

        for(unsigned i=0; i<SSRC_N_IN_SAMPLES; i++) {
            unsigned tmp;
            for (unsigned j=0; j<SSRC_CHANNELS_PER_CORE; j++) {
            	c_dsp :> tmp;
            	in_buff[i*SSRC_CHANNELS_PER_CORE + j] = tmp;
                //printf("n_samp=%d chan=%d, tmp=%d\n", i, j, tmp);
            }
        }

        n_samps_in_tot += SSRC_N_IN_SAMPLES;
        
	    c_dsp <: n_samps_out;

        for(unsigned uj = 0; uj < n_samps_out; uj++)
        {
            unsigned tmp;
            for (unsigned j=0; j<SSRC_CHANNELS_PER_CORE; j++) {
            	tmp = out_buff[uj*SSRC_CHANNELS_PER_CORE + j];
                c_dsp <: tmp;
                //printf("n_samp=%d chan=%d, tmp=%d\n", i, j, tmp);
            }
        }

        if (sr_in_out_new != sr_in_out) {
            // Set input/output sampling rate codes. See SSRCFs_t in SSRC.h (ie 0=44.1 5=192)
            unsigned InFs                     = (sr_in_out_new >> 16) & 0xffff;
            unsigned OutFs                    = sr_in_out_new & 0xffff;

            unsafe{
              ssrc_init(InFs, OutFs, &sSSRCCtrl);
            }
            sr_in_out = sr_in_out_new;
            printf("DSP SR in=%d, SR out=%d\n", InFs, OutFs);
        }
        t:> t1;
        unsafe {
          n_samps_out = ssrc_process(in_buff, out_buff, &sSSRCCtrl);
        }
    }
}

void dsp_mgr(chanend c_dsp[]){

    FILE            * movable InFileDat[2];
    FILE            * movable OutFileDat[2];


    unsigned count_in = 0, count_out = 0;

    unsigned iEndOfFile  = 0;

    unsigned int    sr_in_out = INPUT_FS_DEFAULT << 16 | OUTPUT_FS_DEFAULT ; //(192/176)

    //int             out_buff[SSRC_N_IN_SAMPLES * SSRC_N_OUT_IN_RATIO_MAX];


    if ((InFileDat[0] = fopen(INPUT_FILE_CHANNEL_0_DEFAULT, "rt")) == NULL)
        {
            printf("Error while opening input file, %s", INPUT_FILE_CHANNEL_0_DEFAULT);
        }

    if ((InFileDat[1] = fopen(INPUT_FILE_CHANNEL_1_DEFAULT, "rt")) == NULL)
        {
            printf("Error while opening input file, %s", INPUT_FILE_CHANNEL_1_DEFAULT);
        }
    
    if ((OutFileDat[0] = fopen(OUTPUT_FILE_CHANNEL_0_DEFAULT, "wt")) == NULL)
        {
            printf("Error while opening output file, %s", OUTPUT_FILE_CHANNEL_0_DEFAULT);
        }
    if ((OutFileDat[1] = fopen(OUTPUT_FILE_CHANNEL_1_DEFAULT, "wt")) == NULL)
        {
            printf("Error while opening output file, %s", OUTPUT_FILE_CHANNEL_1_DEFAULT);
        }

    while(!iEndOfFile)
    {
        for (unsigned j=0; j<SSRC_N_CORES; j++) {
            c_dsp[j] <: sr_in_out;
        }

        for(unsigned i = 0; i < SSRC_N_IN_SAMPLES * SSRC_CHANNELS_PER_CORE; i++) {
            int samp;
            for (unsigned j=0; j<SSRC_N_CORES; j++) {
                unsigned file_index = (i * SSRC_N_CORES + j) % SSRC_N_CHANNELS;
                if ((fscanf(InFileDat[file_index], "%i\n", &samp) == EOF) || (count_in >= SAMP_IN_LIMIT))  {
                        iEndOfFile = 1;     // We are at the end of the file
                        printf("EOF\n");
                }
                c_dsp[j] <: samp;
            }            
	    }
		count_in += SSRC_N_IN_SAMPLES; 


        unsigned n_samps;
        for (unsigned j=0; j<SSRC_N_CORES; j++) {
            c_dsp[j] :> n_samps;
        }

        //printf("n_samps from dsp=%d\n", n_samps);
        for(unsigned i = 0; i < n_samps * SSRC_CHANNELS_PER_CORE; i++) {
            int samp;
            for (unsigned j=0; j<SSRC_N_CORES; j++) {
                unsigned file_index = (i * SSRC_N_CORES + j) % SSRC_N_CHANNELS;
                c_dsp[j] :> samp;
                if(fprintf(OutFileDat[file_index], "%i\n", samp) < 0)
                    printf("Error while writing to output file\n");
                //xscope_int(j, samp);
	        }
        }

        count_out += n_samps;


        //printf("Count in=%d, count out=%d\n", count_in, count_out);
        if (count_in >= SAMP_IN_CHANGE_SR) sr_in_out = INPUT_FS_DEFAULT << 16 | INPUT_FS_DEFAULT;
    }

    printf("DSP manager done\n");
    if (fclose(move(InFileDat[0])))
    {
        printf("Error while closing input file, %s", INPUT_FILE_CHANNEL_0_DEFAULT);
    }
    if (fclose(move(InFileDat[1])))
    {
        printf("Error while closing input file, %s", INPUT_FILE_CHANNEL_1_DEFAULT);
    }

    if (fclose(move(OutFileDat[0])))
    {
        printf("Error while closing output file, %s", OUTPUT_FILE_CHANNEL_0_DEFAULT);
    }
    if (fclose(move(OutFileDat[1])))
    {
        printf("Error while closing output file, %s", OUTPUT_FILE_CHANNEL_1_DEFAULT);
    }


    _Exit(0);
}

int main(void)
{
    chan c_dsp[SSRC_N_CORES];
    par
    {
        par (unsigned i=0; i<SSRC_N_CORES; i++) dsp_slave(c_dsp[i]);
        dsp_mgr(c_dsp);

    }
    return 0;
}
