// ===========================================================================
// ===========================================================================
//
// File: Main.c
//
// Main implementation file for the ASRC demonstration program
//
// Target:	MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================


// ===========================================================================
//
// Includes
//
// ===========================================================================

// General includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

// ASRC includes
#include "ASRC_wrapper.h"

// Main file includes
#include "Main.h"

#define		ASRC_N_CHANNELS						1									// Number of channels processed by ASRC instance
#define		ASRC_N_IO_CHANNELS					2									// Number of input / output channels in I/O streams
#define		ASRC_N_IN_OUT_RATIO_MAX				5									// Maximum ratio (as integer) between input and output number of samples
// Parameter values
// ----------------
#define		ASRC_DITHER_ON_OFF_MIN				ASRC_DITHER_OFF
#define		ASRC_DITHER_ON_OFF_MAX				ASRC_DITHER_ON

// ===========================================================================
//
// Defines
//
// ===========================================================================

// General constants
// -----------------
#define			N_TOTAL_IN_SAMPLES_MAX							1048576
#define			N_IN_SAMPLES_MAX								1024

// Default values
// --------------
#define			INPUT_FILE_CHANNEL_0_DEFAULT					"input_ch0.dat"
#define			INPUT_FILE_CHANNEL_1_DEFAULT					"input_ch1.dat"
#define			OUTPUT_FILE_CHANNEL_0_DEFAULT					"output_ch0.dat"
#define			OUTPUT_FILE_CHANNEL_1_DEFAULT					"output_ch1.dat"

#define			INPUT_FS_DEFAULT								ASRC_FS_44
#define			OUTPUT_FS_DEFAULT								ASRC_FS_96
#define			FS_RATIO_DEVIATION_DEFAULT						1.0

#define			N_TOTAL_IN_SAMPLES_DEFAULT						4096
#define			N_IN_SAMPLES_DEFAULT							32

#define			ASRC_DITHER_ON_OFF_DEFAULT						ASRC_DITHER_OFF
#define			ASRC_RAND_SEED_CHANNEL_0_DEFAULT				1
#define			ASRC_RAND_SEED_CHANNEL_1_DEFAULT				1458976




// ===========================================================================
//
// Variables
//
// ===========================================================================

// User parameters
// ---------------
char*			pzInFileName[ASRC_N_IO_CHANNELS];				// Input data file names
char*			pzOutFileName[ASRC_N_IO_CHANNELS];				// Output data file name

unsigned int	uiInFs;											// Input sampling rate
unsigned int	uiOutFs;										// Output sampling rate
double			fFsRatioDeviation;								// Fs ratio deviation

unsigned int	uiNTotalInSamples;								// Total number of input sample pairs to process
unsigned int	uiNTotalOutSamples;								// Total number of output sample pairs produced
unsigned int	uiNInSamples;									// Number of input sample pairs (L/R) to process in one call to processing function

unsigned int	uiASRCDitherOnOff;								// ASRC Dither on/off flag
unsigned int	uiASRCRandSeed[ASRC_N_IO_CHANNELS];				// ASRC channel 0/1 random seeds for dither


// Data I/O
// --------
// Input and output data buffers and file pointers (stereo, 32bits integer, time-domain interleaved)
int				iIn[ASRC_N_IO_CHANNELS * N_TOTAL_IN_SAMPLES_MAX];
int				iOut[ASRC_N_IO_CHANNELS * N_TOTAL_IN_SAMPLES_MAX * ASRC_N_IN_OUT_RATIO_MAX];
FILE			*InFileDat[ASRC_N_IO_CHANNELS];
FILE			*OutFileDat[ASRC_N_IO_CHANNELS];


// Error message string
// --------------------
char			pzError[1024];


// Various
// -------
unsigned int	uiFsTable[ASRC_N_FS]	= {44100, 48000, 88200, 96000, 176400, 192000};



// ===========================================================================
//
// Functions
//
// ===========================================================================




