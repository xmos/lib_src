// ===========================================================================
// ===========================================================================
//	
// File: Main.c
//
// Main implementation file for the SSRC demonstration program
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

// SSRC includes
#include "SSRC.h"

// Main file includes
#include "Main.h"



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

#define			INPUT_FS_DEFAULT								SSRC_FS_44
#define			OUTPUT_FS_DEFAULT								SSRC_FS_96

#define			N_TOTAL_IN_SAMPLES_DEFAULT						4096
#define			N_IN_SAMPLES_DEFAULT							32

#define			SSRC_DITHER_ON_OFF_DEFAULT						SSRC_DITHER_OFF
#define			SSRC_RAND_SEED_CHANNEL_0_DEFAULT				1
#define			SSRC_RAND_SEED_CHANNEL_1_DEFAULT				1458976




// ===========================================================================
//
// Variables
//
// ===========================================================================

// User parameters
// ---------------
char*			pzInFileName[SSRC_N_CHANNELS];					// Input data file names
char*			pzOutFileName[SSRC_N_CHANNELS];					// Output data file name

unsigned int	uiInFs;											// Input sampling rate
unsigned int	uiOutFs;										// Output sampling rate

unsigned int	uiNTotalInSamples;								// Total number of input sample pairs to process
unsigned int	uiNTotalOutSamples;								// Total number of output sample pairs produced
unsigned int	uiNInSamples;									// Number of input sample pairs (L/R) to process in one call to processing function

unsigned int	uiSSRCDitherOnOff;								// SSRC Dither on/off flag
unsigned int	uiSSRCRandSeed[SSRC_N_CHANNELS];				// SSRC channel 0/1 random seeds for dither


// SSRC instance variables
// -----------------------
// State, Stack, Coefs and Control structures
SSRCState_t		sSSRCState;
int				iSSRCStack[SSRC_STACK_LENGTH_MULT * N_IN_SAMPLES_MAX];
SSRCCtrl_t		sSSRCCtrl;
	

// Data I/O
// --------
// Input and output data buffers and file pointers (stereo, 32bits integer, time-domain interleaved)
int				iIn[SSRC_N_CHANNELS * N_TOTAL_IN_SAMPLES_MAX];
int				iOut[SSRC_N_CHANNELS * N_TOTAL_IN_SAMPLES_MAX * SSRC_N_IN_OUT_RATIO_MAX];
FILE			*InFileDat[SSRC_N_CHANNELS];
FILE			*OutFileDat[SSRC_N_CHANNELS];


// Error message string
// --------------------
char			pzError[1024];


// Various
// -------
unsigned int	uiFsTable[SSRC_N_FS]	= {44100, 48000, 88200, 96000, 176400, 192000};



// ===========================================================================
//
// Functions
//
// ===========================================================================




