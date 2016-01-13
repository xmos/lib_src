// General includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ASRC includes
#include "src.h"

#define			INPUT_FILE_CHANNEL_0_DEFAULT					"../input_sines/s1k_0dB_192.dat"
#define         INPUT_FILE_CHANNEL_1_DEFAULT                    "../input_sines/s1k_0dB_176.dat"
#define			OUTPUT_FILE_CHANNEL_0_DEFAULT					"output_ch0.dat"
#define         OUTPUT_FILE_CHANNEL_1_DEFAULT                   "output_ch1.dat"

#define			INPUT_FS_DEFAULT								1 //See asrc.h, 0=44.1 - 5=192
#define			OUTPUT_FS_DEFAULT								3

#define         SAMP_IN_LIMIT                                   128
#define         SAMP_IN_CHANGE_SR                               SAMP_IN_LIMIT + 1

const int sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000, 352800, 384000};

void dsp_slave(chanend c_dsp)
{
// ASRC instances variables
// ------------------------
// State, Stack, Coefs and Control structures (one for each channel)
    ASRCState_t     sASRCState[ASRC_CHANNELS_PER_CORE]; //ASRC state machine state
    int             iASRCStack[ASRC_CHANNELS_PER_CORE][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES]; //Buffer between filter stages
    ASRCCtrl_t      sASRCCtrl[ASRC_CHANNELS_PER_CORE];  //Control structure
    iASRCADFIRCoefs_t SiASRCADFIRCoefs;                 //Adaptive filter coefficients

    unsigned int    sr_in_out = 99999; //Invalid SR code to force initialisation on first run
    unsigned int    sr_in_out_new;

    int             in_buff[ASRC_N_IN_SAMPLES * ASRC_CHANNELS_PER_CORE];
    int             out_buff[ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX * ASRC_CHANNELS_PER_CORE];

    timer t;
    unsigned t1=0,t2=0,t_dsp=0;

    unsigned int    n_samps_out = 0;	//number of samples produced by last call to ASRC
    unsigned int    n_samps_in_tot = 0; //Total number of input samples through ASRC
    unsigned int    FsRatio = ASRC_NOMINAL_FS_SCALE; //Deviation between in Fs and out Fs

    for(int ui = 0; ui < ASRC_CHANNELS_PER_CORE; ui++)
    unsafe {
        // Set state, stack and coefs into ctrl structure
        sASRCCtrl[ui].psState                   = &sASRCState[ui];
        sASRCCtrl[ui].piStack                   = iASRCStack[ui];
        sASRCCtrl[ui].piADCoefs                 = SiASRCADFIRCoefs.iASRCADFIRCoefs;
        printf("sASRCCtrl[%d].piADCoefs0x%p\n",ui, sASRCCtrl[ui].piADCoefs);
    }
    
        

    memset(out_buff, 0, ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX * ASRC_CHANNELS_PER_CORE * sizeof(int));

    while(1){
        t :> t2;
        t_dsp = (t2 - t1);
	int sample_time = 100000000 / sample_rates[sr_in_out >> 16];
#if 1
        if (n_samps_in_tot) printf("proc time chan=%d, In sample period=%d, Thread util=%d%%, Tot samp in count=%d\n",
           (t_dsp / (ASRC_CHANNELS_PER_CORE * ASRC_N_IN_SAMPLES)), sample_time, (100 * (t_dsp / ( ASRC_N_IN_SAMPLES))) / sample_time, n_samps_in_tot);
#endif
        c_dsp :> sr_in_out_new;
        c_dsp :> FsRatio;

        for(unsigned i=0; i<ASRC_N_IN_SAMPLES; i++) {
            unsigned tmp;
            for (unsigned j=0; j<ASRC_CHANNELS_PER_CORE; j++) {
            	c_dsp :> tmp;
            	in_buff[i*ASRC_CHANNELS_PER_CORE + j] = tmp;
                //printf("n_samp=%d chan=%d, tmp=%d\n", i, j, tmp);
            }
        }

        n_samps_in_tot += ASRC_N_IN_SAMPLES;
        
	    c_dsp <: n_samps_out;

        for(unsigned uj = 0; uj < n_samps_out; uj++)
        {
            unsigned tmp;
            for (unsigned j=0; j<ASRC_CHANNELS_PER_CORE; j++) {
            	tmp = out_buff[uj*ASRC_CHANNELS_PER_CORE + j];
                c_dsp <: tmp;
                //printf("n_samp=%d chan=%d, tmp=%d\n", i, j, tmp);
            }
        }

        if (sr_in_out_new != sr_in_out) {
            // Set input/output sampling rate codes. See ASRCFs_t in ASRC.h (ie 0=44.1 5=192 etc.)
            unsigned InFs                     = (sr_in_out_new >> 16) & 0xffff;
            unsigned OutFs                    = sr_in_out_new & 0xffff;

            FsRatio = asrc_init(InFs, OutFs, sASRCCtrl);
            sr_in_out = sr_in_out_new;
            printf("DSP init Initial FsRatio=%d, SR in=%d, SR out=%d\n", FsRatio, InFs, OutFs);
        }
        t:> t1;
        n_samps_out = asrc_process(in_buff, out_buff, FsRatio, sASRCCtrl);
    }
}