int main(int argc, char *argv[])
{
	int				*piIn, *piOut;
	int				iNRemainingSamples;
	int				uiSplCntr;
	int				iEndOfFile;
	unsigned int	ui, uj;

	//	Display application information
	fprintf(stdout, "\n%s  \nVersion: %s\n%s\n\n",
		APPLICATION, VERSION, COPYRIGHT);

	// Initialize default values
	pzInFileName[0]		= INPUT_FILE_CHANNEL_0_DEFAULT;			// Input data for channel 0
	pzInFileName[1]		= INPUT_FILE_CHANNEL_1_DEFAULT;			// Input data for channel 1
	pzOutFileName[0]	= OUTPUT_FILE_CHANNEL_0_DEFAULT;		// Output data for channel 0
	pzOutFileName[1]	= OUTPUT_FILE_CHANNEL_0_DEFAULT;		// Output data for channel 1

	uiInFs				= INPUT_FS_DEFAULT;
	uiOutFs				= OUTPUT_FS_DEFAULT;
	fFsRatioDeviation	= FS_RATIO_DEVIATION_DEFAULT;

	uiNTotalInSamples	= N_TOTAL_IN_SAMPLES_DEFAULT;
	uiNInSamples		= N_IN_SAMPLES_DEFAULT;

	uiASRCDitherOnOff	= ASRC_DITHER_ON_OFF_DEFAULT;

	uiASRCRandSeed[0]	= ASRC_RAND_SEED_CHANNEL_0_DEFAULT;		// Random seed for channel 0
	uiASRCRandSeed[1]	= ASRC_RAND_SEED_CHANNEL_1_DEFAULT;		// Random seed for channel 1

	ASRCCtrl_profile_only_t *profile_info_ptr[MAX_ASRC_N_IO_CHANNELS]; // Array of pointers

	//	Parse command line arguments
	for (ui = 1; ui < (unsigned int)argc; ui++)
	{
		if (*(argv[ui]) == '-')
		{
			ParseCmdLine(argv[ui] + 1);
		}
		else
		{
			ShowHelp();
		}
	}

	// Report settings
	printf("ASRC Settings\n");
	printf("-------------\n");
	printf("Dither flag: %i\n", uiASRCDitherOnOff);
	for(ui = 0; ui < ASRC_N_IO_CHANNELS; ui++)
		printf("Random seed channel %i: %i\n", ui, uiASRCRandSeed[ui]);
	printf("\n");

	for(ui = 0; ui < ASRC_N_IO_CHANNELS; ui++)
		printf("Input file channel %i: %s\n", ui, pzInFileName[ui]);
	for(ui = 0; ui < ASRC_N_IO_CHANNELS; ui++)
		printf("Output file channel %i: %s\n", ui, pzOutFileName[ui]);

	printf("\nInput sampling frequency code: %i (%iHz)\n", uiInFs, uiFsTable[uiInFs]);
	printf("Output sampling frequency code: %i (%iHz)\n", uiOutFs, uiFsTable[uiOutFs]);
	printf("Fs ratio deviation factor: %f\n\n", fFsRatioDeviation);

	printf("Total number of input samples: %i\n", uiNTotalInSamples);
	printf("Number of input samples to process in one call: %i\n\n", uiNInSamples);


	// Clear data buffers
	for(ui = 0; ui < ASRC_N_IO_CHANNELS * N_TOTAL_IN_SAMPLES_MAX; ui++)
		iIn[ui]		= 0;
	for(ui = 0; ui < ASRC_N_IO_CHANNELS * N_TOTAL_IN_SAMPLES_MAX * ASRC_N_IN_OUT_RATIO_MAX; ui++)
		iOut[ui]	= 0;

	// Open i/o files
	for(ui = 0; ui < ASRC_N_IO_CHANNELS; ui++)
	{
		if ((InFileDat[ui] = fopen(pzInFileName[ui], "rt")) == NULL)
		{
			sprintf(pzError, "Error while opening input file, %s", pzInFileName[ui]);
			HandleError(pzError, FATAL);
		}
		if ((OutFileDat[ui] = fopen(pzOutFileName[ui], "wt")) == NULL)
		{
			sprintf(pzError, "Error while creating output file, %s", pzOutFileName[ui]);
			HandleError(pzError, FATAL);
		}
	}


	// Process init
	// ------------
	uint64_t nominal_fs_ratio = wrapper_asrc_init(
										&profile_info_ptr, // Pointer to array of pointers
										uiFsTable[uiInFs],
										uiFsTable[uiOutFs],
										uiNInSamples,
										ASRC_N_IO_CHANNELS,
										ASRC_N_CHANNELS,
										uiASRCDitherOnOff,
										uiASRCRandSeed
									);


	// Load input data from files
	// --------------------------
	for(ui = 0; ui < ASRC_N_IO_CHANNELS; ui++)
	{
		iEndOfFile	= 0;
		for(uj = 0; uj < uiNTotalInSamples; uj++)
		{
			if(iEndOfFile == 0)
				if (fscanf(InFileDat[ui], "%i\n", &iIn[ui + uj * ASRC_N_IO_CHANNELS]) == EOF)
					iEndOfFile = 1;		// We are at the end of the file
		}
	}

	// For impulse response test on second channel
	//for(uj = 0; uj < uiNTotalInSamples ; uj++)
	//	iIn[2 * uj + 1] = 0;
	//iIn[1] = 1000000000;


	// Process data
	// ------------
	// Initialize remaing number of samples to total number of input samples,
	// total number of output samples to 0 and setup input / output data pointers to base of buffers
	iNRemainingSamples		= (int)uiNTotalInSamples;
	uiNTotalOutSamples		= 0;
	piIn					= iIn;
	piOut					= iOut;


	// Update Fs Ratio
	uint64_t actual_fs_ratio = (unsigned long long)(nominal_fs_ratio * fFsRatioDeviation);


	// Loop through input samples, block by block
	while(iNRemainingSamples >= (int)uiNInSamples)
	{
		// Setup
		// -----
		for(ui = 0; ui < ASRC_N_IO_CHANNELS; ui++)
		{
			// Reset cycle counts for MIPS estimations
			profile_info_ptr[ui]->fCycleCountF1F2				= 0;
			profile_info_ptr[ui]->fCycleCountF3AdaptiveCoefs	= 0;
			profile_info_ptr[ui]->fCycleCountF3					= 0;
			profile_info_ptr[ui]->fCycleCountDither				= 0;
		}

		unsigned num_output_samples = wrapper_asrc_process(piIn, piOut, actual_fs_ratio);

		// Write output data to files
		// --------------------------
		for(ui = 0; ui < ASRC_N_IO_CHANNELS; ui++)
		{
			int *piOut_ch = piOut + ui;
			for(uj = 0; uj < num_output_samples; uj++)
			{
				if(fprintf(OutFileDat[ui], "%i\n", *(piOut_ch + ASRC_N_IO_CHANNELS * uj) ) < 0)
				{
					sprintf(pzError, "Error while writing to output file, %s", pzOutFileName[ui]);
					HandleError(pzError, FATAL);
				}
			}
		}


		// Update input and outut data pointers for next round
		piIn					+= (uiNInSamples * ASRC_N_IO_CHANNELS);
		piOut					+= num_output_samples * ASRC_N_IO_CHANNELS;
		iNRemainingSamples		-= (int)uiNInSamples;

		// Update total output sample counter
		uiNTotalOutSamples		+= num_output_samples;
	}


	// Report MIPS
	for(ui = 0; ui < ASRC_N_IO_CHANNELS; ui++)
	{
		printf("MIPS total load channel %i: %f\n", ui, (profile_info_ptr[ui]->fCycleCountF1F2 + profile_info_ptr[ui]->fCycleCountF3AdaptiveCoefs + profile_info_ptr[ui]->fCycleCountF3 + profile_info_ptr[ui]->fCycleCountDither) / (float)(uiNInSamples) * (float)uiFsTable[uiInFs]/ 1000000.0);
		printf("MIPS F1+F2 load channel %i: %f\n", ui, profile_info_ptr[ui]->fCycleCountF1F2 / (float)(uiNInSamples) * (float)uiFsTable[uiInFs]/ 1000000.0);
		printf("MIPS F3 Adaptive Coefs computation load channel %i: %f\n", ui, profile_info_ptr[ui]->fCycleCountF3AdaptiveCoefs / (float)(uiNInSamples) * (float)uiFsTable[uiInFs]/ 1000000.0);
		printf("MIPS F3 load channel %i: %f\n", ui, profile_info_ptr[ui]->fCycleCountF3 / (float)(uiNInSamples) * (float)uiFsTable[uiInFs]/ 1000000.0);
		printf("MIPS Dither load channel %i: %f\n\n", ui, profile_info_ptr[ui]->fCycleCountDither / (float)(uiNInSamples) * (float)uiFsTable[uiInFs]/ 1000000.0);

	}
	// Report number of output samples produced
	printf("Total number of output samples produced: %i\n\n\n", uiNTotalOutSamples);


	//	Close i/o files
	for(ui = 0; ui < ASRC_N_IO_CHANNELS; ui++)
	{
		if (fclose(InFileDat[ui]))
		{
			sprintf(pzError, "Error while closing input file, %s", pzInFileName[ui]);
			HandleError(pzError, FATAL);
		}
		if (fclose(OutFileDat[ui]))
		{
			sprintf(pzError, "Error while closing output file, %s", pzOutFileName[ui]);
			HandleError(pzError, FATAL);
		}
	}



	fprintf(stdout, "Application run finished, press any key to exit\n\n");
	getchar();

	return 0;
}



