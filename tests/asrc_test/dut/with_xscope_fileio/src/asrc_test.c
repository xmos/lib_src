#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <xcore/hwtimer.h>
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

//Input and output files
char* pzInFileName[ASRC_N_CHANNELS] = {NULL};
char* pzOutFileName[ASRC_N_CHANNELS] = {NULL};

//Number of samples to process
int uiNTotalInSamples = -1;

//Input and output frequency indicies. Initialise to invalid. See src_mrhf_asrc.h ASRCFs_t for these codes. 0=44.1 - 5=192
int uiInFs = -1;
int uiOutFs = -1;

extern double fFsRatioDeviation;


void ShowUsage()
{
    puts(
        "Write the args in a single line in a file named args.txt\n\n"
        "         -i     Q1.31 line separated format input binary file names (eg. -i in_l.bin in_r.bin)\n\n"
        "         -o     Q1.31 line separated format output binary file names (eg. -o out_l.bin out_r.bin)\n\n"
        "         -h     Show this usage message and abort\n\n"
        "         -f     Input sample rate (44100 - 192000)\n\n"
        "         -g     Output sample rate (44100 - 192000)\n\n"
        "         -e     Frequency deviation\n\n"
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

void parse_cmd_line(char *arg_part_1)
{

    if((strcmp(arg_part_1, "-h") == 0) || (strcmp(arg_part_1, "-H") == 0))
    {
      ShowUsage();
      exit(0);
    }
    else if((strcmp(arg_part_1, "-i") == 0) || (strcmp(arg_part_1, "-I") == 0))
    {
      for (int i=0; i<ASRC_N_CHANNELS; i++)
      {
        char *arg_part_2 = strtok(NULL, " ");
        if(arg_part_2[0] == '-')
        {
          printf("ERROR: expecting %d input file names and only found %d\n", ASRC_N_CHANNELS, i + 1);
          exit(1);
        }

        pzInFileName[i] = malloc(strlen(arg_part_2) + 10);
        strcpy(pzInFileName[i], arg_part_2);

        printf("Input file name = %s\n", pzInFileName[i]);
      }
    }
    else if((strcmp(arg_part_1, "-o") == 0) || (strcmp(arg_part_1, "-O") == 0))
    {
      for (int i=0; i<ASRC_N_CHANNELS; i++)
      {
        char *arg_part_2 = strtok(NULL, " ");
        if(arg_part_2[0] == '-')
        {
          printf("ERROR: expecting %d input file names and only found %d\n", ASRC_N_CHANNELS, i + 1);
          exit(1);
        }

        pzOutFileName[i] = malloc(strlen(arg_part_2) + 10);
        strcpy(pzOutFileName[i], arg_part_2);
        printf("Output file name = %s\n", pzOutFileName[i]);
      }

    }
    else if((strcmp(arg_part_1, "-f") == 0) || (strcmp(arg_part_1, "-F") == 0))
    {
      char *arg_part_2 = strtok(NULL, " ");
      uiInFs = (unsigned int)(atoi((char *)arg_part_2));
      uiInFs = samp_rate_to_code(uiInFs);
      if((uiInFs < ASRC_FS_MIN) || (uiInFs > ASRC_FS_MAX))
      {
        printf("ERROR: invalid frequency index %d\n", uiInFs);
        exit(1);
      }
      printf("uiInFs = %d\n", uiInFs);
    }
    else if((strcmp(arg_part_1, "-g") == 0) || (strcmp(arg_part_1, "-G") == 0))
    {
      char *arg_part_2 = strtok(NULL, " ");
      uiOutFs = (unsigned int)(atoi((char *)arg_part_2));
      uiOutFs = samp_rate_to_code(uiOutFs);
      if((uiOutFs < ASRC_FS_MIN) || (uiOutFs > ASRC_FS_MAX))
      {
        printf("ERROR: invalid frequency index %d\n", uiOutFs);
        exit(1);
      }
      printf("uiOutFs = %d\n", uiOutFs);
    }
    else if((strcmp(arg_part_1, "-e") == 0) || (strcmp(arg_part_1, "-E") == 0))
    {
      char *arg_part_2 = strtok(NULL, " ");
      fFsRatioDeviation = (double)(atof((char *)arg_part_2));
      printf("fFsRatioDeviation = %f\n", fFsRatioDeviation);
    }
    else if((strcmp(arg_part_1, "-n") == 0) || (strcmp(arg_part_1, "-N") == 0))
    {
      char *arg_part_2 = strtok(NULL, " ");
      uiNTotalInSamples = (unsigned int)(atoi((char *)arg_part_2));
      printf("uiNTotalInSamples = %d\n", uiNTotalInSamples);
    }
    else
    {
      printf("ERROR: Invalid argument %s\n", arg_part_1);
      assert(0);
    }
}

void parse_args()
{
  printf("Running ASRC test simulation \n");
    FILE *fp = fopen("args.txt", "r");
    if(fp == NULL)
    {
      printf("ERROR: Missing args. Exiting\n");
      ShowUsage();
      _Exit(1);
    }

    // Get the length of the line first
    int count = 0;
    int ch = fgetc(fp);
    while((ch != '\n') && (ch != EOF)) {
        count += 1;
        ch = fgetc(fp);
    }
    if(count == 0)
    {
      printf("ERROR: Missing args. Exiting\n");
      ShowUsage();
      _Exit(1);
    }
    fseek(fp, 0, SEEK_SET);
    char *args = malloc((count+1)*sizeof(char));
    // Read the full line
    assert(fgets(args, count+1, fp) != NULL);
    printf("args = %s\n", args);

    char *pch;
    pch = strtok(args, " ");
    while(pch != NULL)
    {
        parse_cmd_line(pch);
        pch = strtok(NULL, " ");
    }

    //Test for valid arguments
    if (uiNTotalInSamples == -1)
    {
        printf("ERROR: number of input samples not set\n");
        _Exit(1);
    }

    if (uiInFs == -1)
    {
        printf("ERROR: input sample rate not set\n");
        _Exit(1);
    }

    if (uiOutFs == -1)
    {
        printf("ERROR: output sample rate not set\n");
        _Exit(1);
    }

    if (fFsRatioDeviation == -1)
    {
        printf("ERROR: sample ratio nominal ration deviation not set\n");
        _Exit(1);
    }
    return;
}

#include "fileio.h"
#include <xcore/channel.h>
#include <xcore/chanend.h>

const int sample_rates[] = {44100, 48000, 88200, 96000, 176400, 192000};

#define FILEREAD_CHUNK_SIZE (1024)

void dsp_mgr(chanend_t c_dsp[], double fFsRatioDeviation){

    file_t InFileDat[ASRC_N_CHANNELS];
    file_t OutFileDat[ASRC_N_CHANNELS];


    unsigned count_in = 0, count_out = 0;
    unsigned iEndOfFile  = 0;
    unsigned int    sr_in_out = uiInFs << 16 | uiOutFs ; //Input fs in upper 16bits and Output fs in lower 16bits

    uint64_t FsRatio = (unsigned long long)(((double)sample_rates[uiInFs] / sample_rates[uiOutFs]) * ((unsigned long long)1 << (28 + 32)));

    FsRatio =  (uint64_t)((double)FsRatio * fFsRatioDeviation); //Ensure is precisely the same as golden value complete with trucation due to 32b float
    printf("Adjusted FsRatio dsp_mgr = %lld, 0x%llx\n", FsRatio, FsRatio);


    int ret;
    for (int i=0; i<ASRC_N_CHANNELS; i++)
    {
        ret = file_open(&InFileDat[i], (char *)pzInFileName[i], "rb");
        if(ret)
        {
          printf("Error while opening input file, %s\n", pzInFileName[i]);
          _Exit(1);
        }
        printf("Simulator opening input file, %s\n", pzInFileName[i]);

        ret = file_open(&OutFileDat[i], (char *)pzOutFileName[i], "wb");
        if(ret)
        {
          printf("Error while opening output file, %s\n", pzOutFileName[i]);
          _Exit(1);
        }
        printf("Simulator opening output file, %s\n", pzOutFileName[i]);
    }


    while(!iEndOfFile)
    {
        for (unsigned j=0; j<ASRC_N_INSTANCES; j++) {
            chan_out_word(c_dsp[j], sr_in_out);
            chan_out_buf_byte(c_dsp[j], (uint8_t*)&FsRatio, sizeof(FsRatio));
        }

        for(unsigned i = 0; i < ASRC_N_IN_SAMPLES * ASRC_CHANNELS_PER_INSTANCE; i++) {
            int32_t samp;
            for (unsigned j=0; j<ASRC_N_INSTANCES; j++) {
                unsigned file_index = (i * ASRC_N_INSTANCES + j) % ASRC_N_CHANNELS;
                file_read (&InFileDat[file_index], (uint8_t*)&samp, sizeof(samp));
                if(count_in >= uiNTotalInSamples)
                {
                  iEndOfFile = 1;     //We are at the end of the file
                  printf("EOF\n");
                }
                chan_out_word(c_dsp[j], samp);  //Send the sample
            }
        }
        count_in += ASRC_N_IN_SAMPLES;


        unsigned n_samps;
        for (unsigned j=0; j<ASRC_N_INSTANCES; j++)
        {
            n_samps = chan_in_word(c_dsp[j]); //Get number of samps to receive
        }

        for(unsigned i = 0; i < n_samps * ASRC_CHANNELS_PER_INSTANCE; i++)
        {
            int samp;
            for (unsigned j=0; j<ASRC_N_INSTANCES; j++)
            {
                unsigned file_index = (i * ASRC_N_INSTANCES + j) % ASRC_N_CHANNELS;
                samp = chan_in_word(c_dsp[j]); //Get samples

                file_write(&OutFileDat[file_index], (uint8_t*)(&samp),  sizeof(samp));
            }
        }

        count_out += n_samps;

        //Force a sample rate change at a given count
        //if (count_in >= SAMP_IN_CHANGE_SR) sr_in_out = INPUT_FS_DEFAULT << 16 | INPUT_FS_DEFAULT;
    }

    printf("DSP manager done - %d output samples produced\n", count_out);

    for (int i=0; i<ASRC_N_CHANNELS; i++)
    {
        file_close(&InFileDat[i]);
        file_close(&OutFileDat[i]);
        shutdown_session();
    }

    exit(0);
}


void dsp_slave(chanend_t c_dsp)
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

    unsigned t1=0,t2=0,t_dsp=0;

    unsigned int    n_samps_out = 0;    //number of samples produced by last call to ASRC
    unsigned int    n_samps_in_tot = 0; //Total number of input samples through ASRC
    uint64_t    FsRatio = ASRC_NOMINAL_FS_SCALE; //Deviation between in Fs and out Fs

    for(int ui = 0; ui < ASRC_CHANNELS_PER_INSTANCE; ui++)
    {
        // Set state, stack and coefs into ctrl structure
        asrc_ctrl[ui].psState                   = &asrc_state[ui];
        asrc_ctrl[ui].piStack                   = asrc_stack[ui];
        asrc_ctrl[ui].piADCoefs                 = asrc_adfir_coefs.iASRCADFIRCoefs;
    }

    memset(out_buff, 0, ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX * ASRC_CHANNELS_PER_INSTANCE * sizeof(int));

    while(1){
        t2 = get_reference_time();  //Grab time at processing finished (t1 set at end of this loop)
        t_dsp = (t2 - t1);
        int sample_time = 100000000 / sample_rates[sr_in_out >> 16];
        if (n_samps_in_tot) printf("Process time per chan ticks=%d, Tot samp in count=%d\n",
            (t_dsp / (ASRC_CHANNELS_PER_INSTANCE * ASRC_N_IN_SAMPLES)),
            n_samps_in_tot);
        sr_in_out_new = chan_in_word(c_dsp);
        chan_in_buf_byte(c_dsp, (uint8_t*)&FsRatio, sizeof(FsRatio));

        for(unsigned i=0; i<ASRC_N_IN_SAMPLES; i++) {
            unsigned tmp;
            for (unsigned j=0; j<ASRC_CHANNELS_PER_INSTANCE; j++) {
                tmp = chan_in_word(c_dsp);
                in_buff[i*ASRC_CHANNELS_PER_INSTANCE + j] = tmp;
                //printf("n_samp=%d chan=%d, tmp=%d\n", i, j, tmp);
            }
        }

        n_samps_in_tot += ASRC_N_IN_SAMPLES;

        chan_out_word(c_dsp, n_samps_out); //Send number of samples to receive

        for(unsigned uj = 0; uj < n_samps_out; uj++)
        {
            unsigned tmp;
            for (unsigned j=0; j<ASRC_CHANNELS_PER_INSTANCE; j++) {
                tmp = out_buff[uj*ASRC_CHANNELS_PER_INSTANCE + j];
                chan_out_word(c_dsp, tmp);
            }
        }

        if (sr_in_out_new != sr_in_out) {
            // Set input/output sampling rate codes. See ASRCFs_t in src_mrhf_asrc.h (ie 0=44.1 5=192 etc.)
            unsigned InFs                     = (sr_in_out_new >> 16) & 0xffff;
            unsigned OutFs                    = sr_in_out_new & 0xffff;

            uint64_t nominal_FsRatio = asrc_init(InFs, OutFs, asrc_ctrl, ASRC_CHANNELS_PER_INSTANCE, ASRC_N_IN_SAMPLES, ASRC_DITHER_SETTING);

            sr_in_out = sr_in_out_new;
            printf("DSP init Initial nominal_FsRatio=%lld, SR in=%d, SR out=%d\n", nominal_FsRatio, InFs, OutFs);
        }
        t1 = get_reference_time();
        n_samps_out = asrc_process(in_buff, out_buff, FsRatio, asrc_ctrl);
    }
}