void main(int argc, char *argv[])
{
	int				*piIn, *piOut;
	int				iNRemainingSamples;
	int				iEndOfFile;
	unsigned int	ui, uj;


	//	Display application information 
	fprintf(stdout, "\n%s  \nVersion: %s\n%s\n\n",
		APPLICATION, VERSION, COPYRIGHT);
	
	// Initialize default values
	pzInFileName[SSRC_CHANNEL_0]		= INPUT_FILE_CHANNEL_0_DEFAULT;				
	pzInFileName[SSRC_CHANNEL_1]		= INPUT_FILE_CHANNEL_1_DEFAULT;				
	pzOutFileName[SSRC_CHANNEL_0]		= OUTPUT_FILE_CHANNEL_0_DEFAULT;	
	pzOutFileName[SSRC_CHANNEL_1]		= OUTPUT_FILE_CHANNEL_0_DEFAULT;

	uiInFs								= INPUT_FS_DEFAULT;
	uiOutFs								= OUTPUT_FS_DEFAULT;

	uiNTotalInSamples					= N_TOTAL_IN_SAMPLES_DEFAULT;
	uiNInSamples						= N_IN_SAMPLES_DEFAULT;

	uiSSRCDitherOnOff					= SSRC_DITHER_ON_OFF_DEFAULT;

	uiSSRCRandSeed[SSRC_CHANNEL_0]		= SSRC_RAND_SEED_CHANNEL_0_DEFAULT;
	uiSSRCRandSeed[SSRC_CHANNEL_1]		= SSRC_RAND_SEED_CHANNEL_1_DEFAULT;

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
	printf("SSRC Settings\n");
	printf("-------------\n");
	printf("Dither flag: %i\n", uiSSRCDitherOnOff);
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
		printf("Random seed channel %i: %i\n", ui, uiSSRCRandSeed[ui]);
	printf("\n");

	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
		printf("Input file channel %i: %s\n", ui, pzInFileName[ui]);
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
		printf("Output file channel %i: %s\n", ui, pzOutFileName[ui]);

	printf("\nInput sampling frequency code: %i (%iHz)\n", uiInFs, uiFsTable[uiInFs]);
	printf("Output sampling frequency code: %i (%iHz)\n\n", uiOutFs, uiFsTable[uiOutFs]);

	printf("Total number of input samples: %i\n", uiNTotalInSamples);
	printf("Number of input samples to process in one call: %i\n", uiNInSamples);
	

	// Clear data buffers
	for(ui = 0; ui < SSRC_N_CHANNELS * N_TOTAL_IN_SAMPLES_MAX; ui++)
		iIn[ui]		= 0;
	for(ui = 0; ui < SSRC_N_CHANNELS * N_TOTAL_IN_SAMPLES_MAX * SSRC_N_IN_OUT_RATIO_MAX; ui++)
		iOut[ui]	= 0;

	// Open i/o files 
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
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
	// Set parameters into parameters structure

	// Set state, stack and coefs into ctrl structure
	sSSRCCtrl.psState					= &sSSRCState;
	sSSRCCtrl.piStack					= iSSRCStack;

	// Set input/output sampling rate codes
	sSSRCCtrl.eInFs						= uiInFs;
	sSSRCCtrl.eOutFs					= uiOutFs;

	// Set number of samples
	sSSRCCtrl.uiNInSamples				= uiNInSamples;

	// Set dither flag and random seeds
	sSSRCCtrl.uiDitherOnOff				= uiSSRCDitherOnOff;
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
		sSSRCCtrl.uiRndSeedInit[ui]			= uiSSRCRandSeed[ui];

	// Init SSRC
	if(SSRC_init(&sSSRCCtrl) != SSRC_NO_ERROR)
	{
		sprintf(pzError, "Error at SSRC initialization");
		HandleError(pzError, FATAL);
	}


	// Load input data from files
	// -------------------------
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
	{
		iEndOfFile	= 0;
		for(uj = 0; uj < uiNTotalInSamples; uj++)
		{
			if(iEndOfFile == 0)
				if (fscanf(InFileDat[ui], "%i\n", &iIn[ui + uj * SSRC_N_CHANNELS]) == EOF)
					iEndOfFile = 1;		// We are at the end of the file
		}
	}

	// For impulse response test on second channel
	//for(uj = 0; uj < uiNTotalInSamples ; uj++)
	//	iIn[2 * uj + 1] = 0;
	//iIn[1] = 1000000000;


	// Sync
	// ----
	// Sync SSRC. This is just to show that the function works and returns success
	if(SSRC_sync(&sSSRCCtrl) != SSRC_NO_ERROR)
	{
		sprintf(pzError, "Error at SSRC sync");
		HandleError(pzError, FATAL);
	}

	// Process data
	// ------------
	// Initialize remaing number of samples to total number of input samples, 
	// total number of output samples to 0 and setup input / output data pointers to base of buffers
	iNRemainingSamples		= (int)uiNTotalInSamples;
	uiNTotalOutSamples		= 0;
	piIn					= iIn;
	piOut					= iOut;


	// Loop through input samples, block by block
	while(iNRemainingSamples >= (int)sSSRCCtrl.uiNInSamples)
	{
		// Reset cycle count for MIPS estimations
		sSSRCCtrl.fCycleCount	= 0;

		// Set input and output data pointers
		sSSRCCtrl.piIn			= piIn;
		sSSRCCtrl.piOut			= piOut;

		if(SSRC_proc(&sSSRCCtrl) != SSRC_NO_ERROR)
		{
			sprintf(pzError, "Error at SSRC process");
			HandleError(pzError, FATAL);
		}

		// Update input and outut data pointers for next round
		piIn					+= (sSSRCCtrl.uiNInSamples * SSRC_N_CHANNELS);
		piOut					+= ((*sSSRCCtrl.puiNOutSamples) * SSRC_N_CHANNELS);
		iNRemainingSamples		-= (int)sSSRCCtrl.uiNInSamples;

		// Update total output sample counter
		uiNTotalOutSamples		+= (*sSSRCCtrl.puiNOutSamples);
	}
	

	// Report MIPS 
	printf("MIPS load: %f\n", sSSRCCtrl.fCycleCount / (float)(sSSRCCtrl.uiNInSamples) * (float)uiFsTable[sSSRCCtrl.eInFs]/ 1000000.0);
	// Report number of output samples produced
	printf("Total number of output samples produced: %i\n\n\n", uiNTotalOutSamples);
	

	// Write output data to files
	// --------------------------
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
		for(uj = 0; uj < uiNTotalOutSamples; uj++)
			if(fprintf(OutFileDat[ui], "%i\n", iOut[ui + SSRC_N_CHANNELS * uj]) < 0)
			{
				sprintf(pzError, "Error while writing to output file, %s", pzOutFileName[ui]);
				HandleError(pzError, FATAL);
			}

	//	Close i/o files
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
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
	
	exit(0);
}



