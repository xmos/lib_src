# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Wrapper to run the legacy ff3 voice tests so we get a nice test report in Jenkins
"""

import pytest
import subprocess
from pathlib import Path

@pytest.mark.parametrize("voice_test", ["ds3_voice", "us3_voice", "unity_gain_voice"])
def test_voice_ff3_xs2(voice_test):
    """ Runs the signal through the simulator via an xcore test app and compares to golden ref """

    file_dir = Path(__file__).resolve().parent
    cmd = f"xsim {str(file_dir)}/../build/tests/{voice_test}_test/test_{voice_test}.xe"
  
    print(f"Running: {cmd}")
    output = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    print(output.stdout)
    assert output.returncode == 0
