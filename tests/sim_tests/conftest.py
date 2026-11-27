# Copyright 2023-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from collections import defaultdict
from utils.src_test_utils import generate_mips_report


class MipsReporter:
    def __init__(self):
        self.results = defaultdict(list)

    def pytest_runtest_logreport(self, report):
        if report.when == "call" and report.passed:
            for name, value in report.user_properties:
                if name == "src_mips":
                    src_type, architecture, *row = value
                    self.results[src_type, architecture].append(row)

    def pytest_sessionfinish(self):
        for (src_type, architecture), rows in self.results.items():
            generate_mips_report(src_type, architecture, rows)


def pytest_configure(config):
    if not hasattr(config, "workerinput"):
        config.pluginmanager.register(MipsReporter())
