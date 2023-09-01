# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Wrapper to run the legacy build test so we get a nice test report in Jenkins
"""

import pytest
import subprocess
from pathlib import Path

def test_legacy_build():

    cmd = f"xmake"
  
    print(f"Running: {cmd}")
    output = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd="legacy_build_test")
    print(output.stderr)

    assert output.returncode == 0
