#!/usr/bin/env python
import xmostest, sys, subprocess

if __name__ == "__main__":
    if sys.platform.startswith("darwin"):
        p = subprocess.Popen("ulimit -n 1024", shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for line in p.stdout.readlines():
            print line,
        retval = p.wait()
        #OSX has a very low limit on open files - 256. Each spawn of xsim consumes about 5 so to run the 144 tests in this repo, we need at least 720 spare
    xmostest.init()
    xmostest.runtests()
    xmostest.finish()
