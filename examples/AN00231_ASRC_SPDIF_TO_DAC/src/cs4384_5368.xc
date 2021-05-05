// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xassert.h>
#include <timer.h>
#include <debug_print.h>
#include "cs4384_5368.h"

//Address on I2C bus
#define CS4384_I2C_ADDR      (0x18)

//Register Addresess
#define CS4384_CHIP_REV      0x01
#define CS4384_MODE_CTRL     0x02
#define CS4384_PCM_CTRL      0x03
#define CS4384_DSD_CTRL      0x04
#define CS4384_FLT_CTRL      0x05
#define CS4384_INV_CTRL      0x06
#define CS4384_GRP_CTRL      0x07
#define CS4384_RMP_MUTE      0x08
#define CS4384_MUTE_CTRL     0x09
#define CS4384_MIX_PR1       0x0a
#define CS4384_VOL_A1        0x0b
#define CS4384_VOL_B1        0x0c
#define CS4384_MIX_PR2       0x0d
#define CS4384_VOL_A2        0x0e
#define CS4384_VOL_B2        0x0f
#define CS4384_MIX_PR3       0x10
#define CS4384_VOL_A3        0x11
#define CS4384_VOL_B3        0x12
#define CS4384_MIX_PR4       0x13
#define CS4384_VOL_A4        0x14
#define CS4384_VOL_B4        0x15
#define CS4384_CM_MODE       0x16

//Address on I2C bus
#define CS5368_I2C_ADDR      (0x4C)

//Register Addresess
#define CS5368_CHIP_REV      0x00
#define CS5368_GCTL_MDE      0x01
#define CS5368_OVFL_ST       0x02
#define CS5368_OVFL_MSK      0x03
#define CS5368_HPF_CTRL      0x04
#define CS5368_PWR_DN        0x06
#define CS5368_MUTE_CTRL     0x08
#define CS5368_SDO_EN        0x0a

