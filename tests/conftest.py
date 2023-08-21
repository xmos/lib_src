import pytest
import subprocess
import re

def pytest_sessionfinish(session, exitstatus):
    """
    Called after whole test run finished, right before
    returning the exit status to the system.
    """
    # Ensure we play nicely with xdist
    if not hasattr(session.config, "workerinput"):
        pass

    mips_report_filename = "mips_report.csv"
    with open(mips_report_filename, "w") as mips_report:
        mips_report.write("input_sr, output_sr, MIPS\n")
        file_list = subprocess.run('find . -iname "max_mips.txt"', shell=True, capture_output=True, text=True).stdout
        print(file_list.split())
        for mips_file in file_list.split():
            with open(mips_file) as mf:
                vals = re.search(r'(\d+)->(\d+):([\d.]+)', mf.readlines()[0])
                mips_report.write(f"{vals.group(1)}, {vals.group(2)}, {vals.group(3)}\n")