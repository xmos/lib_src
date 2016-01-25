#!/usr/bin/env python
import xmostest

if __name__ == "__main__":
    xmostest.init()
    
    xmostest.register_group("lib_src",
                           "ssrc_tests",
                           "Basic SSRC test 44.1KHz to 192KHz and 176.4KHz to 48KHz",
                           "This test is run on the simulator using a 1KHz sine full scale input and compares the output with an expected set of data. The two tests (out of a possible 36) exercise six of the thirteen filters and so provide a basic smoke test of the datapath and FIR filter functions")
    xmostest.register_group("lib_src",
                            "asrc_tests",
                            "Basic asrc test 44.1KHz to 192KHz and 176.4KHz to 48KHz using a 1:1 ratio",
                            "This test is run on the simulator using a 1KHz sine full scale input and compares the output with an expected set of data. The two tests (out of a possible 36) exercise five of the twelve filters and so provide a basic smoke test of the datapath and FIR filter functions")
    
    xmostest.runtests()
    xmostest.finish()
