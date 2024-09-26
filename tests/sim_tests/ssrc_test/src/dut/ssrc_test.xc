// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// General includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


// SSRC includes
#include "src.h"

//General SSRC configuration defines
#define     SSRC_N_CHANNELS                  2  //Total number of audio channels to be processed by SRC (minimum 1)

#define     SSRC_N_INSTANCES                 1  //Number of instances (each usuall run a logical core) used to process audio (minimum 1)
#define     SSRC_CHANNELS_PER_INSTANCE       (SSRC_N_CHANNELS/SSRC_N_INSTANCES)
                                                //Calcualted number of audio channels processed by each core
#define     SSRC_N_IN_SAMPLES                4  //Number of samples per channel in each block passed into SRC each call
                                                //Must be a power of 2 and minimum value is 4 (due to two /2 decimation stages)
#define     SSRC_N_OUT_IN_RATIO_MAX          5  //Max ratio between samples out:in per processing step (44.1->192 is worst case)
#define     SSRC_DITHER_SETTING              OFF//Enables or disables quantisation of output with dithering to 24b

#include "src_mrhf_ssrc_checks.h"               //Do some checking on the defines above

//Input and output files
char * unsafe pzInFileName[SSRC_N_CHANNELS] = {null};
char * unsafe pzOutFileName[SSRC_N_CHANNELS] = {null};

//Number of samples to process
int uiNTotalInSamples = -1;

//Input and output frequency indicies. See src_mrhf_ssrc.h SSRCFs_t for these codes. 0=44.1 - 5=192
int uiInFs = -1;
int uiOutFs = -1;

void dsp_slave(chanend c_dsp)
{
    ssrc_state_t     ssrc_state[SSRC_CHANNELS_PER_INSTANCE];                  //State of SSRC module
    int              ssrc_stack[SSRC_CHANNELS_PER_INSTANCE][SSRC_STACK_LENGTH_MULT * SSRC_N_IN_SAMPLES];  //Buffers between processing stages
    ssrc_ctrl_t      ssrc_ctrl[SSRC_CHANNELS_PER_INSTANCE];                   //SSRC Control structure

    // Set state, stack and coefs into ctrl structures
    for(int ui = 0; ui < SSRC_CHANNELS_PER_INSTANCE; ui++)
    {
        unsafe{
            ssrc_ctrl[ui].psState                   = &ssrc_state[ui];
            ssrc_ctrl[ui].piStack                   = ssrc_stack[ui];
        }
    }

    unsigned int    sr_in_out = 99999; //Invalid SR code to force initialisation on first run
    unsigned int    sr_in_out_new;

    const int sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000};

    int             in_buff[SSRC_N_IN_SAMPLES * SSRC_CHANNELS_PER_INSTANCE];
    int             out_buff[SSRC_N_IN_SAMPLES * SSRC_N_OUT_IN_RATIO_MAX * SSRC_CHANNELS_PER_INSTANCE];

    timer t;
    unsigned t1=0,t2=0,t_dsp=0;

    unsigned int    n_samps_out = 0;  //number of samples produced by last call to SSRC
    unsigned int    n_samps_in_tot = 0; //Total number of input samples through SSRC

    memset(out_buff, 0, SSRC_N_IN_SAMPLES * SSRC_N_OUT_IN_RATIO_MAX * SSRC_CHANNELS_PER_INSTANCE * 4);

    while(1){
        t :> t2;  //Grab time at processing finished (t1 set at end of this loop)
        t_dsp = (t2 - t1);
        int sample_time = 100000000 / sample_rates[sr_in_out >> 16];
        if (n_samps_in_tot) printf("Process time per chan ticks=%d, Tot samp in count=%d\n",
            (t_dsp / (SSRC_CHANNELS_PER_INSTANCE * SSRC_N_IN_SAMPLES)),
            n_samps_in_tot);
        c_dsp :> sr_in_out_new;

        for(unsigned i=0; i<SSRC_N_IN_SAMPLES; i++) {
            unsigned tmp;
            for (unsigned j=0; j<SSRC_CHANNELS_PER_INSTANCE; j++) {
                c_dsp :> tmp;
                in_buff[i*SSRC_CHANNELS_PER_INSTANCE + j] = tmp;
            }
        }

        n_samps_in_tot += SSRC_N_IN_SAMPLES;

        c_dsp <: n_samps_out;       //Send number of samples to receive

        for(unsigned uj = 0; uj < n_samps_out; uj++)
        {
            unsigned tmp;
            for (unsigned j=0; j<SSRC_CHANNELS_PER_INSTANCE; j++) {
                tmp = out_buff[uj*SSRC_CHANNELS_PER_INSTANCE + j];
                c_dsp <: tmp;
            }
        }

        if (sr_in_out_new != sr_in_out) {
            // Set input/output sampling rate codes. See SSRCFs_t in src_mrhf_ssrc.h (ie 0=44.1 5=192)
            unsigned InFs                     = (sr_in_out_new >> 16) & 0xffff;
            unsigned OutFs                    = sr_in_out_new & 0xffff;

            unsafe{
                ssrc_init(InFs, OutFs, ssrc_ctrl, SSRC_CHANNELS_PER_INSTANCE, SSRC_N_IN_SAMPLES, SSRC_DITHER_SETTING);
            }
            sr_in_out = sr_in_out_new;
            printf("SSRC sample rate in=%d, out=%d\n", sample_rates[InFs], sample_rates[OutFs]);
        }
        t:> t1; //Grab time at start of processing
        unsafe {
            n_samps_out = ssrc_process(in_buff, out_buff, ssrc_ctrl);
        }
    }
}

