// Copyright 2024-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

// See https://github.com/xmos/lib_src/issues/163

#if defined(__VX4B__)
#include <stdint.h>

#include <basix/architecture.h>

static inline void delay_milliseconds(unsigned milliseconds)
{
    uint64_t ticks = (uint64_t)milliseconds * BASIX_TIMER_HZ / 1000U;
    basix_delay_ticks((uint32_t)ticks);
}
#endif
