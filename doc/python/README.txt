INTRODUCTION
============

This folder includes a python script which runs the C emulators for the ASRC, SSRC, OS3 and DS3 modules.
For each model we present test signals - both a single tone to test the THD performance and a range of tones to 
observe roll-off at the top of the band.
Using an un-windowed FFT we calculate the SNR.
We scrape the logs to extract MIPS/Utilization where supported.

Tests are run for all suported input and output sample freqencies, and for worst case frequency deviation.
Note that not all modules support all input scenarios - these are ignored.

The result of this script is the generation an rst file which is inclorporated into the documentation,
incorporating a full set of charts and summary table.



HOW TO
======

This has been developed and tested under WSL2 linux for Windows.
In a WSL2 command window (e.g., Ubuntu20.04):


To create and setup the conda environment to run the asrc_test.py script

	conda env create -f condaEnv.yml
	conda activate .conda-env
	


To run the script:

	python3 doc_asrc.py
	
	
By default, this will create a "_build" folder, containing sub-folders:
	./input		all generated test files
	./expected	simulation output from the models
	./output	plots and a csv format log file
	./rst		a single rst file which incorporates all the plots and log file as a table