void dsp_mgr(chanend c_dsp[]){

    FILE            * movable InFileDat[SSRC_N_CHANNELS];
    FILE            * movable OutFileDat[SSRC_N_CHANNELS];


    unsigned count_in = 0, count_out = 0;
    unsigned iEndOfFile  = 0;
    unsigned int    sr_in_out = uiInFs << 16 | uiOutFs ; //Input fs in upper 16bits and Output fs in lower 16bits


    for (int i=0; i<SSRC_N_CHANNELS; i++)
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
        for (unsigned j=0; j<SSRC_N_INSTANCES; j++)
        {
            c_dsp[j] <: sr_in_out;  //Send in/out sample rate
        }

        for(unsigned i = 0; i < SSRC_N_IN_SAMPLES * SSRC_CHANNELS_PER_INSTANCE; i++)
        {
            int samp;
            for (unsigned j=0; j<SSRC_N_INSTANCES; j++) {
                unsigned file_index = (i * SSRC_N_INSTANCES + j) % SSRC_N_CHANNELS;
                if ((fscanf(InFileDat[file_index], "%i\n", &samp) == EOF) || (count_in >= uiNTotalInSamples))  \
                {
                    iEndOfFile = 1;     //We are at the end of the file
                    printf("EOF\n");
                }
                c_dsp[j] <: samp;       //Send the sample
            }
        }
        count_in += SSRC_N_IN_SAMPLES;


        unsigned n_samps;
        for (unsigned j=0; j<SSRC_N_INSTANCES; j++)
        {
            c_dsp[j] :> n_samps;  //Get number of samps to receive
        }

        for(unsigned i = 0; i < n_samps * SSRC_CHANNELS_PER_INSTANCE; i++)
        {
            int samp;
            for (unsigned j=0; j<SSRC_N_INSTANCES; j++)
            {
                unsigned file_index = (i * SSRC_N_INSTANCES + j) % SSRC_N_CHANNELS;
                c_dsp[j] :> samp; //Get samples
                if(fprintf(OutFileDat[file_index], "%i\n", samp) < 0)
                    printf("Error while writing to output file\n");
                //xscope_int(j, samp);
          }
        }

        count_out += n_samps;

        //Force a sample rate change at a given count
        //if (count_in >= SAMP_IN_CHANGE_SR) sr_in_out = INPUT_FS_DEFAULT << 16 | INPUT_FS_DEFAULT;
    }

    printf("DSP manager done - %d output samples produced\n", count_out);

    for (int i=0; i<SSRC_N_CHANNELS; i++)
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
        "Usage: xsim ssrc_simple.xe <args>\n\n"
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
      for (int i=0; i<SSRC_N_CHANNELS; i++)
      {
        if (*input == '-'){
          printf("ERROR: expecting %d input file names and only found %d\n", SSRC_N_CHANNELS, i + 1);
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
      if((uiInFs < SSRC_FS_MIN) || (uiInFs > SSRC_FS_MAX))
      {
        printf("ERROR: invalid frequency index %d\n", uiInFs);
        exit(1);
      }
      break;

    case 'g':
    case 'G':
      uiOutFs = (unsigned int)(atoi((char *)argv[ui + 1]));
      uiOutFs = samp_rate_to_code(uiOutFs);
      if((uiOutFs < SSRC_FS_MIN) || (uiOutFs > SSRC_FS_MAX))
      {
        printf("ERROR: invalid frequency index %d\n", uiOutFs);
        exit(1);
      }
      break;


    case 'n':
    case 'N':
      uiNTotalInSamples = (unsigned int)(atoi((char *)argv[ui + 1]));
      break;

    case 'o':
    case 'O':
      for (int i=0; i<SSRC_N_CHANNELS; i++)
      {
        if (*input == '-'){
          printf("ERROR: expecting %d output file names and only found %d\n", SSRC_N_CHANNELS, i + 1);
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

int main(int argc, char * unsafe argv[])
{
    int ui;

    printf("Running SSRC test simulation\n");

    if (argc == 1) ShowUsage();

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
        printf("ERROR: number input samples not set\n");
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

    chan c_dsp[SSRC_N_INSTANCES];
    par
    {
        par (unsigned i=0; i<SSRC_N_INSTANCES; i++) dsp_slave(c_dsp[i]);
        dsp_mgr(c_dsp);

    }
    return 0;
}