void ParseCmdLine(char *input)
{
	switch (*input)
	{
		case 'd':
		case 'D':
			uiASRCDitherOnOff = (unsigned int)(atoi(input + 1));
			if((uiASRCDitherOnOff < ASRC_DITHER_ON_OFF_MIN) || (uiASRCDitherOnOff > ASRC_DITHER_ON_OFF_MAX))
			{
				sprintf(pzError, "Error: unsupported dither on off flag %u", uiASRCDitherOnOff);
				HandleError(pzError, FATAL);
			}
			break;

		case 'e':
		case 'E':
			fFsRatioDeviation = (double)(atof(input + 1));
			break;

		case 'h':
		case 'H':
			ShowUsage();
			exit(0);
			break;

		case 'i':
		case 'I':
			pzInFileName[0] = input + 1;
			break;

		case 'j':
		case 'J':
			pzInFileName[1] = input + 1;
			break;

		case 'k':
		case 'K':
			uiInFs = (unsigned int)(atoi(input + 1));
			if((uiInFs < ASRC_FS_MIN) || (uiInFs > ASRC_FS_MAX))
			{
				sprintf(pzError, "Error: unsupported input sampling frequency code %u", uiInFs);
				HandleError(pzError, FATAL);
			}
			break;

		case 'l':
		case 'L':
			uiNTotalInSamples = (unsigned int)(atoi(input + 1));
			if(uiNTotalInSamples > N_TOTAL_IN_SAMPLES_MAX)
			{
				sprintf(pzError, "Error: unsupported total number of samples %u, maximum is %u", uiNTotalInSamples, N_TOTAL_IN_SAMPLES_MAX);
				HandleError(pzError, FATAL);
			}
			break;

		case 'n':
		case 'N':
			uiNInSamples = (unsigned int)(atoi(input + 1));
			if(uiNInSamples > N_IN_SAMPLES_MAX)
			{
				sprintf(pzError, "Error: unsupported number of samples to process in one call %u, maximum is %u", uiNInSamples, N_IN_SAMPLES_MAX);
				HandleError(pzError, FATAL);
			}
			break;

		case 'o':
		case 'O':
			pzOutFileName[0] = input + 1;
			break;

		case 'p':
		case 'P':
			pzOutFileName[1] = input + 1;
			break;

		case 'q':
		case 'Q':
			uiOutFs = (unsigned int)(atoi(input + 1));
			if((uiOutFs < ASRC_FS_MIN) || (uiOutFs > ASRC_FS_MAX))
			{
				sprintf(pzError, "Error: unsupported output sampling frequency code %u", uiInFs);
				HandleError(pzError, FATAL);
			}
			break;

		case 'r':
		case 'R':
			uiASRCRandSeed[0] = (unsigned int)(atoi(input + 1));
			break;

		case 's':
		case 'S':
			uiASRCRandSeed[1] = (unsigned int)(atoi(input + 1));
			break;

		default:
			ShowHelp();
			break;
	}
}



