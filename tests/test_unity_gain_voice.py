import xmostest

def runtest():
    resources = xmostest.request_resource("xsim")

    tester = xmostest.ComparisonTester(open('unity_gain_voice_test.expect'),
                                       'lib_src', 'fixed_factor_of_3_tests',
                                       'app_unity_gain_voice', {})

    xmostest.run_on_simulator(resources['xsim'],
                              '../examples/app_unity_gain_voice/bin/app_unity_gain_voice.xe',
                              tester=tester)
