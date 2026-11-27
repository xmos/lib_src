// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef WAV_WRITER_H_
#define WAV_WRITER_H_

/* Write interleaved, signed 32-bit PCM; return zero on success. */
int wav_write(const char *filename, unsigned sample_rate, unsigned channels,
              const int samples[], unsigned frames);

#endif