[[distributable]]
void audio_codec_cs4384_cs5368(server audio_codec_config_if i_codec,
                   client i2c_master_if i2c,
                   enum codec_mode_t codec_mode,
                   client output_gpio_if i_dsd_mode,
                   client output_gpio_if i_dac_rst_n,
                   client output_gpio_if i_adc_rst_n,
                   client output_gpio_if i_mclk_fsel)
{
    while (1) {
      select {
        case i_codec.reset(unsigned sample_frequency, unsigned master_clock_frequency):

            i_dac_rst_n.output(0);//Assert DAC reset
            i_adc_rst_n.output(0);//Assert ADC reset

            delay_milliseconds(10); //Allow reset to establish. Also ensures that invalid SR will be detected briefly

            if ((sample_frequency % 48000) != 0) i_mclk_fsel.output(0);
            else                                 i_mclk_fsel.output(1);
            /* dsdMode == 0 */
            /* Set MUX to PCM mode (muxes ADC I2S data lines) */
            i_dsd_mode.output(0);

            /* Configure DAC with PCM values. Note 2 writes to mode control to enable/disable freeze/power down */
            i_dac_rst_n.output(1);//De-assert DAC reset

            delay_microseconds(10); //Allow exit from reset

            /* Mode Control 1 (Address: 0x02) */
            /* bit[7] : Control Port Enable (CPEN)     : Set to 1 for enable
            * bit[6] : Freeze controls (FREEZE)       : Set to 1 for freeze
            * bit[5] : PCM/DSD Selection (DSD/PCM)    : Set to 0 for PCM
            * bit[4:1] : DAC Pair Disable (DACx_DIS)  : All Dac Pairs enabled
            * bit[0] : Power Down (PDN)               : Powered down
            */
            i2c.write_reg(CS4384_I2C_ADDR, CS4384_MODE_CTRL, 0b11000001);

#ifdef I2S_MODE_TDM
            /* PCM Control (Address: 0x03) */
            /* bit[7:4] : Digital Interface Format (DIF) : 0b1100 for TDM
            * bit[3:2] : Reserved
            * bit[1:0] : Functional Mode (FM) : 0x11 for auto-speed detect (32 to 200kHz)
            */
            i2c.write_reg(CS4384_I2C_ADDR, CS4384_PCM_CTRL, 0b11000111);
#else
            /* PCM Control (Address: 0x03) */
            /* bit[7:4] : Digital Interface Format (DIF) : 0b0001 for I2S up to 24bit
            * bit[3:2] : Reserved
            * bit[1:0] : Functional Mode (FM) : 0x00 - single-speed mode (4-50kHz)
            *                                 : 0x01 - double-speed mode (50-100kHz)
            *                                 : 0x10 - quad-speed mode (100-200kHz)
            *                                 : 0x11 - auto-speed detect (32 to 200kHz)
            *                                 (note, some Mclk/SR ratios not supported in auto)
            *
            */
            unsigned char regVal = 0;
            if(sample_frequency < 50000)
               regVal = 0b00010100;
            else if(sample_frequency < 100000)
               regVal = 0b00010101;
            else //if(sample_frequency < 200000)
               regVal = 0b00010110;

            i2c.write_reg(CS4384_I2C_ADDR, CS4384_PCM_CTRL, regVal);
#endif

            /* Mode Control 1 (Address: 0x02) */
            /* bit[7] : Control Port Enable (CPEN)     : Set to 1 for enable
            * bit[6] : Freeze controls (FREEZE)       : Set to 0 for freeze
            * bit[5] : PCM/DSD Selection (DSD/PCM)    : Set to 0 for PCM
            * bit[4:1] : DAC Pair Disable (DACx_DIS)  : All Dac Pairs enabled
            * bit[0] : Power Down (PDN)               : Not powered down
            */
            i2c.write_reg(CS4384_I2C_ADDR, CS4384_MODE_CTRL, 0b10000000);

            /* Take ADC out of reset */
            i_adc_rst_n.output(1);

            delay_microseconds(10); //Allow exit from reset


            {
               unsigned dif = 0, mode = 0;
#ifdef I2S_MODE_TDM
                dif = 0x02;   /* TDM */
#else
                dif = 0x01;   /* I2S */
#endif

                if(codec_mode == CODEC_IS_I2S_MASTER) {
                    /* Note, only the ADC device supports being I2S master.
                    * Set ADC as master and run DAC as slave */
                    if(sample_frequency < 54000)
                       mode = 0x00;     /* Single-speed Mode Master */
                    else if(sample_frequency < 108000)
                       mode = 0x01;     /* Double-speed Mode Master */
                    else if(sample_frequency < 216000)
                       mode = 0x02;     /* Quad-speed Mode Master */
                }
                else { //CODEC_IS_I2S_SLAVE - i.e. xCore is master
                    mode = 0x03;    /* Slave mode all speeds */
                }

                /* Reg 0x01: (GCTL) Global Mode Control Register */
                /* Bit[7]: CP-EN: Manages control-port mode
                * Bit[6]: CLKMODE: Setting puts part in 384x mode
                * Bit[5:4]: MDIV[1:0]: Set to 01 for /2
                * Bit[3:2]: DIF[1:0]: Data Format: 0x01 for I2S, 0x02 for TDM
                * Bit[1:0]: MODE[1:0]: Mode: 0x11 for slave mode
                */
                i2c.write_reg(CS5368_I2C_ADDR, CS5368_GCTL_MDE, 0b10010000 | (dif << 2) | mode);
            }

            /* Reg 0x06: (PDN) Power Down Register */
            /* Bit[7:6]: Reserved
            * Bit[5]: PDN-BG: When set, this bit powers-own the bandgap reference
            * Bit[4]: PDM-OSC: Controls power to internal oscillator core
            * Bit[3:0]: PDN: When any bit is set all clocks going to that channel pair are turned off
            */
            i2c.write_reg(CS5368_I2C_ADDR, CS5368_PWR_DN, 0b00000000);

            debug_printf("SR change in lib_audio_codec - %d\n", sample_frequency);
            break;
        } //select
    } //while (1)
}
