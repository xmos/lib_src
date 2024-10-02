# Copyright 2023-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Wrapper to run the legacy ff3 voice tests so we get a nice test report in Jenkins
"""

import pytest
import subprocess
from pathlib import Path
from utils.src_test_utils import build_firmware_xcommon_cmake

@pytest.mark.prepare
@pytest.mark.parametrize("testname", ["ds3_voice_test", "us3_voice_test", "unity_gain_voice_test"])
def test_prepare(testname):
    """ Build firmware """
    build_firmware_xcommon_cmake(Path(__file__).parent / testname)


@pytest.mark.main
@pytest.mark.parametrize("voice_test", ["ds3_voice_test", "us3_voice_test", "unity_gain_voice_test"])
@pytest.mark.parametrize("arch", ["xs2"])
def test_voice_ff3_xs2(voice_test, arch):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """

    file_dir = Path(__file__).parent
    xe = file_dir / voice_test / "bin" / arch / f"{voice_test}_{arch}.xe"
    cmd = f"xsim {xe}"

    print(f"Running: {cmd}")
    output = subprocess.run(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    print(output.stdout)
    assert output.returncode == 0
