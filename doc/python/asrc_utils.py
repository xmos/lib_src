# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from subprocess import Popen, PIPE
from subprocess import run as srun
import os
import shutil
import matplotlib.pyplot as plt
from scipy.fft import fft, ifft
import scipy.signal as signal
import numpy as np
from scipy.io import wavfile
import re
from math import gcd
from mpmath import mp
from scipy.io.wavfile import write as writewav
from pathlib import Path
import subprocess


class asrc_util:
    def __init__(self, path, relative, fftPoints, ch0_bins, ch1_bins):
        self.path = str(path)
        self.relative = relative

        # locations for results - note they are purged every time this is run
        self.cleanFolder("_build") # create temp location for results
        self.outputFolder = self.cleanFolder("_build/output") # put all results of simulation and plots here
        self.inputFolder = self.cleanFolder("_build/input") # put any input test files here
        self.expectedFolder = self.cleanFolder("_build/expected") # put C ref model o/p here
        self.rstFolder = self.cleanFolder("_build/rst") # put RST o/p here

        self.logFile = "/log.csv" # tabulated results

        # the C-models and XSIM models which get run
        self.asrc_model = self.build_model_exe("asrc")
        self.ssrc_model = self.build_model_exe("ssrc")
        self.ds3_model = self.build_model_exe("ds3")
        self.os3_model = self.build_model_exe("os3")

        # definitions of sample ratea
        self.allRates =["16", "32", "44", "48", "88", "96", "176", "192"]
        self.srcRates =["44", "48", "88", "96", "176", "192"] # supported by the asrc and ssrc
        self.factors = {"44":0, "48":1, "88":2, "96":3, "176":4, "192":5} # look up to convert rate to the filter bank id needed by the ASRC
        self.sampleRates = {"16":16000, "32":32000, "44":44100, "48":48000, "88":88200, "96":96000, "176":176400, "192":192000} #convenience to save typing out sample rates in full
        self.sigMax = {"16":7300, "32":14600, "44":18000, "48":21800, "88":40000, "96":42000, "176":80000, "192":85000} # upper limit on input freq for given sample rate. Note that these need to be well within Nyquist point to prevent aliasing.

        self.numSamples = {} #populated later based on op-rate to ensure sufficient samples for fft
        self.ignoreSamples = 2000 #worst case for high up-sampling
        self.fftPoints = fftPoints

        self.sig = [ch0_bins, ch1_bins] # defines the test signals for each of 2 channels, each is a list so more than one tone can be generated and combined
        self.log=[]
        self.plots=[]
        mp.prec = 1024
        self.fudge = 0.5**128
        self.plot_data = []
        self.plot_label = []
        self.plot_text = []
        self.skip_xsim = True # FIX THIS LATER
        self.rstFile = ""

    def build_model_exe(self, target):
        file_dir = Path(__file__).resolve().parent
        build_path = file_dir / "../../build"
        build_path.mkdir(exist_ok=True)
        subprocess.run("rm -rf CMakeCache.txt CMakeFiles/", shell=True, cwd=str(build_path))
        subprocess.run("cmake  ..", shell=True, cwd=str(build_path))
        bin_path = file_dir / f"{target}_/model"
        subprocess.run(f"make {target}_golden",  shell=True, cwd=str(build_path))

        return f"{build_path}/tests/{target}_test/{target}_golden"



    # update the input frequencies
    # allows input of frequencies as a list for each channel, or use the autoFill which
    # populates channel 1 with mutiple tones, spaced logrithmically, most dense at higher freq, and ch0 is always 10% in
    # set in 'sigMax' beyond which reflections degrade the SNR.
    def updateSig(self, ipRate, ch0_bins, ch1_bins, autoFill=False):
        # this manipulates the fft size to attempt to force any reflections around the ip sample rate nyquist also ending up
        # in an integer fft bin
        self.fftPoints = int(10000 *(self.sampleRates[ipRate] * self.fDev) / self.sampleRates[self.opRate])
        print(f"Over-wrote the fft size to {self.fftPoints} points. ipRate = {ipRate}")

        #for channel 0
        ch0_ratio = 0.05 # 5% of the way into the fft bins, so 10% on the plot
        ch0bin = int(self.fftPoints * ch0_ratio)
        print("For channel 0: Creating test tone at FFT bin: {}".format(ch0bin))
        # for channel 1
        sigMax = self.sigMax  # upper limit on ip frequency to be safely inside the filter cutoff
        maxBin = int((min(sigMax[ipRate], sigMax[self.opRate]) / (self.sampleRates[self.opRate])) * self.fftPoints)
        i=1
        r=2 # spread factor
        ch1_range = [maxBin]
        while maxBin-r**i > 0: # this bit spreads the bins out exponentially, concentrated at the top
            ch1_range.append(int(maxBin-r**i))
            i+=1
        print("For channel 1: Creating data set for ipRate {} and opRate {} which resulted in\n{}\n".format(ipRate, self.opRate, ch1_range ))
        if autoFill:
            self.sig = [[ch0bin], ch1_range]
        else:
            self.sig = [ch0_bins, ch1_bins]



    # handy util to add data into an array
    def pushPlotInfo(self, a,b,c):
        self.plot_data.append(a)
        self.plot_label.append(b)
        self.plot_text.append(c)

    def resetPlotInfo(self):
        self.plot_data = []
        self.plot_label = []
        self.plot_text = []

    def makePlotTitle(self, simLog, channel):
        firstKey = list(simLog[channel].keys())[0]
        title = simLog[channel][firstKey][0].replace(self.expectedFolder, '')[1:]
        title = title.split('_')[:-1]
        for k in simLog[channel]:  title.append(k)
        title = "_".join(title)
        return title


    def logResults(self, source, ipRate, opRate, fDev, ch, SNR, THD, totalmips, ch0mips=None, ch1mips=None, txt=None, snr2=None, mips2=None):
        # simple function to append information to a log, which is kept as an array of dictionaries
        realRate = self.sampleRates[opRate] / self.fDev
        sigList = []
        for s in self.sig[ch]:
            sigList.append("{:.3f}".format(float(mp.fmul(realRate ,mp.fdiv(s, self.fftPoints)))))
        signals = ";".join(sigList)
        #info = {"source":source, "ipRate(Hz)":self.sampleRates[ipRate], "opRate(Hz)":self.sampleRates[opRate], "fDev":fDev, "ch":ch, "signals(Hz)":signals, "SNR(dB)":SNR, "THD(dB)":THD,"Total MIPS":totalmips, "MIPS(ch0)":ch0mips, "MIPS(ch1)":ch1mips, "Text":txt}
        info = {"source":source, "ipRate(Hz)":str(self.sampleRates[ipRate]), "opRate(Hz)":str(self.sampleRates[opRate]), "fDev":str(fDev), "ch":str(ch), "signals(Hz)":signals, "SNR(dB)":"{:.1f}".format(SNR), "THD(dB)":THD}
        self.log.append(info)


    def log2csv(self):
        # export the log array as a csv file, using the keys as headings
        logfile = open(self.outputFolder + self.logFile, "w")
        logfile.write(",".join(self.log[0].keys()) + "\n") # write headings
        for i in self.log:
            logfile.write((','.join(i.values())).replace("\n", ";") + "\n") # write data
        logfile.close()


    def setOpRate(self, opRate, fDev):
        self.opRate = opRate
        self.fDev = fDev

    def cleanFolder(self, folder):
        # utility to delete and recreate folders used for input and output files
        p = "{}/{}".format(self.path, folder)
        if os.path.isdir(p):
            shutil.rmtree(p)
        os.mkdir(p)
        if self.relative:
            p = p.replace(self.path, ".")
        return p

    def listFactors(self, x):
        factors=[]
        for i in range(1,x+1):
            if x%i==0:
                factors.append(i)
        return factors

    def makeSignal(self, fsamp, fsig, asig, l, ferr=1.0):
        # populates ipdata with sinewave
        # fsamp: sample rate, as abbreviated string (e.g., "48", interpreted as 48,000Hz)
        # fsig: signal freq, in Hz
        # asig: amplitude, in range 0-1
        # l: number of samples
        fsamphz = mp.fmul(mp.fmul(float(self.sampleRates[fsamp]), self.fDev), ferr)
        f = mp.fdiv(fsig, fsamphz)
        print("Make signal: {:.2f}".format(float(fsig)))
        a = asig * 2**31
        x = np.linspace(0, float(2*mp.pi*f*(l-1)), l)
        ipdata = a * np.sin(x)
        return ipdata


    def saveInputFile(self, signals, fileName):
        # sums the signals into one signal and saves as a file
        # signals: an array of arrays [data0[], data[1], ...]
        # return the file with path
        data = np.sum(signals, axis=0).astype('int32')
        file = self.inputFolder + "/" + fileName
        np.savetxt(file, data, fmt='%d', delimiter='\n')
        #writewav("/mnt/c/Users/andrewdewhurst/OneDrive - Xmos/Temp/rawdata/test.wav", 44100, (data[0]/2**16).astype(np.int16))
        return file, data

    def makeInputFiles(self, test_rates, ferr=1.0):
        # create two channels of input
        # define some test signal frequencies w.r.t. op rate
        realRate = self.sampleRates[self.opRate] # mp.fdiv(self.sampleRates[self.opRate], self.fDev)
        #
        ipFiles=np.empty((0,3), str)
        for ipRate in test_rates:
            self.numSamples[ipRate] = int( ((self.sampleRates[ipRate]/self.sampleRates[self.opRate]) * ((self.fftPoints*1.5) + self.ignoreSamples )))
            chan=[]
            chanSigData=[]
            i=0
            for ch in self.sig: #iterates over channels
                sig=[]
                for s in ch:
                    f = mp.fmul(realRate ,mp.fdiv(s, self.fftPoints))
                    data = self.makeSignal(ipRate, f, 0.98/len(ch), self.numSamples[ipRate], ferr)
                    sig.append([data])
                #freqs = "-".join(str(round(realRate * x / self.fftPoints)) for x in ch)[0:32] #crop filename
                if len(ch)==1:
                    freqs = str(round(realRate * ch[0] / self.fftPoints))
                else:
                    freqs = str(round(realRate * ch[0] / self.fftPoints)) + "_to_" + str(round(realRate * ch[-1] / self.fftPoints))
                if ferr==1:
                    filename = "ch{}_{}_fsi{}.dat".format(str(i), freqs, ipRate)
                else:
                    filename = "ch{}_{}_fsi{}_err{}.dat".format(str(i), freqs, ipRate, ferr)
                print("Making {} with F={:.6f} using {} points".format(filename, float(f), self.numSamples[ipRate]))
                sigFile, sigData = self.saveInputFile(sig, filename)
                chan.append(sigFile)
                chanSigData.append(sigData)
                i+=1
            ipFiles = np.append(ipFiles, np.array([[chan[0], chan[1], ipRate]]), axis=0)
        return ipFiles, chanSigData


    def doFFT(self, data, window=False):
        # convenient way to select between fft styles.  Note that the periodic one will need a lot more samples, so
        # use window=True for debuging.
        if window:
            return self.winFFT(data)
        else:
            return self.rawFFT(data)


    def mpAbs(self, data):
        result = np.zeros(len(data))
        for i in range(0, len(data)):
            result[i] = mp.fabs(data[i])
        return result


    def rawFFT(self, data):
        # fft assuming signal is already an integer fraction of the true output rate
        min = np.argmin(np.abs(data[self.ignoreSamples:-(self.fftPoints)])) + self.ignoreSamples #investigating if picking a data set that looks like it is nearly zero crossing at the ends is better
        samples = data[min:min+self.fftPoints]
        l=self.fftPoints
        fftData = self.mpAbs(np.fft.fft(samples))
        fftDataDB = 20 * np.log10((fftData/np.max(fftData)) + self.fudge)
        realRate = self.sampleRates[self.opRate] * float(self.fDev)
        x = np.linspace(0,realRate/2, num=int(l/2) ) / 1000
        return [x, fftDataDB[0:int(l/2)], fftData[0:int(l/2)]/np.max(fftData[0:int(l/2)])]


    def plotFFT(self, xydata, combine=False, title=None, subtitles=None, log=True, text=None):
        # Plot style setup for the FFT plots, labels x asis as KHz and y axis as dB.
        # xydata: an array of datasets, each dataset is a 3 element array containing the xdata array and ydata_dB array and ydata_lin array
        # combine: (optional) forces all plots onto the same chart, otherwise it will create a grid of plots
        # title: (optional) the chart title at the top, common to any subplots
        # subtitles: (optional) an array of subtitles, used for each subplot.
        # log: plots the dB data in the input
        grid = {1:[1,1], 2:[1,2], 3:[1,3], 4:[2,2], 5:[2,3], 6:[2,3]}
        clut = ['lawngreen','deepskyblue','orange','dimgray']
        slut = [(0,(1,1)), (0,(5,1))] if combine else ['solid']
        n = len(xydata)
        if n in grid and not(combine): # preffered layout of multiple subplots
            [prows, pcols] = grid[n]
        else:
            prows = int(1 if combine else int(n/3) + 1)
            pcols = int(1 if combine else np.ceil(n/prows))
        fig = plt.figure(figsize=(6*pcols, 6*prows))
        i = 0
        for xy in xydata:
            if not(combine) or i<1:
                ax = fig.add_subplot(prows, pcols, i+1)
            ax.plot(xy[0], xy[1] if log else xy[2], linestyle=slut[i%len(slut)], color=clut[i%len(clut)], linewidth=1, label=subtitles[i])
            ax.set_xlabel("Frequency (KHz)")
            ax.set_ylabel("dB")
            ax.set_ylim(-250,0)
            ax.grid()
            if text != None:
                tl = np.sum([text[x].count("\n") + 1 for x in range(i)]) if combine else 0 # this stacks up the text comments in the case of combined plots
                ax.text(0.95, 0.95-(0.025*tl), text[i], transform=ax.transAxes, fontsize=6, fontweight='normal', va='top', horizontalalignment='right')
            if subtitles != None and not(combine) and i<=n:
                ax.set_title(subtitles[i], fontsize=8, x=0, y=1.0, ha='left') #subtitle above subplots when not combined
            i += 1
            fig.suptitle(title, fontsize=12, x=0.5, y=0.97, ha='center') # title top centre
        if title != None:
            filename = title + "-combined" if combine else title
        else:
            "-".join(subtitles) + "-combined" if combine else "-".join(subtitles)
        if combine:
            fig.legend(loc='lower left')
        plotFile = "{}/{}.png".format(self.outputFolder, filename)
        plt.savefig(plotFile, dpi=100)
        plt.close()
        return "{}.png".format(filename)


    def opFileName(self, fin, label, fDev, opRate):
        #a simple utility to convert an input file path+name to an output version, appending the frequency deviation
        newName = fin.replace(self.inputFolder, self.outputFolder).replace(".dat", "_fso{}_fdev{:f}_{}.dat".format(opRate, fDev, label))
        return newName

    def exFileName(self, fin, label, fDev, opRate):
        #a simple utility to convert an input file path+name to an output version, appending the frequency deviation
        newName = fin.replace(self.inputFolder, self.expectedFolder).replace(".dat", "_fso{}_fdev{:f}_{}.dat".format(opRate, fDev, label))
        return newName


    def run_c_model(self, ipFiles, opRate, blocksize, fDev):
        # Runs the various simulators across the input files, for the provided opRate and frequency deviation
        # ipFiles: an array of arrays, each sub array in format [Ch0 file name, Ch1 filename, ipRate]
        # generates the output filename based on input and returns another array of files in the same format as the input files.
        # also returns the "simLog" which is a dictionary which links every input file to all available output results for that input,
        # since we can't run all sims on all data (e.g. DS3 needs a factor of 3)
        dither=0
        opFiles=np.empty((0,6), str)
        simLog = {}
        for ipFile in ipFiles:
            simLog[ipFile[0]]={}
            simLog[ipFile[1]]={}

            # ASRC C Model
            # ------------
            if ipFile[2] in self.srcRates and self.opRate in self.srcRates:
                ch0 = self.exFileName(ipFile[0], 'c-asrc', fDev, opRate)
                ch1 = self.exFileName(ipFile[1], 'c-asrc', fDev, opRate)

                cmd = "{} -i{} -j{} -k{} -o{} -p{} -q{} -d{} -l{} -n{} -e{}".format(
                    self.asrc_model, ipFile[0], ipFile[1] , self.factors[ipFile[2]], ch0, ch1, self.factors[opRate], dither, self.numSamples[ipFile[2]], blocksize, fDev)
                p = Popen([cmd], stdin=PIPE, stdout=PIPE, shell=True)
                output, error = p.communicate(input=b'\n')
                opFiles = np.append(opFiles, np.array([[ch0, ch1, opRate, str(fDev), output, "c-asrc"]]), axis=0)
                simLog[ipFile[0]]['asrc']=[str(ch0), 0, opRate, str(fDev), output, "c-asrc"]
                simLog[ipFile[1]]['asrc']=[str(ch1), 1, opRate, str(fDev), output, "c-asrc"]


            # SSRC C Model
            # ------------
            if ipFile[2] in self.srcRates and self.opRate in self.srcRates and fDev==1:
                ch0 = self.exFileName(ipFile[0], 'c-ssrc', fDev, opRate)
                ch1 = self.exFileName(ipFile[1], 'c-ssrc', fDev, opRate)

                cmd = "{} -i{} -j{} -k{} -o{} -p{} -q{} -d{} -l{} -n{}".format(
                    self.ssrc_model, ipFile[0], ipFile[1] , self.factors[ipFile[2]], ch0, ch1, self.factors[opRate], dither, self.numSamples[ipFile[2]], blocksize)
                p = Popen([cmd], stdin=PIPE, stdout=PIPE, shell=True)
                output, error = p.communicate(input=b'\n')
                opFiles = np.append(opFiles, np.array([[ch0, ch1, opRate, "1.0", output, "c-ssrc"]]), axis=0)
                simLog[ipFile[0]]['ssrc']=[str(ch0), 0, opRate, str(fDev), output, "c-ssrc"]
                simLog[ipFile[1]]['ssrc']=[str(ch1), 1, opRate, str(fDev), output, "c-ssrc"]

            # DS3 C Model
            # ------------
            if self.sampleRates[opRate] * 3 == self.sampleRates[ipFile[2]] and fDev==1.0:
                opf=["na", "na"]
                for c in [0,1]: #do both channels seperately
                    if os.path.isfile("./input.dat"): # this only proceses a single channel and assumes an input file called "input.dat" and generates "output.dat"
                        os.remove("./input.dat")
                    os.link(ipFile[c], "./input.dat")
                    opf[c] = self.exFileName(ipFile[c], 'c-ds3', fDev, opRate)
                    cmd = '{}'.format(self.ds3_model)
                    p = Popen([cmd], stdin=PIPE, stdout=PIPE, shell=True)
                    output, error = p.communicate(input=b'\n')
                    shutil.copy("output.dat", opf[c])
                    simLog[ipFile[c]]['ds3']=[str(opf[c]), c, opRate, str(fDev), output, "c-ds3"]
                opFiles = np.append(opFiles, np.array([[opf[0], opf[1], opRate, "1.0", output, "c-ds3"]]), axis=0)
                os.remove("./input.dat")
                os.remove("./output.dat")

            # OS3 C Model
            # ------------
            if self.sampleRates[opRate] == self.sampleRates[ipFile[2]] * 3 and fDev==1.0:
                opf=["na", "na"]
                for c in [0,1]: #do both channels seperately
                    if os.path.isfile("./input.dat"): # this only proceses a single channel and assumes an input file called "input.dat" and generates "output.dat"
                        os.remove("./input.dat")
                    os.link(ipFile[c], "./input.dat")
                    opf[c] = self.exFileName(ipFile[c], 'c-os3', fDev, opRate)
                    cmd = '{}'.format(self.os3_model)
                    p = Popen([cmd], stdin=PIPE, stdout=PIPE, shell=True)
                    output, error = p.communicate(input=b'\n')
                    shutil.copy("output.dat", opf[c])
                    simLog[ipFile[c]]['os3']=[str(opf[c]), c, opRate, str(fDev), output, "c-os3"]
                opFiles = np.append(opFiles, np.array([[opf[0], opf[1], opRate, "1.0", output, "c-os3"]]), axis=0)
                os.remove("./input.dat")
                os.remove("./output.dat")


        return opFiles, simLog

    def scrapeXsimLog(self, data):
        # This bit scrapes the output of xsim and takes the average of all reported CPU utilization
        log = data['log']
        p = re.compile("Thread util=\d*%")
        q = re.compile("\d+")
        u=re.findall(p,log)
        tu=0
        for i in range(0, len(u)):
            tu += int(re.findall(q, u[i])[0])
        tu = tu / len(u)
        return tu #thread utilization %


    def scrapeCLog(self, data):
        # This scrapes the CPU data from the C model
        ch0mips = 0
        ch1mips = 0
        totalmips = 0
        txtmips = ""
        if data['src'] == "c-asrc":
            p = re.compile("MIPS.*?\\\\n")
            m = re.findall(p, str(data['log']))
            ch0mips = float(m[0].replace("\\n", "").split(":")[1].strip())
            ch1mips = float(m[5].replace("\\n", "").split(":")[1].strip())
            totalmips = ch0mips + ch1mips
            txtmips = "\n".join(m)
        if data['src'] == "c-ssrc":
            p = re.compile("MIPS.*?\n")
            m = re.findall(p, str(data['log']).replace("\\n","\n"))
            if len(m) > 0:
                totalmips = float(m[0].split(":")[1].strip())

        return {"log":txtmips, "total_mips":totalmips, "ch0_mips":ch0mips, "ch1_mips":ch1mips}


    def makeLabel(self, item):
        #utility to generate a title / filename to use in plots from an item from a file list which is in the foramt [file1, file2, rate as str]
        #and should work for ipFiles and opFiles
        a = item[0][item[0].rfind("/")+1:]
        label = a[0:a.rfind(".")]
        return label


    def loadDataFromOutputFile(self, file):
        #loads an array of files in the format returned by running 'run_c_model' and returns an array of elements, each of which is a dictionary of [ 2 channels of samples], [2 channels of labels], o/p rate
        ch0 = np.loadtxt(file[0]).astype("int32")
        label = self.makeLabel(file)
        return {'samples':ch0, 'labels':label, 'chan':file[-5], 'rate':file[-4], 'fdev':file[-3], 'log':file[-2], 'src':file[-1]}


    def calcSNR(self, fftdata, ch):
        signal=0
        tmpNoise=np.copy(fftdata[2])
        #calculates the SNR from fft data
        for s in self.sig[ch]: #these are the expected bins in the output
            bin = int(s)
            signal += fftdata[2][bin]
            tmpNoise[bin] = 0
        noise =  np.sum(tmpNoise)
        snr = 20 * np.log10((signal / noise) + self.fudge)
        return snr


    # calculates the THD, but limited to cases where there is PRECISELY one signal tone (i.e., in one fft bin)
    def calcTHD(self, fftdata, ch):
        if len(self.sig[ch]) > 1:
            thd = "NA"
        else:
            sigbin = self.sig[ch][0]
            signal = np.abs(fftdata[2][sigbin]) ** 2
            noisebins = np.arange(int(2*sigbin), (self.fftPoints/2)-1, sigbin).astype(int) # harmonic bins, which are the noise
            noise = 0
            for nb in noisebins:
                noise += np.abs(fftdata[2][int(nb)]) ** 2
            thd = 10*np.log10((noise/signal))
            thd="{:.1f}dB".format(thd) #returned as string so we can pass the "NA" for muti-tone cases
        return thd


    def makeRST(self, file, ipRate, opRate, fDev, sims):
        relFile = os.path.relpath(self.outputFolder, self.rstFolder) + "/" + file
        fsi = "{:,d}Hz".format(self.sampleRates[ipRate])
        fso = "{:,d}Hz".format(self.sampleRates[opRate])
        ferr = "{:f}".format(fDev)
        plots = ", ".join(sims)
        self.rstFile = self.rstFile + "\n\n\n" + ".. figure:: {}".format(relFile)
        self.rstFile = self.rstFile + "\n"     + "   :scale: {}".format("90%")
        self.rstFile = self.rstFile + "\n\n"   + "   Input Fs: {}, Output Fs: {}, Fs error: {}, Results for: {}".format(fsi, fso, ferr, plots)


    def addRSTHeader(self, title, level):
        underline=["=", "+", "-", "."]
        self.rstFile = self.rstFile + "\n\n\n" + title
        self.rstFile = self.rstFile + "\n" + underline[level-1] * len(title) + "\n\n"

    def addRSTText(self, text):
        self.rstFile = self.rstFile + "\n\n" + text


    def addLog2RST(self):
        relFile = os.path.relpath(self.outputFolder, self.rstFolder) + self.logFile
        # source	ipRate(Hz)	opRate(Hz)	fDev	ch	signals(Hz)  	SNR(dB)	THD(dB)  Total MIPS	MIPS(ch0)	MIPS(ch1) Text
        self.rstFile = self.rstFile + "\n\n\n" + ".. csv-table:: Data table"
        self.rstFile = self.rstFile + "\n"     + "  :file: {}".format(relFile)
        self.rstFile = self.rstFile + "\n"     + "  :widths: 8, 9, 9, 9, 6, 14, 9, 9, 9, 9, 9"
        self.rstFile = self.rstFile + "\n"     + "  :header-rows: 1"
        self.rstFile = self.rstFile + "\n"



    def saveRST(self, file):
        rst = open("{}/{}".format(self.rstFolder, file), "w")
        rst.write(self.rstFile)
        rst.close()
