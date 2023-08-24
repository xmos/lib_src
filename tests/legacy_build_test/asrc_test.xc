// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// General includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// ASRC includes
#include "src.h"

#define     ASRC_N_CHANNELS                  2  //Total number of audio channels to be processed by SRC (minimum 1)
#define     ASRC_N_INSTANCES                 1  //Number of instances (each usuall run a logical core) used to process audio (minimum 1)
#define     ASRC_CHANNELS_PER_INSTANCE       (ASRC_N_CHANNELS/ASRC_N_INSTANCES)
                                                //Calcualted number of audio channels processed by each core
#define     ASRC_N_IN_SAMPLES                4  //Number of samples per channel in each block passed into SRC each call
                                                //Must be a power of 2 and minimum value is 4 (due to two /2 decimation stages)
#define     ASRC_N_OUT_IN_RATIO_MAX          5  //Max ratio between samples out:in per processing step (44.1->192 is worst case)
#define     ASRC_DITHER_SETTING              OFF

#include "src_mrhf_asrc_checks.h"               //Do some checking on the defines above


//Input and output files
char * unsafe pzInFileName[ASRC_N_CHANNELS] = {null};
char * unsafe pzOutFileName[ASRC_N_CHANNELS] = {null};

//Number of samples to process
int uiNTotalInSamples = -1;

//Input and output frequency indicies. Initialise to invalid. See src_mrhf_asrc.h ASRCFs_t for these codes. 0=44.1 - 5=192
int uiInFs = -1;
int uiOutFs = -1;

//Global deviation ratio from nominal sample rate in/out ratio. Initialise to invalid.
float fFsRatioDeviation = -1.0000;

const int sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000};

void dsp_slave(chanend c_dsp)
{
// ASRC instances variables
// ------------------------
// State, Stack, Coefs and Control structures (one for each channel)
    asrc_state_t     asrc_state[ASRC_CHANNELS_PER_INSTANCE]; //ASRC state machine state
    int              asrc_stack[ASRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * ASRC_N_IN_SAMPLES]; //Buffer between filter stages
    asrc_ctrl_t      asrc_ctrl[ASRC_CHANNELS_PER_INSTANCE];  //Control structure
    asrc_adfir_coefs_t asrc_adfir_coefs;                 //Adaptive filter coefficients

    unsigned int    sr_in_out = 99999; //Invalid SR code to force initialisation on first run
    unsigned int    sr_in_out_new;

    int             in_buff[ASRC_N_IN_SAMPLES * ASRC_CHANNELS_PER_INSTANCE];
    int             out_buff[ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX * ASRC_CHANNELS_PER_INSTANCE];

    timer t;
    unsigned t1=0,t2=0,t_dsp=0;

    unsigned int    n_samps_out = 0;    //number of samples produced by last call to ASRC
    unsigned int    n_samps_in_tot = 0; //Total number of input samples through ASRC
    unsigned int    FsRatio = ASRC_NOMINAL_FS_SCALE; //Deviation between in Fs and out Fs

    for(int ui = 0; ui < ASRC_CHANNELS_PER_INSTANCE; ui++)
    unsafe {
        // Set state, stack and coefs into ctrl structure
        asrc_ctrl[ui].psState                   = &asrc_state[ui];
        asrc_ctrl[ui].piStack                   = asrc_stack[ui];
        asrc_ctrl[ui].piADCoefs                 = asrc_adfir_coefs.iASRCADFIRCoefs;
    }
    
/*
     // Update Fs Ratio
    for(int i = 0; i < ASRC_N_CHANNELS; i++)
    {
    // Make Fs Ratio deviate
    asrc_ctrl[i].uiFsRatio        = (unsigned int)(asrc_ctrl[i].uiFsRatio * fFsRatioDeviation);
        if(ASRC_update_fs_ratio(&asrc_ctrl[i]) != ASRC_NO_ERROR)
        {
              printf("Error updating ASRC fs ratio\n");
        }
    }
*/

    memset(out_buff, 0, ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX * ASRC_CHANNELS_PER_INSTANCE * sizeof(int));

    while(1){
        t :> t2;  //Grab time at processing finished (t1 set at end of this loop)
        t_dsp = (t2 - t1);
    int sample_time = 100000000 / sample_rates[sr_in_out >> 16];
        if (n_samps_in_tot) printf("Process time per chan ticks=%d, Tot samp in count=%d\n",
            (t_dsp / (ASRC_CHANNELS_PER_INSTANCE * ASRC_N_IN_SAMPLES)),
            n_samps_in_tot);
        c_dsp :> sr_in_out_new;
        c_dsp :> FsRatio;

        for(unsigned i=0; i<ASRC_N_IN_SAMPLES; i++) {
            unsigned tmp;
            for (unsigned j=0; j<ASRC_CHANNELS_PER_INSTANCE; j++) {
                c_dsp :> tmp;
                in_buff[i*ASRC_CHANNELS_PER_INSTANCE + j] = tmp;
                //printf("n_samp=%d chan=%d, tmp=%d\n", i, j, tmp);
            }
        }

        n_samps_in_tot += ASRC_N_IN_SAMPLES;
        
        c_dsp <: n_samps_out;       //Send number of samples to receive

        for(unsigned uj = 0; uj < n_samps_out; uj++)
        {
            unsigned tmp;
            for (unsigned j=0; j<ASRC_CHANNELS_PER_INSTANCE; j++) {
                tmp = out_buff[uj*ASRC_CHANNELS_PER_INSTANCE + j];
                c_dsp <: tmp;
            }
        }

        if (sr_in_out_new != sr_in_out) {
            // Set input/output sampling rate codes. See ASRCFs_t in src_mrhf_asrc.h (ie 0=44.1 5=192 etc.)
            unsigned InFs                     = (sr_in_out_new >> 16) & 0xffff;
            unsigned OutFs                    = sr_in_out_new & 0xffff;

            unsigned nominal_FsRatio = asrc_init(InFs, OutFs, asrc_ctrl, ASRC_CHANNELS_PER_INSTANCE, ASRC_N_IN_SAMPLES, ASRC_DITHER_SETTING);
            
            sr_in_out = sr_in_out_new;
            printf("DSP init Initial nominal_FsRatio=%d, SR in=%d, SR out=%d\n", nominal_FsRatio, InFs, OutFs);
        }
        t:> t1;
        n_samps_out = asrc_process(in_buff, out_buff, FsRatio, asrc_ctrl);
    }
}

