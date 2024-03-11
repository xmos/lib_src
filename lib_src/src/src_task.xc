
#include "src_task.h"

fs_code_t sr_to_fscode(unsigned sr)
{
    fs_code_t fsCode;
    switch (sr)
    {
        case 44100:
            fsCode = FS_CODE_44;
            break;
        case 48000:
            fsCode = FS_CODE_48;
            break;
        case 88200:
            fsCode = FS_CODE_88;
            break;
        case 96000:
            fsCode = FS_CODE_96;
            break;
        case 176400:
            fsCode = FS_CODE_176;
            break;
        case 192000:
            fsCode = FS_CODE_192;
            break;
        default:
            assert(0);
            break;
    }
    return fsCode;
}

static void src_change_worker_freqs(streaming chanend c[numInstances], unsigned numInstances, int inputSr, int outputSr)
{
    for(int i=0; i < numInstances; i++)
    unsafe
    {
        soutct(c[i], XS1_CT_END);
        schkct(c[i], XS1_CT_END);
        c[i] <: (int)sr_to_fscode(inputSr);
        c[i] <: (int)sr_to_fscode(outputSr);
        soutct(c[i], XS1_CT_END);
        schkct(c[i], XS1_CT_END);
    }
}

void src_task_init(src_task_t * unsafe srcState, int64_t array[], int numChans, int fifoLength, int xscopeUsed)
{
    unsafe
    {
        srcState->async_fifo = (asynchronous_fifo_t *) array;
        srcState->xscopeUsed = xscopeUsed;
        asynchronous_fifo_init(srcState->async_fifo, numChans, fifoLength);
    }
}

void src_task_set_sr(src_task_t * unsafe srcState, int inputSr, int outputSr, streaming chanend c[numInstances], unsigned numInstances)
{
    float floatRatio = (float)inputSr/(float)outputSr;
    unsafe
    {
        srcState->fsRatio = (uint64_t) (floatRatio * (1LL << 60));
        srcState->idealFsRatio = (srcState->fsRatio + (1<<31)) >> 32;

        asynchronous_fifo_reset_producer(srcState->async_fifo);
        asynchronous_fifo_init_PID_fs_codes(srcState->async_fifo, sr_to_fscode(inputSr), sr_to_fscode(outputSr));
    }

    src_change_worker_freqs(c, numInstances, inputSr, outputSr);
}

/* Send/receive samples from the SRC workers */
#pragma unsafe arrays
void src_task_write(streaming chanend c_src[SRC_N_INSTANCES],
                                int srcInputBuff[SRC_N_INSTANCES][SRC_N_IN_SAMPLES][SRC_CHANNELS_PER_INSTANCE],
                                int32_t now,
                                src_task_t * unsafe srcState)
{
    int32_t error = 0;
    int nSamps = 0;
    int32_t timestamp;
    int32_t samples[SRC_CHANNELS_PER_INSTANCE*SRC_N_CHANNELS * SRC_MAX_NUM_SAMPS_OUT];
#pragma loop unroll
    for (int i=0; i<SRC_N_INSTANCES; i++)
    {
        unsafe
        {
            c_src[i] <: (uint64_t) srcState->fsRatio;
        }

#pragma loop unroll
        for (int j=0; j<SRC_N_IN_SAMPLES; j++)
        {
#pragma loop unroll
            for (int k=0; k<SRC_CHANNELS_PER_INSTANCE; k++)
            {
                c_src[i] <: srcInputBuff[i][j][k];
            }
        }
    }

    /* Get number of samples to receive from all SRC cores */
    /* Note, all nSamps should be equal */
#pragma loop unroll
    for (int i=0; i < SRC_N_INSTANCES; i++)
    {
        c_src[i] :> nSamps;
        c_src[i] :> timestamp;
        c_src[i] <: now;
    }

    int chanIdx = 0;
    for (int j=0; j < nSamps; j++)
    {
#pragma loop unroll
        for (int k=0; k<SRC_CHANNELS_PER_INSTANCE; k++)
        {
            int32_t sample;
#pragma loop unroll
            for (int i=0; i<SRC_N_INSTANCES; i++)
            {
                c_src[i] :> sample;
                samples[chanIdx++] = sample;
            }
        }
    }

    unsafe
    {
        error = asynchronous_fifo_producer_put(srcState->async_fifo, samples, nSamps, timestamp, srcState->xscopeUsed);

        /* Produce fsRatio from error */
        srcState->fsRatio = (((int64_t)srcState->idealFsRatio) << 32) + (error * (int64_t) srcState->idealFsRatio);
    }
}

static int interpolation_ticks_2D[6][6] =
{
    {  2268, 2268, 2268, 2268, 2268, 2268},
    {  2083, 2083, 2083, 2083, 2083, 2083},
    {  2268, 2268, 1134, 1134, 1134, 1134},
    {  2083, 2083, 1042, 1042, 1042, 1042},
    {  2268, 2268, 1134, 1134,  567,  567},
    {  2083, 2083, 1042, 1042,  521,  521}
};

