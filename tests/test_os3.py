# Copyright 2016-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import xmostest

def runtest():
    resources = xmostest.request_resource("xsim")

    tester = xmostest.ComparisonTester(open('os3_test.expect'),
                                       'lib_src', 'fixed_factor_of_3_tests',
                                       'app_os3', {})

    xmostest.run_on_simulator(resources['xsim'],
                              '../examples/app_os3/bin/app_os3.xe',
                              tester=tester)
