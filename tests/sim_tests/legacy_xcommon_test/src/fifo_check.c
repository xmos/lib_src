// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <string.h>
#include "src.h"
#include "asynchronous_fifo.h"

#define FIFO_CHANNELS 2
// Deep enough that the producer's overflow guard does not drop any test frames.
#define FIFO_DEPTH 32
#define FIFO_PREFILL (FIFO_DEPTH / 2)
#define FIFO_FRAMES 6
// The consumer only advances while more than two frames remain in the FIFO.
#define FIFO_READABLE (FIFO_FRAMES - 2)

static int64_t fifo_storage[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(FIFO_DEPTH, FIFO_CHANNELS)];

int fifo_check(void)
{
    asynchronous_fifo_t *fifo = (asynchronous_fifo_t *)fifo_storage;
    int32_t frame[FIFO_CHANNELS];
    int32_t timestamp = 0;

    memset(fifo_storage, 0, sizeof(fifo_storage));
    asynchronous_fifo_init(fifo, FIFO_CHANNELS, FIFO_DEPTH);
    asynchronous_fifo_init_PID_fs_codes(fifo, FS_CODE_48, FS_CODE_48);

    for (int i = 0; i < FIFO_FRAMES; ++i) {
        frame[0] = 100 + i;
        frame[1] = -100 - i;
        asynchronous_fifo_producer_put(fifo, frame, 1, timestamp);
        timestamp += 2083;
    }
    if (fifo->stop_producing) {
        printf("FIFO producer stopped unexpectedly\n");
        return 1;
    }

    for (int i = 0; i < FIFO_PREFILL + FIFO_READABLE; ++i) {
        if (asynchronous_fifo_consumer_get(fifo, frame, timestamp) != ASYNCH_FIFO_OK) {
            printf("FIFO get %d did not return ASYNCH_FIFO_OK\n", i);
            return 1;
        }
        if (i >= FIFO_PREFILL) {
            const int32_t expected = 100 + (i - FIFO_PREFILL);
            if (frame[0] != expected || frame[1] != -expected) {
                printf("FIFO frame %d: expected {%d, %d}, got {%d, %d}\n",
                       i - FIFO_PREFILL, (int)expected, (int)-expected,
                       (int)frame[0], (int)frame[1]);
                return 1;
            }
        }
    }

    if (asynchronous_fifo_consumer_get(fifo, frame, timestamp) != ASYNCH_FIFO_UNDERFLOW) {
        printf("FIFO did not report underflow\n");
        return 1;
    }
    if (asynchronous_fifo_consumer_get(fifo, frame, timestamp) != ASYNCH_FIFO_IN_RESET) {
        printf("FIFO did not report reset after underflow\n");
        return 1;
    }

    asynchronous_fifo_producer_put(fifo, frame, 1, timestamp);
    if (fifo->reset) {
        printf("FIFO producer did not clear reset\n");
        return 1;
    }

    asynchronous_fifo_reset_consumer(fifo);
    if (asynchronous_fifo_consumer_get(fifo, frame, timestamp) != ASYNCH_FIFO_IN_RESET) {
        printf("FIFO did not report consumer-requested reset\n");
        return 1;
    }

    asynchronous_fifo_exit(fifo);
    return 0;
}
