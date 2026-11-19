# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from pathlib import Path
import subprocess

import pytest

from utils.src_test_utils import build_firmware_xcommon_cmake

# These need to match the defines in asynchronous_fifo_example.c
NUM_SAMPLES = 1000
FIFO_LENGTH = 100
APP_DIR = Path(__file__).resolve().parents[2] / "examples" / "app_asynchronous_fifo"
XE = APP_DIR / "bin" / "app_asynchronous_fifo.xe"


@pytest.mark.prepare
def test_prepare_asynchronous_fifo():
    build_firmware_xcommon_cmake(APP_DIR)
    assert XE.exists(), f"Executable file {XE} doesn't exist"


@pytest.mark.main
def test_asynchronous_fifo_output():
    result = subprocess.run(["xsim", str(XE)], capture_output=True, text=True, timeout=360)
    assert result.returncode == 0, (
        f"asynchronous FIFO simulation exited {result.returncode}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )

    samples = []
    for line in result.stdout.splitlines():
        try:
            samples.append(int(line.strip()))
        except ValueError:
            continue

    assert len(samples) == NUM_SAMPLES, f"expected {NUM_SAMPLES} samples, got {len(samples)}"
    assert -1 not in samples, "consumer reported a FIFO underflow or reset"

    startup_samples = FIFO_LENGTH // 2
    assert samples[:startup_samples] == [0] * startup_samples
    expected_data = [sample * sample for sample in range(NUM_SAMPLES - startup_samples)]
    assert samples[startup_samples:] == expected_data