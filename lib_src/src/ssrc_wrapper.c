// General includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <timer.h>

// SSRC includes
#include "ssrc.h"

static void bomb_out(int code){
    printf("SSRC_proc Error code %d\n", code);
    delay_milliseconds(1);
	_Exit(0);
}

// SSRC instances variables - State, Stack, Coefs and Control structures
static SSRCState_t     sSSRCState[SSRC_N_CORES];
static int             iSSRCStack[SSRC_N_CORES][SSRC_STACK_LENGTH_MULT * SSRC_N_IN_SAMPLES];
static SSRCCtrl_t      sSSRCCtrl[SSRC_N_CORES];



void ssrc_init(unsigned sr_in, unsigned sr_out, unsigned thread_number)
{
    printf("DSP init sr_in=%d, sr_out=%d\n", sr_in, sr_out);

    SSRCReturnCodes_t ret_code;

    // Set state, stack and coefs into ctrl structure
    sSSRCCtrl[thread_number].psState                   = &sSSRCState[thread_number];
    sSSRCCtrl[thread_number].piStack                   = iSSRCStack[thread_number];

    // Set number of samples
    sSSRCCtrl[thread_number].uiNInSamples              = SSRC_N_IN_SAMPLES;

    // Set dither flag and random seeds
    sSSRCCtrl[thread_number].uiDitherOnOff             = SSRC_DITHER_OFF;
    sSSRCCtrl[thread_number].uiRndSeedInit[0]             = 1234567;


    sSSRCCtrl[thread_number].eInFs                     = sr_in;
    sSSRCCtrl[thread_number].eOutFs                    = sr_out;

    printf("structure initialised\n");
    // Init SSRC instances

    ret_code = SSRC_init(&sSSRCCtrl[thread_number]) ;
    if(ret_code != SSRC_NO_ERROR) bomb_out(ret_code);
    printf("Init function success\n");

}

unsigned ssrc_process(int *in_buff, int *out_buff, unsigned thread_number){
    sSSRCCtrl[thread_number].piIn = in_buff;
    sSSRCCtrl[thread_number].piOut = out_buff;
    //printf("in_buff=%p, out_buff=%p\n", in_buff, out_buff);
    if(SSRC_proc(&sSSRCCtrl[thread_number]) != SSRC_NO_ERROR) bomb_out(0);
    unsigned n_samps_out = (*sSSRCCtrl[thread_number].puiNOutSamples);
    //for(unsigned uj = 0; uj < n_samps_out; uj++)  printf("out=%i\n",out_buff[uj]);
    return n_samps_out;
}
