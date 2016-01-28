// General includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <timer.h>

// SSRC includes
#include "src.h"

static void bomb_out(int code){
	_Exit(code);
}


void ssrc_init(unsigned sr_in, unsigned sr_out, SSRCCtrl_t *sSSRCCtrl)
{
    SSRCReturnCodes_t ret_code;

    // Set number of samples
    sSSRCCtrl->uiNInSamples              = SSRC_N_IN_SAMPLES;

    // Set dither flag and random seeds
    sSSRCCtrl->uiDitherOnOff             = SSRC_DITHER_OFF;
    sSSRCCtrl->uiRndSeedInit             = 1234567;


    sSSRCCtrl->eInFs                     = sr_in;
    sSSRCCtrl->eOutFs                    = sr_out;

    // Init SSRC instances
    ret_code = SSRC_init(sSSRCCtrl) ;
    if(ret_code != SSRC_NO_ERROR) bomb_out(ret_code);

}

unsigned ssrc_process(int in_buff[], int out_buff[], SSRCCtrl_t *sSSRCCtrl){
    sSSRCCtrl->piIn = in_buff;
    sSSRCCtrl->piOut = out_buff;
    if(SSRC_proc(sSSRCCtrl) != SSRC_NO_ERROR) bomb_out(0);
    unsigned n_samps_out = (*sSSRCCtrl->puiNOutSamples);
    return n_samps_out;
}
