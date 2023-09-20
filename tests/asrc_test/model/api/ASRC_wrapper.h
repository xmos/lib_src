#ifndef _ASRC_WRAPPER_H_
#define _ASRC_WRAPPER_H_

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

#define         MAX_ASRC_N_IO_CHANNELS                          (2)

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
