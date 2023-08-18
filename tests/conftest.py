def pytest_sessionfinish(session, exitstatus):
    """
    Called after whole test run finished, right before
    returning the exit status to the system.
    """
    # Ensure we play nicely with xdist
    if not hasattr(session.config, "workerinput"):
        pass

    mips_report_filename = "mips_report.csv"
    with open(mips_report_filename, "a") as mips_file:
        mips_file.write("hellord\n")