void ShowHelp()
{
	fprintf(stdout,"Invalid argument. Application will terminate now. Use -h to get usage.\n");
	fprintf(stdout, "Press any key to exit\n\n");
	getchar();

	exit(0);
}


void ShowUsage()
{
	puts(
		"Usage: ASRC_Demo [-d<Dither>][e<Fs Ratio deviation>][-h][-i<Input ch0 dat file.ext>][-j<Input ch1 dat file.ext>]\n\n"
		"            [-l<Total number of input samples>][-n<Number of input samples to process>]\n\n"
		"			 [-o<Output ch0 dat file.ext>][-p<Output ch0 dat file.ext>]\n\n"
		"			 [-r<Random Seed for channel 0>][-s<Random Seed for channel 1>]\n\n"
		"         -d     Dither enable flag (default: 0=disabled)\n\n"
		"         -e     Fs Ratio deviation (default: 1.0, allowable 0.99-1.01)\n\n"
		"         -h     Show this usage message and abort\n\n"
		"         -i     Dat input channel 0 file name (default: input_ch0.dat)\n\n"
		"         -j     Dat input channel 1 file name (default: input_ch1.dat)\n\n"
		"         -l     Total number of input sample pairs to process (default: 4096, max: 1048576)\n\n"
		"         -n     Number of input sample pairs (L/R) to process in one call to processing functions (default: 32)\n\n"
		"         -o	 Dat output channel 0 file name (default: output_ch0.dat)\n\n"
		"         -p	 Dat output channel 1 file name (default: output_ch1.dat)\n\n"
		"		  -r	 Random seed for channel 0 for dither generation (default: 1)\n\n"
		"		  -s	 Random seed for channel 1 for dither generation (default: 1458976)\n\n"
		);

	exit(0);
}


void HandleError(char *pzErrorMessage, short sErrorType)
{

	if (sErrorType == FATAL)
	{
		fprintf(stdout, "\nFATAL ERROR: %s: %s\nPress any key to exit\n", APPLICATION, pzErrorMessage);
		getchar();
		exit(0);
	}
	else if (sErrorType == WARNING)
	{
		fprintf(stdout, "\nWARNING: %s: %s\nPress any key to continue\n", APPLICATION, pzErrorMessage);
		getchar();
		return;
	}

	// Signal unhandled error
	fprintf(stdout, "\nUNHANDLED: %s: %s\nPress any key to continue\n", APPLICATION, pzErrorMessage);
	getchar();
	return;
}
