// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "wav_writer.h"

#include <stdint.h>
#include <stdio.h>

static void put_u16(unsigned char *bytes, unsigned value)
{
    bytes[0] = (unsigned char)value;
    bytes[1] = (unsigned char)(value >> 8);
}

static void put_u32(unsigned char *bytes, uint32_t value)
{
    for (unsigned byte = 0; byte < 4; ++byte) {
        bytes[byte] = (unsigned char)(value >> (8 * byte));
    }
}

int wav_write(const char *filename, unsigned sample_rate, unsigned channels,
              const int samples[], unsigned frames)
{
    if (channels == 0 || channels > UINT16_MAX / 4 ||
        sample_rate == 0 || sample_rate > UINT32_MAX / (channels * 4) ||
        frames > (UINT32_MAX - 36) / (channels * 4)) {
        printf("Error: invalid WAV format or size for %s\n", filename);
        return 1;
    }
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error: cannot open %s for writing\n", filename);
        return 1;
    }

    unsigned char header[44] = {
        'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0,
        'd', 'a', 't', 'a', 0, 0, 0, 0
    };
    unsigned data_bytes = frames * channels * 4;
    put_u32(header + 4, 36 + data_bytes);
    put_u16(header + 22, channels);
    put_u32(header + 24, sample_rate);
    put_u32(header + 28, sample_rate * channels * 4);
    put_u16(header + 32, channels * 4);
    put_u32(header + 40, data_bytes);
    int failed = fwrite(header, 1, sizeof(header), file) != sizeof(header);

    unsigned char buffer[256];
    unsigned used = 0;
    for (unsigned i = 0; i < frames * channels && !failed; ++i) {
        put_u32(buffer + used, (uint32_t)samples[i]);
        used += 4;
        if (used == sizeof(buffer) || i + 1 == frames * channels) {
            failed = fwrite(buffer, 1, used, file) != used;
            used = 0;
        }
    }
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        printf("Error: writing %s failed\n", filename);
        return 1;
    }
    return 0;
}
