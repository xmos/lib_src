// Copyright (c) 2016, XMOS Ltd, All rights reserved
#ifndef AUDIO_CODEC_H_
#define AUDIO_CODEC_H_
#include <xs1.h>
#include <gpio.h>
#include <i2c.h>
#include <stdint.h>

enum codec_mode_t {
  CODEC_IS_I2S_MASTER,
  CODEC_IS_I2S_SLAVE
};

/** This interface provides a generic set of functions that control audio
    codec functionality */
typedef interface audio_codec_config_if
{
  /** Reset a codec.
   *
   *  This function resets a codec and configures it for a particular
   *  sample frequency.
   *
   *  \param sample_frequency        The required sample frequency to run at
   *                                 after reset (in Hz).
   *  \param master_clock_frequency  The frequency of the master clock
   *                                 supplied to the codec (in Hz).
   *
   */
  void reset(unsigned sample_frequency, unsigned master_clock_frequency);
} audio_codec_config_if;


/** A driver for a the adc/dac pair on the xu216/mc hardware.
 *
 *  This driver provides the audio codec configuration interface for
 *  the XU216 Multichannel Audio board 2.0
 *
 *  \param  i                  The provided codec configuration interface.
 *  \param  i_i2c              I2C interface. This should connect to an
 *                             I2C component connected to the I2C bus
 *                             connected to the codec.
 *  \param  device_addr        The I2C bus address of the codec
 *  \param  mode               Whether the codec should be in master of
 *                             slave mode.
 */
[[distributable]]
void audio_codec_cs4384_cs5368(server audio_codec_config_if i_codec,
                    client i2c_master_if i2c,
                    enum codec_mode_t codec_mode,
                    client output_gpio_if i_dsd_mode,
                    client output_gpio_if i_dac_rst_n,
                    client output_gpio_if i_adc_rst_n,
                    client output_gpio_if i_mclk_fsel);

#endif /* AUDIO_CODEC_H_ */
