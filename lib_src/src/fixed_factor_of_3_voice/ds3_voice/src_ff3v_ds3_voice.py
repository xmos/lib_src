import numpy as np
from scipy import signal
import matplotlib.pyplot as plt


def plot_response(fs, w, h, title):
    plt.figure()
    plt.plot(0.5*fs*w/np.pi, 20*np.log10(np.abs(h)))

    plt.xlim(0, 0.5*fs)
    plt.grid(True)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Gain (dB)')
    plt.title(title)

def plot_response_passband(fs, w, h, title):
    plt.figure()
    plt.plot(0.5*fs*w/np.pi, 20*np.log10(np.abs(h)))
    plt.ylim(-1, 1)
    plt.xlim(0, 0.5*fs)
    plt.grid(True)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Gain (dB)')
    plt.title(title)


# Low-pass filter design parameters
fs = 48000.0        # Sample rate, Hz
numtaps = 12*2*3    # Size of the FIR filter.

taps = signal.remez(numtaps, [0, 7300, 8700, 0.5*fs], [1, 0], [.008, 1], Hz=fs)
w, h = signal.freqz(taps)
#plot_response(fs, w, h, "Low-pass Filter")
#plot_response_passband(fs, w, h, "Low-pass Filter")

taps = taps / sum(abs(taps))    # Guarantee no overflow

print 'int32_t src_ff3v_ds3_voice_coefs_debug[72] = {'
for c in taps:
    c = int(c*(2**31 - 1))
    print str(c) + ', '
print '};'

print 'const int32_t src_ff3v_ds3_voice_coefs[3][24] = {'
for step in range(2, -1, -1):
    print '\t{'
    for i in range(step, len(taps), 3):
        c = int(taps[i]*(2**31 - 1))
        print '\t' + str(c) + ','
    print '\t},'
print '};'
