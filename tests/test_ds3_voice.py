# Copyright (c) 2016-2019, XMOS Ltd, All rights reserved
import xmostest

def runtest():
    resources = xmostest.request_resource("xsim")

    tester = xmostest.ComparisonTester(open('ds3_voice_test.expect'),
                                       'lib_src', 'fixed_factor_of_3_tests',
                                       'app_ds3_voice', {})

    xmostest.run_on_simulator(resources['xsim'],
                              '../examples/app_ds3_voice/bin/app_ds3_voice.xe',
                              tester=tester)
