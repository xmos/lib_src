#include <stdint.h>
#include "asrc_timestamp_interpolation.h"

// TODO: replace TODO with BASE and 8/24 with division with MAX

// TODO: check with 88200 and other input frequencies

int asrc_timestamp_interpolation(int timestamp, asrc_ctrl_t *asrc_ctrl, int interpolation_ticks) {
    // iTimeInt is an 8-bit number; fract a 32 bit number, with the two we make a 16-bit
    // fraction that we multiply with and then shift down by 16 bits.
    uint32_t fraction_away_from_final_ts = (((asrc_ctrl->iTimeInt - 128) << 8) |
                                            ((asrc_ctrl->uiTimeFract >> 24) & 0xff));
    int32_t left_over_ticks = (fraction_away_from_final_ts * interpolation_ticks) >> 16;
    return timestamp + left_over_ticks;
}
