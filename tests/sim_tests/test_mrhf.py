# Copyright 2016-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Test verify the operation of both SSRC and ASRC (which belong to the multi-rate HiFi SRC collection)
"""

import pytest
from utils.src_test_utils import gen_golden, run_dut, build_firmware_xcommon_cmake, build_host_app_xcommon_cmake
from pathlib import Path
import os
import subprocess


NUM_SAMPLES_TO_PROCESS = 256
SR_LIST = (44100, 48000, 88200, 96000, 176400, 192000)
ASRC_DEVIATIONS = ("1.000000", "0.990099", "1.009999")


def get_test_architecture(src_type):
    env_var = f"{src_type.upper()}_TEST_PROFILE"
    architecture = os.environ.get(env_var, "xs3a")
    if architecture not in ("xs3a", "vx4b"):
        pytest.fail(f"Unsupported {env_var}: {architecture}")
    return architecture


@pytest.mark.prepare
@pytest.mark.parametrize("src_type", ["ssrc", "asrc"])
def test_prepare(src_type):
    """ Builds firmware and host reference and generates the golden reference signals on the host """
    host_app = build_host_app_xcommon_cmake(Path(__file__).parent / f"{src_type}_test")
    if get_test_architecture(src_type) == "xs3a":
        build_firmware_xcommon_cmake(Path(__file__).parent / f"{src_type}_test")
    gen_golden(host_app, src_type, NUM_SAMPLES_TO_PROCESS, SR_LIST, SR_LIST, ASRC_DEVIATIONS if src_type == "asrc" else None)


@pytest.mark.prepare
def test_prepare_asrc_profile_selftest():
    """Build the XS3 VPU self-test firmware; VX4 is built by its Slipgate stage."""
    profile = os.environ.get("ASRC_TEST_PROFILE", "xs3a")
    if profile == "xs3a":
        build_firmware_xcommon_cmake(Path(__file__).parent / "asrc_vpu_test")
    elif profile != "vx4b":
        pytest.fail(f"Unsupported ASRC_TEST_PROFILE: {profile}")


@pytest.mark.main
@pytest.mark.parametrize("profile,compare_mode", [("scalar", None), ("vpu", "abs_diff")])
@pytest.mark.parametrize("sr_out", SR_LIST)
@pytest.mark.parametrize("sr_in", SR_LIST)
def test_ssrc(sr_in, sr_out, profile, compare_mode, request):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """
    file_path = Path(__file__).parent
    xe = file_path / "ssrc_test" / "bin" / profile / f"ssrc_test_{profile}.xe"
    architecture = get_test_architecture("ssrc")
    mips = run_dut(xe, sr_in, sr_out, "ssrc", NUM_SAMPLES_TO_PROCESS, compare_mode=compare_mode, profile=profile)
    request.node.user_properties.append(("src_mips", ("ssrc", architecture, sr_in, sr_out, profile, mips)))


@pytest.mark.main
@pytest.mark.parametrize("profile,compare_mode", [("scalar", None), ("vpu", "abs_diff")])
@pytest.mark.parametrize("sr_out", SR_LIST)
@pytest.mark.parametrize("sr_in", SR_LIST)
@pytest.mark.parametrize("fs_deviation", ASRC_DEVIATIONS)
def test_asrc(sr_in, sr_out, fs_deviation, profile, compare_mode, request):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """
    file_path = Path(__file__).parent
    xe = file_path / "asrc_test" / "bin" / profile / f"asrc_test_{profile}.xe"
    architecture = get_test_architecture("asrc")
    mips = run_dut(xe, sr_in, sr_out, "asrc", NUM_SAMPLES_TO_PROCESS, fs_deviation=fs_deviation, compare_mode=compare_mode, profile=profile)
    request.node.user_properties.append(("src_mips", ("asrc", architecture, sr_in, sr_out, fs_deviation, profile, mips)))


@pytest.mark.main
def test_asrc_profile_selftest():
    """Run the architecture-specific ASRC self-test firmware under xsim."""
    profile = os.environ.get("ASRC_TEST_PROFILE", "xs3a")
    firmware_names = {
        "xs3a": "asrc_vpu_test",
        "vx4b": "asrc_vpu_test",
    }
    firmware_name = firmware_names.get(profile)
    assert firmware_name is not None, f"Unsupported ASRC_TEST_PROFILE: {profile}"

    xe = Path(__file__).parent / firmware_name / "bin" / f"{firmware_name}.xe"
    result = subprocess.run(["xsim", str(xe)], capture_output=True, text=True, timeout=900)
    assert result.returncode == 0, (
        f"{profile} ASRC self-test exited {result.returncode} for {xe}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
