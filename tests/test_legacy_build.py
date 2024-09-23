# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

"""
Wrapper to run the legacy build test so we get a nice test report in Jenkins
"""

import pytest
import subprocess
from pathlib import Path

def test_legacy_build():
    file_dir = Path(__file__).parent
    test_dir = file_dir / "legacy_build_test"
    assert test_dir.exists(), f"test directory {test_dir} doesn't exist"
    build_dir = test_dir / "build"
    cmd = f"cmake -B {build_dir} -S {test_dir}"

    print(f"Running: {cmd}")
    output = subprocess.run(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, cwd=test_dir)
    print(output.stdout)
    assert output.returncode == 0

    cmd = f"xmake -j -C {build_dir}"
    output = subprocess.run(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, cwd=test_dir)
    print(output.stdout)
    assert output.returncode == 0
