# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import pytest
from src_test_utils import generate_mips_report

def pytest_sessionfinish(session, exitstatus):
    """
    Called after whole test run finished, right before
    returning the exit status to the system.
    """
    # Ensure we play nicely with xdist
    if not hasattr(session.config, "workerinput"):
        pass

    for mips_report_type in ["ssrc", "asrc"]:
        generate_mips_report(mips_report_type)
