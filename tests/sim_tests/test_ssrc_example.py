# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import math
import os
from pathlib import Path
import re
import struct
import subprocess
import wave

import pytest


@pytest.fixture(scope="module")
def example_executable(tmp_path_factory):
    app = Path(__file__).resolve().parents[2] / "examples" / "app_ssrc"
    build = tmp_path_factory.mktemp("ssrc-example-build")
    target = os.environ.get("SSRC_EXAMPLE_TARGET", "XK-EVK-XU316")
    subprocess.run(
        ["cmake", "-G", "Unix Makefiles", "-S", str(app), "-B", str(build),
         f"-DAPP_HW_TARGET={target}"],
        check=True,
    )
    subprocess.run(["xmake", "-C", str(build), "-j4"], check=True)
    return app / "bin" / "app_ssrc.xe"


@pytest.mark.main
@pytest.mark.parametrize("input_rate,output_rate", [
    (44100, 192000), (192000, 44100), (176400, 48000), (48000, 96000),
])
def test_ssrc_example_capture(example_executable, tmp_path, input_rate, output_rate):
    result = subprocess.run(
        ["xsim", "--args", str(example_executable), str(input_rate), str(output_rate)],
        cwd=tmp_path, capture_output=True, text=True, timeout=240,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "Starting SSRC" in result.stdout
    assert "input frames; converting" in result.stdout
    assert "Complete:" in result.stdout

    captures = {}
    for name, rate in (("input", input_rate), ("output", output_rate)):
        with wave.open(str(tmp_path / f"{name}.wav"), "rb") as wav:
            assert wav.getnchannels() == 4
            assert wav.getsampwidth() == 4
            assert wav.getframerate() == rate
            frames = wav.getnframes()
            assert frames / rate <= 0.1
            data = wav.readframes(frames)
            assert len(data) == frames * 4 * 4
            captures[name] = struct.unpack(f"<{frames * 4}i", data)

    input_frames = input_rate // 100 // 4 * 4
    assert len(captures["input"]) == input_frames * 4
    output_frames = len(captures["output"]) // 4
    assert abs(output_frames - input_frames * output_rate / input_rate) <= 1
    for channel in range(4):
        for name, rate, count in (("input", input_rate, input_frames),
                                  ("output", output_rate, output_frames)):
            signal = captures[name][channel::4]
            assert len(signal) == count
            if name == "input":
                assert all(
                    abs(value - 0.9 * ((1 << 31) - 1) *
                        math.sin(2 * math.pi * 1000 * (channel + 1) * index / rate)) < 128
                    for index, value in enumerate(signal)
                )
            start = count // 2
            omega = 2 * math.pi * 1000 * (channel + 1) / rate
            real = sum(value * math.cos(omega * index)
                       for index, value in enumerate(signal[start:], start))
            imag = sum(value * math.sin(omega * index)
                       for index, value in enumerate(signal[start:], start))
            amplitude = 2 * math.hypot(real, imag) / (count - start) / (1 << 31)
            assert 0.8 < amplitude < 1.0


@pytest.mark.main
def test_ssrc_example_rejects_invalid_rate(example_executable, tmp_path):
    result = subprocess.run(
        ["xsim", "--args", str(example_executable), "123", "48000"],
        cwd=tmp_path, capture_output=True, text=True, timeout=30,
    )
    assert result.returncode != 0
    assert "xsim --args bin/app_ssrc.xe" in result.stdout
    assert not (tmp_path / "input.wav").exists()
    assert not (tmp_path / "output.wav").exists()


@pytest.mark.main
def test_ssrc_example_default_rates(example_executable, tmp_path):
    result = subprocess.run(
        ["xsim", str(example_executable)],
        cwd=tmp_path, capture_output=True, text=True, timeout=240,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    timing = re.search(
        r"Max ssrc_process time: (\d+)\.(\d{3}) us \((\d+) ticks at (\d+) Hz\); "
        r"(\d+) input samples and (\d+) output samples per channel, (\d+) channels",
        result.stdout,
    )
    assert timing is not None, result.stdout
    microseconds, fraction, ticks, frequency, inputs, outputs, channels = map(int, timing.groups())
    assert ticks > 0 and frequency > 0
    assert microseconds * 1000 + fraction == ticks * 1_000_000_000 // frequency
    assert inputs == 4 and 0 < outputs <= 20 and channels == 4
    for name, rate in (("input", 44100), ("output", 96000)):
        with wave.open(str(tmp_path / f"{name}.wav"), "rb") as wav:
            assert wav.getframerate() == rate