void src_process(streaming chanend c, int instance, int inputFsCode, int outputFsCode)
{
    int inputBuff[SRC_N_IN_SAMPLES * SRC_CHANNELS_PER_INSTANCE];
    int outputBuff[SRC_OUT_BUFF_SIZE];
    int sampsOut = 0;

    int interpolation_ticks = interpolation_ticks_2D[inputFsCode][outputFsCode];

    memset(inputBuff, 0, sizeof(inputBuff));
    memset(outputBuff, 0, sizeof(outputBuff));

#if USE_ASRC
    asrc_state_t sASRCState[SRC_CHANNELS_PER_INSTANCE];                                   // ASRC state machine state
    int iASRCStack[SRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES]; // Buffer between filter stages
    asrc_ctrl_t sASRCCtrl[SRC_CHANNELS_PER_INSTANCE];                                     // Control structure
    asrc_adfir_coefs_t asrc_adfir_coefs;                                                  // Adaptive filter coefficients
    uint64_t fsRatio;

    for(int ui = 0; ui < SRC_CHANNELS_PER_INSTANCE; ui++)
    {
        unsafe
        {
            // Set state, stack and coefs into ctrl structure
            sASRCCtrl[ui].psState                   = &sASRCState[ui];
            sASRCCtrl[ui].piStack                   = iASRCStack[ui];
            sASRCCtrl[ui].piADCoefs                 = asrc_adfir_coefs.iASRCADFIRCoefs;
        }
    }
    fsRatio = asrc_init(inputFsCode, outputFsCode, sASRCCtrl, SRC_CHANNELS_PER_INSTANCE, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);

#else
    ssrc_state_t sSSRCState[SRC_CHANNELS_PER_INSTANCE];                                     // State of SSRC module
    int iSSRCStack[SRC_CHANNELS_PER_INSTANCE][SSRC_STACK_LENGTH_MULT * SRC_N_IN_SAMPLES];   // Buffers between processing stages
    ssrc_ctrl_t sSSRCCtrl[SRC_CHANNELS_PER_INSTANCE];                                       // SSRC Control structure

    /* Set state, stack and coefs into ctrl structures */
    for(int ui = 0; ui < SRC_CHANNELS_PER_INSTANCE; ui++)
    {
        unsafe
        {
            sSSRCCtrl[ui].psState                   = &sSSRCState[ui];
            sSSRCCtrl[ui].piStack                   = iSSRCStack[ui];
        }
    }

    ssrc_init(inputFsCode, outputFsCode, sSSRCCtrl, SRC_CHANNELS_PER_INSTANCE, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);
#endif

    int32_t tsIn = 0, tsOut = 0;

    while(1)
    {
        uint64_t fsRatio_;

        /* Check for exit */
        if(stestct(c))
        {
            sinct(c);
            soutct(c, XS1_CT_END);
            c :> inputFsCode;
            c :> outputFsCode;
            sinct(c);

            interpolation_ticks = interpolation_ticks_2D[inputFsCode][outputFsCode];

            fsRatio = asrc_init(inputFsCode, outputFsCode, sASRCCtrl, SRC_CHANNELS_PER_INSTANCE, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);

            /* Handshake back when init complete */
            soutct(c, XS1_CT_END);
            continue;
        }

        c :> fsRatio_;

#pragma loop unroll
        /* Receive samples to process */
        for(int i=0; i<SRC_N_IN_SAMPLES * SRC_CHANNELS_PER_INSTANCE; i++)
        {
            c :> inputBuff[i];
        }

        /* Send out the number of samples we have to output */
        c <: sampsOut;

        /* Send out the output timestamp */
        c <: tsOut;
        c :> tsIn;

        /* Send output samples */
#pragma loop unroll
        for(int i = 0; i < sampsOut * SRC_CHANNELS_PER_INSTANCE; i++)
        {
            c <: outputBuff[i];
        }

        /* Process input buffer into output buffer */
#if USE_ASRC
        sampsOut = asrc_process(inputBuff, outputBuff, fsRatio_, sASRCCtrl);
        if(sampsOut)
        unsafe
        {
            tsOut = asrc_timestamp_interpolation(tsIn, &sASRCCtrl[0], interpolation_ticks);
        }
#else
        sampsOut = ssrc_process(inputBuff, outputBuff, sSSRCCtrl);
#endif
    }
}

void src_task(streaming chanend c[numInstances], unsigned numInstances, int inputSr, int outputSr)
{
    par(int i = SRC_N_INSTANCES ; i < 2*SRC_N_INSTANCES; i++)
    {
        unsafe
        {
            src_process(c[i-SRC_N_INSTANCES], i, sr_to_fscode(inputSr), sr_to_fscode(outputSr));
        }
    }
}

