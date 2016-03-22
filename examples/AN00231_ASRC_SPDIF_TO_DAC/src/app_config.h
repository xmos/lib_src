// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_


#define     ASRC_N_CHANNELS                 2  //Total number of audio channels to be processed by SRC (minimum 1)
#define     ASRC_N_INSTANCES                2  //Number of instances (each usually run a logical core) used to process audio (minimum 1)
#define     ASRC_CHANNELS_PER_INSTANCE      (ASRC_N_CHANNELS/ASRC_N_INSTANCES)
                                               //Calcualted number of audio channels processed by each core
#define     ASRC_N_IN_SAMPLES               8 //Number of samples per channel in each block passed into SRC each call
                                               //Must be a power of 2 and minimum value is 4 (due to two /2 decimation stages)
#define     ASRC_N_OUT_IN_RATIO_MAX         3  //Max ratio between samples out:in per processing step (44.1->192 is worst case)
#define     ASRC_MAX_BLOCK_SIZE             (ASRC_N_IN_SAMPLES * ASRC_N_OUT_IN_RATIO_MAX)
#define     OUT_FIFO_SIZE                   (ASRC_MAX_BLOCK_SIZE * 8)  //Size per channel of block2serial output FIFO
#define     ASRC_DITHER_SETTING             0  //No output dithering of samples from 32b to 24b

#define     DEFAULT_FREQ_HZ_SPDIF           48000
#define     DEFAULT_FREQ_HZ_I2S             48000
#define     MCLK_FREQUENCY_48               24576000
#define     MCLK_FREQUENCY_44               22579200


#endif /* APP_CONFIG_H_ */