void dsp_mgr(chanend c_dsp[]){

    FILE            * movable InFileDat[2];
    FILE            * movable OutFileDat[2];


    unsigned count_in = 0, count_out = 0;
    unsigned iEndOfFile  = 0;

    unsigned int    FsRatio; //Deviation between in Fs and out Fs

    unsigned int    sr_in_out = INPUT_FS_DEFAULT << 16 | OUTPUT_FS_DEFAULT ; //(192/176)

    //int             out_buff[ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX];


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
        //quick hack to get it going - TODO - do proper calc
        FsRatio = (((sample_rates[sr_in_out >> 16] * 16384) / sample_rates[sr_in_out & 0xffff]) * 16384);

        for (unsigned j=0; j<ASRC_N_CORES; j++) {
            c_dsp[j] <: sr_in_out;
            c_dsp[j] <: FsRatio;
        }

        for(unsigned i = 0; i < ASRC_N_IN_SAMPLES * ASRC_CHANNELS_PER_CORE; i++) {
            int samp;
            for (unsigned j=0; j<ASRC_N_CORES; j++) {
                unsigned file_index = (i * ASRC_N_CORES + j) % ASRC_N_CHANNELS;
                if ((fscanf(InFileDat[file_index], "%i\n", &samp) == EOF) || (count_in >= SAMP_IN_LIMIT))  {
                        iEndOfFile = 1;     // We are at the end of the file
                        printf("EOF\n");
                }
                c_dsp[j] <: samp;
            }            
	    }
		count_in += ASRC_N_IN_SAMPLES; 


        unsigned n_samps;
        for (unsigned j=0; j<ASRC_N_CORES; j++) {
            c_dsp[j] :> n_samps;
        }

        //printf("n_samps from dsp=%d\n", n_samps);
        for(unsigned i = 0; i < n_samps * ASRC_CHANNELS_PER_CORE; i++) {
            int samp;
            for (unsigned j=0; j<ASRC_N_CORES; j++) {
                unsigned file_index = (i * ASRC_N_CORES + j) % ASRC_N_CHANNELS;
                c_dsp[j] :> samp;
                if(fprintf(OutFileDat[file_index], "%i\n", samp) < 0)
                    printf("Error while writing to output file\n");
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
    chan c_dsp[ASRC_N_CORES];
    par
    {
        par (unsigned i=0; i<ASRC_N_CORES; i++) dsp_slave(c_dsp[i]);
        dsp_mgr(c_dsp);

    }
    return 0;
}



