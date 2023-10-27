INTRODUCTION
============

This folder includes a python script which runs the C emulators for the ASRC, SSRC, OS3 and DS3 modules.
For each model test signals are injected - both a single tone to test the THD performance and a range of tones to
observe roll-off at the top of the band. Using an un-windowed FFT the SNR and THD is calculated.
The logs are scraped to extract MIPS and utilization where supported.

Tests are run for all suported input and output sample freqencies, and for worst case frequency deviation.
Note that not all modules support all input scenarios due to input and output rate specifications - these cases are ignored.

The result of this script is the generation an `rst` file which is included into the documentation,
incorporating a full set of charts and summary table.



REQUIREMENTS
============

Python 3.9 or above. Make sure the dependencies listed in lib_src/requirements.txt are installed.

	cd ../../
	pip install -r requirements.txt



To run the script:

	python3 doc_asrc.py


By default, this will create a "_build" folder, containing sub-folders:
	./input		all generated test files
	./expected	simulation output from the models
	./output	plots and a csv format log file
	./rst		a single rst file which incorporates all the plots and log file as a table
