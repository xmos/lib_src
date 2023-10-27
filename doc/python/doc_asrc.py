# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from asrc_utils import asrc_util
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

################################################################################################################
# OVERVIEW
#
# This script generates a set of test files for the supported input and output rates.
# It passes these through the golden reference "C" models, and
# Plots the FFTs, and extracts the SNR and THD.
# All this info is annotated on the Plot which is saved to the output folder.
#
# Note that both the C model is a dual-channel implementations, so we pass them a pair of source files
# and since this is ASRC, we vary the frequency deviation parameter fDev.
# But, OS3 and DS3 are single channel apps - which is dealt with!
#
################################################################################################################


# Setup some basics
pkg_dir = Path(__file__).parent
xsim = False
fftPoints=1024 # note when updating the test tones with "updateSig" this is overwritten
FERR = 1.0  #this is an additional sample rate deviation applied to the inpt signal to test effect of errors

U = asrc_util(pkg_dir, True, fftPoints,[200], [500]) # create the util object
U.addRSTText(".. _performance-plots:")
U.addRSTHeader("Performance Characterisation for SSRC ASRC DS3 and OS3", 1) # start the RST it generates with a title
IntroText = """
The FFT plots in this section provide a visual guide to the performance of the SSRC, ASRC, DS3 and OS3 sample rate converters.
Test signals were created allowing analysis of the sample rate converter output across different scenarios.

Two input signals were played through a stereo sample rate converter across a range of input and output sample rates.
For Channel 0, a single pure tone was generated ensuring its frequency was well within the overall nyquist rate.
For Channel 1, multiple tones spaced logarithmically were generated with the spacing most dense at higher frequencies.

The resulting frequency plot output clearly shows the noise floor relative to the sample rate converted injected tone(s).
The plots are annotated with an estimate of the Signal to Noise Ratio (SNR) as well as Total Harmonic Distortion (THD).

For the case of the ASRC, in addition to the nominal input frequency of 0 PPM deviation, the +/-100 PPM frequency deviation cases are also shown.
"""
U.addRSTText(IntroText)
U.addRSTHeader("Pure Tone FFT SRC Plots Across Sample Rate Combinations", 2) # start the RST it generates with a title

for fDev in [0.9999, 1.0, 1.0001]:  # for a set of different frequency deviations
    U.addRSTHeader("Frequency error: {:.6f}Hz".format(fDev), 3) #add a title to the RST for the freq deviation

    for opRate in U.allRates: # for each of the possible output sample rates
        U.addRSTHeader("Output Fs : {:,d}Hz".format(U.sampleRates[opRate]), 4) # add a subtitle to RST for the opRate

        # Choose the o/p rate and freq deviation
        U.setOpRate(opRate, fDev)

        no_results = True # the RST contains headings for all fDev, opRate and ipRate - so if this is an unsupported combination, add a warning to thr RST.
        for ipRate in U.allRates:# for each of the possible input sample rates
            # opportunity to choose different test freq, which also sets a safer fft size
            U.updateSig(ipRate, [int(fftPoints/5)], [int(fftPoints/4),int(fftPoints/6)], True) # specified in terms of FFT o/p bin, when flag set true it auto-fills a logarithmic range for ch1 and 5% in for ch0

            # Make a set of input files based on range of sample rates supported
            ipFiles, sig = U.makeInputFiles([ipRate], FERR)   # makes the signals, saves data as files and returns some info about them

            # Put the input data through the golden "C" emulators
            opFiles, simLog = U.run_c_model(ipFiles, opRate, 4, fDev)

            # Iterate over all the input data files and channels
            for channels in ipFiles: # For each input sata set there will be an output one for each channel
                for channel in channels[0:2]:
                    print(channel)

                    if len(simLog[channel]) > 0: #at least one simulation ran for this combination of iprate, oprate and fdev
                        no_results = False
                        for sim in simLog[channel]:
                            opData = U.loadDataFromOutputFile(simLog[channel][sim])

                            # fft
                            opfft = U.rawFFT(opData['samples'])
                            oplabel = opData['labels']

                            # Get MHz utilization info from the log
                            info = U.scrapeCLog(opData)

                            # Calculate the SNR for each fft
                            opSNR = U.calcSNR(opfft, opData['chan'])

                            # Calculate the THD for each fft
                            opTHD = U.calcTHD(opfft, opData['chan'])

                            # Keep the results to plot later
                            text="SNR:{:.1f}dB\nTHD:{}dB\n".format(opSNR, opTHD)
                            U.pushPlotInfo(opfft, oplabel, text)

                            # Update log with these results
                            U.logResults(sim, ipRate, opRate, fDev, opData['chan'], opSNR, opTHD, info['total_mips'], info['ch0_mips'], info['ch1_mips'], info['log'])


                        #Plot the results
                        plotFile = U.plotFFT(U.plot_data, combine=False, title=U.makePlotTitle(simLog, channel), subtitles=U.plot_label, log=True, text=U.plot_text) # plots a grid of charts, one per simulation model
                        U.makeRST(plotFile, ipRate, opRate, fDev, simLog[channel].keys()) # add this plot to a list to save as an RST file later
                        U.resetPlotInfo() # otherwise the next iteration of rates etc will add more plots to this instead of starting a new plot.
        if no_results:
            U.addRSTText("No SRC available for this scenario.")

U.log2csv() # Save the log file
U.addRSTHeader("Tabulated data", 2) # start the RST it generates with a title
U.addLog2RST() # adds the log, which is the tabulated results, to the RST file
U.saveRST("allPlots.rst") # Save the log file to the output folder

print("Done")
quit()
