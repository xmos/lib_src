# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import math
import os
from pathlib import Path
import struct
import subprocess
import wave

import pytest


SAMPLE_RATE = 16000
CHANNELS = 4
DURATION_MS = 20
FRAMES = SAMPLE_RATE * DURATION_MS // 1000
SAMPLE_WIDTH = 4
AMPLITUDE = 0.9
TONE_SPACING_HZ = 1000
FULL_SCALE = (1 << 31) - 1
OUTPUT_SETTLED_FRAME = 96
OUTPUT_PHASE_OFFSET = 1
INPUT_TOLERANCE = 128
OUTPUT_TOLERANCE = 0.003 * (FULL_SCALE + 1)
APP_DIR = Path(__file__).resolve().parents[2] / "examples" / "app_ff3"
EXAMPLE_EXECUTABLE = APP_DIR / "bin" / "app_ff3.xe"


@pytest.mark.prepare
def test_prepare_ff3_example(tmp_path_factory):
    build = tmp_path_factory.mktemp("ff3-example-build")
    target = os.environ.get("FF3_EXAMPLE_TARGET", "XK-EVK-XU316")
    subprocess.run(
        ["cmake", "-G", "Unix Makefiles", "-S", str(APP_DIR), "-B", str(build),
         f"-DAPP_HW_TARGET={target}"],
        check=True,
    )
    subprocess.run(["xmake", "-C", str(build), "-j4"], check=True)
    assert EXAMPLE_EXECUTABLE.is_file(), f"Executable {EXAMPLE_EXECUTABLE} doesn't exist"


@pytest.mark.main
def test_ff3_example(tmp_path):
    assert EXAMPLE_EXECUTABLE.is_file(), (
        f"Executable {EXAMPLE_EXECUTABLE} doesn't exist; run the prepare tests first"
    )
    result = subprocess.run(
        ["xsim", "--max-cycles", "1000000000", str(EXAMPLE_EXECUTABLE)],
        cwd=tmp_path, capture_output=True, text=True, timeout=360,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert f"Complete: {FRAMES} frames" in result.stdout

    for name in ("input", "output"):
        with wave.open(str(tmp_path / f"{name}.wav"), "rb") as wav:
            assert (wav.getframerate(), wav.getnchannels(), wav.getsampwidth(),
                    wav.getnframes()) == (SAMPLE_RATE, CHANNELS, SAMPLE_WIDTH, FRAMES)
            samples = struct.unpack(f"<{FRAMES * CHANNELS}i", wav.readframes(FRAMES))
        for channel in range(CHANNELS):
            start = 0 if name == "input" else OUTPUT_SETTLED_FRAME
            # The settled round trip shifts the 16-sample base period by one.
            phase_offset = 0 if name == "input" else OUTPUT_PHASE_OFFSET
            tolerance = INPUT_TOLERANCE if name == "input" else OUTPUT_TOLERANCE
            for frame in range(start, FRAMES):
                expected = AMPLITUDE * FULL_SCALE * math.sin(
                    2 * math.pi * TONE_SPACING_HZ * (channel + 1)
                    * (frame + phase_offset) / SAMPLE_RATE
                )
                assert abs(samples[frame * CHANNELS + channel] - expected) < tolerance