void dsp_mgr(chanend c_dsp[], float fFsRatioDeviation){

    FILE            * movable InFileDat[ASRC_N_CHANNELS];
    FILE            * movable OutFileDat[ASRC_N_CHANNELS];


    unsigned count_in = 0, count_out = 0;
    unsigned iEndOfFile  = 0;
    unsigned int    sr_in_out = uiInFs << 16 | uiOutFs ; //Input fs in upper 16bits and Output fs in lower 16bits
    unsigned FsRatio = (unsigned) (((unsigned long long)sample_rates[uiInFs] * (unsigned long long)(1<<28)) / (unsigned long long)sample_rates[uiOutFs]);
    
    FsRatio =  (unsigned int)((float)FsRatio * fFsRatioDeviation); //Ensure is precisely the same as golden value complete with trucation due to 32b float
    printf("Adjusted FsRatio dsp_mgr = %d, 0x%x\n", FsRatio, FsRatio);

    

    for (int i=0; i<ASRC_N_CHANNELS; i++)
    unsafe
    {
        if ((InFileDat[i] = fopen((char *)pzInFileName[i], "rt")) == NULL)
        {
            printf("Error while opening input file, %s", pzInFileName[i]);
            exit(1);
        }
        printf("Simulator opening input file, %s\n",  pzInFileName[i]);
        if ((OutFileDat[i] = fopen((char *)pzOutFileName[i], "wt")) == NULL)
        {
            printf("Error while opening output file, %s", pzOutFileName[i]);
        }
        printf("Simulator opening output file, %s\n",  pzOutFileName[i]);
    }


    while(!iEndOfFile)
    {        
        for (unsigned j=0; j<ASRC_N_INSTANCES; j++) {
            c_dsp[j] <: sr_in_out;
            c_dsp[j] <: FsRatio;
        }

        for(unsigned i = 0; i < ASRC_N_IN_SAMPLES * ASRC_CHANNELS_PER_INSTANCE; i++) {
            int samp;
            for (unsigned j=0; j<ASRC_N_INSTANCES; j++) {
                unsigned file_index = (i * ASRC_N_INSTANCES + j) % ASRC_N_CHANNELS;
                if ((fscanf(InFileDat[file_index], "%i\n", &samp) == EOF) || (count_in >= uiNTotalInSamples))  \
                {
                    iEndOfFile = 1;     //We are at the end of the file
                    printf("EOF\n");
                }
                c_dsp[j] <: samp;       //Send the sample
            }            
        }
        count_in += ASRC_N_IN_SAMPLES; 


        unsigned n_samps;
        for (unsigned j=0; j<ASRC_N_INSTANCES; j++) 
        {
            c_dsp[j] :> n_samps;  //Get number of samps to receive
        }

        for(unsigned i = 0; i < n_samps * ASRC_CHANNELS_PER_INSTANCE; i++) 
        {
            int samp;
            for (unsigned j=0; j<ASRC_N_INSTANCES; j++) 
            {
                unsigned file_index = (i * ASRC_N_INSTANCES + j) % ASRC_N_CHANNELS;
                c_dsp[j] :> samp; //Get samples
                if(fprintf(OutFileDat[file_index], "%i\n", samp) < 0)
                    printf("Error while writing to output file\n");
            }
        }

        count_out += n_samps;

        //Force a sample rate change at a given count
        //if (count_in >= SAMP_IN_CHANGE_SR) sr_in_out = INPUT_FS_DEFAULT << 16 | INPUT_FS_DEFAULT;
    }

    printf("DSP manager done - %d output samples produced\n", count_out);

    for (int i=0; i<ASRC_N_CHANNELS; i++)
    unsafe
    {
        fclose(move(InFileDat[i]));
        fclose(move(OutFileDat[i]));
    }

    exit(0);
}