void ParseCmdLine(char *input)
{
	switch (*input)
	{
		case 'd':
		case 'D':
			uiSSRCDitherOnOff = (unsigned int)(atoi(input + 1));
			if((uiSSRCDitherOnOff < SSRC_DITHER_ON_OFF_MIN) || (uiSSRCDitherOnOff > SSRC_DITHER_ON_OFF_MAX))
			{
				sprintf(pzError, "Error: unsupported dither on off flag %u", uiSSRCDitherOnOff);
				HandleError(pzError, FATAL);
			}
			break;
	
		case 'h':
		case 'H':
			ShowUsage();
			exit(0);
			break;
		
		case 'i':
		case 'I':
			pzInFileName[SSRC_CHANNEL_0] = input + 1;
			break;

		case 'j':
		case 'J':
			pzInFileName[SSRC_CHANNEL_1] = input + 1;
			break;

		case 'k':
		case 'K':
			uiInFs = (unsigned int)(atoi(input + 1));
			if((uiInFs < SSRC_FS_MIN) || (uiInFs > SSRC_FS_MAX))
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
			pzOutFileName[SSRC_CHANNEL_0] = input + 1;
			break;

		case 'p':
		case 'P':
			pzOutFileName[SSRC_CHANNEL_1] = input + 1;
			break;

		case 'q':
		case 'Q':
			uiOutFs = (unsigned int)(atoi(input + 1));
			if((uiOutFs < SSRC_FS_MIN) || (uiOutFs > SSRC_FS_MAX))
			{
				sprintf(pzError, "Error: unsupported output sampling frequency code %u", uiInFs);
				HandleError(pzError, FATAL);
			}
			break;

		case 'r':
		case 'R':
			uiSSRCRandSeed[SSRC_CHANNEL_0] = (unsigned int)(atoi(input + 1));
			break;

		case 's':
		case 'S':
			uiSSRCRandSeed[SSRC_CHANNEL_1] = (unsigned int)(atoi(input + 1));
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
		"Usage: SSRC_Demo [-d<Dither>][-h][-i<Input ch0 dat file.ext>][-j<Input ch1 dat file.ext>]\n\n"
		"            [-l<Total number of input samples>][-n<Number of input samples to process>]\n\n"
		"			 [-o<Output ch0 dat file.ext>][-p<Output ch0 dat file.ext>]\n\n"
		"			 [-r<Random Seed for channel 0>][-s<Random Seed for channel 1>]\n\n"
		"         -d     Dither enable flag (default: 0=disabled)\n\n"
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


