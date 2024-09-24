ReadMe for OS3
==============

23.05.2016 - T.Hb / Digimath for XMOS 

This is the readme file for the OS3 reference application.


Introduction
------------
The OS3 is a standalone application performing 3x synchronous over-sampling. It uses a 144 taps polyphase FIR filter with 3 phases. It takes file input and provides file output.

The OS3 application is based on a pull interface, where the processing function (FIROS3_proc()) is called for every desired output sample. The example code is designed for MS Windows WC Pro 2008 but should be easily transferrable to other platforms.

A phase counter in the control structure (iPhase member of the FIROS3 control structure) is used to define which output phase is to be computed. Once this phase counter reaches 0, a new input sample must be reloaded (this happens every third sample and is done using the FIROS3_input() function which is very short). Hence load is approximately constant for each output sample.


Deliverables
------------
- OS3: top level folder
	- ReadMe.txt: this file
	
- OS3/TestStreams: contains utilities to generate input files
	- s1k_0db_16.dat: Input test signal (1k sine at 0dB, Fs = 16kHz)
	- im5k6k_m6dB_16.dat: Input test signal (5k+6k sines at -6dB, Fs = 16kHz)
	- s1k_0db_48.dat: Output test signal (1k sine at 0dB, Fs = 48kHz)
	- im5k6k_m6dB_48.dat: Output test signal (5k+6k sines at -6dB, Fs = 48kHz)
	- s1k_0dB_48.jpg: FFT of Output test signal (1k sine at 0dB, Fs = 48kHz)
	- im5k6k_m6dB_48.jp: FFT of Output test signal (5k+6k sines at -6dB, Fs = 48kHz)

- OS3/OS3: contains the source and project files

- OS3/OS3/FilterData: contains the filter coefficients and projects (ScopeDSP). Note that coefficients are in the format to be used by OS3 (see Filter coefficients storage later in this document).


Usage:
------
The OS3 application has following command flags:

OS3.exe -i<Input File Name> -o<Output File Name>

Example (these command lines run the example on the two provided input test streams):
OS3.exe -is1k_0dB_16.dat -os1k_0dB_48.dat
OS3.exe -iim5k6k_m6dB_16.dat -oim5k6k_m6dB_48.dat


API
---
The OS3 follows an API similar to DS3, SSRC and ASRC. It is however a pull interface (i.e. where the processing is driven by requests for output samples). The functions are the following:

- FIROS3_init(): initialization function to be called prior to using the OS3

- FIROS3_sync(): synchronization function to clear delay line and reset index pointers and phase counter

- FIROS3_proc(): processing function to be called for each desired output sample. The new output sample is returned by the iOut member of the FIROS3 control struture.

- FIROS3_input(): function to write a new input sample to the FIROS3 filter delay line. This function should be called prior to FIROS3_proc() whenever the iPhase member of the FIROS3 control structure is 0. 


Filters
-------
The OS3 application uses a 3 phases 144 tap polyphase filter (hence each phase is 48 samples).

The OS3 application comes with two filters which can be selected in FIROS3.h using the FIROS3_COEFS_FILE #define:

- Both filters have an out-of-band attenuation of 130dB and an in-band ripple of 0.015dB.
- The first filter has a transition band from 6.85kHz to 8.74kHz
- The second filter has a transition band from 6.3kHz to 8.21kHz

Digimath recommends using the first filter as it has a wider passband, but both are perfectly OK and the effective choice shall remain with XMOS (or its customers).


Filter coefficients storage
---------------------------
Filter coefficients are stored in 32bits integer form, with integrated scaling by a factor 3 (to compensate for the over-sampling rate). They are stored in time reversed order with phases grouped. As an example:

Let H = H(0), ... , H(143) be the coefficients computed by a filter coefficients computing software, expressed as floating point (i.e. range -1 to +1)

To get an integer representation (with scaling factor 3 for 3 x over-sampling), we multiply the coefficients by 3 x 2^31. Hence we get G = G(0), .... G(143) where G(i) = 3 x 2'31 x H(i) as integer.

The coefficients for the OS3 are stored as follows (each group corresponding to a given phase):

Coefs = {
	G(141),
	G(138),
	G(135),
	...
	G(3),
	G(0),

	G(142),
	G(139),
	G(136),
	...
	G(4),
	G(1),

	G(143),
	G(140),
	G(137),
	...
	G(5),
	G(2)
};

This order allows subsequent coefficients to be used (without jumps in the table) and allows for coefficients reads using DUAL_READ instruction to optimize MIPS usage.





