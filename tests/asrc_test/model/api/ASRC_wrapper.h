#ifndef _ASRC_WRAPPER_H_
#define _ASRC_WRAPPER_H_

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif
uint64_t wrapper_asrc_init(
        unsigned uiInFs,
        unsigned uiOutFs,
        unsigned uiNInSamples,
        unsigned num_io_channels,
        unsigned num_channels_per_asrc_instance,
        unsigned dither_on_off,
		unsigned *rand_seed);

//unsigned wrapper_asrc_process(int *piIn, int *piOut, unsigned fs_ratio, unsigned fs_ratio_lsb);

#ifdef __cplusplus
 }
#endif
#endif
