// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _ASRC_WRAPPER_H_
#define _ASRC_WRAPPER_H_

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

#define         MAX_ASRC_N_IO_CHANNELS                          (2)

// Sampling rate codes
// -------------------
typedef enum _ASRCFs
{
    ASRC_FS_44								= 0,								// Fs = 44.1kHz code
    ASRC_FS_48								= 1,								// Fs = 48kHz code
    ASRC_FS_88								= 2,								// Fs = 88.2kHz code
    ASRC_FS_96								= 3,								// Fs = 96kHz code
    ASRC_FS_176								= 4,								// Fs = 176.4kHz code
    ASRC_FS_192								= 5,								// Fs = 192kHz code
} ASRCFs_t;
#define		ASRC_N_FS						(ASRC_FS_192 + 1)
#define		ASRC_FS_MIN						ASRC_FS_44
#define		ASRC_FS_MAX						ASRC_FS_192

// Parameter values
// ----------------
#define		ASRC_ON								1
#define		ASRC_OFF							0
#define		ASRC_DITHER_OFF						ASRC_OFF
#define		ASRC_DITHER_ON						ASRC_ON

typedef struct {
		float									fCycleCountF1F2;					// Variable to hold cycle count for MIPS estimations for F1 and F2 stages
		float									fCycleCountF3AdaptiveCoefs;			// Variable to hold cycle count for MIPS estimations for F3 adaptive filters computation
		float									fCycleCountF3;						// Variable to hold cycle count for MIPS estimations for F3 computation
		float									fCycleCountDither;					// Variable to hold cycle count for MIPS estimations for dither computation
}ASRCCtrl_profile_only_t;

uint64_t wrapper_asrc_init(
        ASRCCtrl_profile_only_t* (*profile_info_ptr)[MAX_ASRC_N_IO_CHANNELS],   // Pointer to array of pointers
        unsigned uiInFs,
        unsigned uiOutFs,
        unsigned uiNInSamples,
        unsigned num_io_channels,
        unsigned num_channels_per_asrc_instance,
        unsigned dither_on_off,
		unsigned *rand_seed);

unsigned wrapper_asrc_process(
    int *piIn,
    int *piOut,
    uint64_t fs_ratio
    );

#ifdef __cplusplus
 }
#endif
#endif