void ShowUsage()
{
    puts(
        "Usage: xsim --args asrc_simple.xe <args>\n\n"
        "         -i     Q1.31 line separated format input file names (eg. -i in_l.dat in_r.dat)\n\n"
        "         -o     Q1.31 line separated format output file names (eg. -o out_l.dat out_r.dat)\n\n"
        "         -h     Show this usage message and abort\n\n"
        "         -f     Input sample rate (44100 - 192000)\n\n"
        "         -g     Output sample rate (44100 - 192000)\n\n"
        "         -n     Number of input samples (all channels) to process\n\n"
        );
    
    exit(0);
}

//Helper function for converting SR to index value
int samp_rate_to_code(int samp_rate){
    int samp_code = -1;
    switch (samp_rate){
    case 44100:
        samp_code = 0;
        break;
    case 48000:
        samp_code = 1;
        break;
    case 88200:
        samp_code = 2;
        break;
    case 96000:
        samp_code = 3;
        break;
    case 176400:
        samp_code = 4;
        break;
    case 192000:
        samp_code = 5;
        break;
    default:
        break;
    }
    return samp_code;
}

void ParseCmdLine(char *input, char * unsafe * argv, int ui)
{
  switch (*input)
  {
  
    case 'h':
    case 'H':
      ShowUsage();
      exit(0);
      break;
    
    case 'i':
    case 'I':
      for (int i=0; i<ASRC_N_CHANNELS; i++)
      {
        if (*input == '-'){
          printf("ERROR: expecting %d input file names and only found %d\n", ASRC_N_CHANNELS, i + 1);
          exit(1);
        }
        unsafe{
          pzInFileName[i] = argv[ui + 1 + i];
        }
      }
      break;

    case 'f':
    case 'F':
      unsafe{
        uiInFs = (unsigned int)(atoi((char *)argv[ui + 1]));
      }
      uiInFs = samp_rate_to_code(uiInFs);
      if((uiInFs < ASRC_FS_MIN) || (uiInFs > ASRC_FS_MAX))
      {
        printf("ERROR: invalid frequency index %d\n", uiInFs);
        exit(1);
      }
      break;

    case 'g':
    case 'G':
      uiOutFs = (unsigned int)(atoi((char *)argv[ui + 1]));
      uiOutFs = samp_rate_to_code(uiOutFs);      
      if((uiOutFs < ASRC_FS_MIN) || (uiOutFs > ASRC_FS_MAX))
      {
        printf("ERROR: invalid frequency index %d\n", uiOutFs);
        exit(1);
      }
      break;

    case 'e':
    case 'E':
      fFsRatioDeviation = (float)(atof((char *)argv[ui + 1]));
      //Note no check. This is done at run-time.
      break;

    case 'n':
    case 'N':
      uiNTotalInSamples = (unsigned int)(atoi((char *)argv[ui + 1]));
      break;

    case 'o':
    case 'O':
      for (int i=0; i<ASRC_N_CHANNELS; i++)
      {
        if (*input == '-'){
          printf("ERROR: expecting %d output file names and only found %d\n", ASRC_N_CHANNELS, i + 1);
          exit(1);
        }
        unsafe{
          pzOutFileName[i] = argv[ui + 1 + i];
        }
      }
      break;

      break;

    default:
      ShowUsage();
      break;
  }
}

int main(int argc, char * unsafe argv[argc]) 
{
    int ui;

    printf("Running ASRC test simulation \n");
    if (argc <= 1 ) ShowUsage();

    //Parse command line arguments 
    for (ui = 1; ui < (unsigned int)argc; ui++)
    unsafe{
        if (*(argv[ui]) == '-')
        {
            ParseCmdLine((char *)argv[ui] + 1, argv, ui);
        }
    }



    //Test for valid arguments
    if (uiNTotalInSamples == -1)
    {
        printf("ERROR: number of input samples not set\n");
        exit(1);
    }
    
    if (uiInFs == -1)
    {
        printf("ERROR: input sample rate not set\n");
        exit(1);
    }

    if (uiOutFs == -1)
    {
        printf("ERROR: output sample rate not set\n");
        exit(1);
    }

    if (fFsRatioDeviation == -1)
    {
        printf("ERROR: sample ratio nominal ration deviation not set\n");
        exit(1);
    }

    chan c_dsp[ASRC_N_INSTANCES];
    par
    {
        par (unsigned i=0; i<ASRC_N_INSTANCES; i++) dsp_slave(c_dsp[i]);
        dsp_mgr(c_dsp, fFsRatioDeviation);

    }
    return 0;
}
