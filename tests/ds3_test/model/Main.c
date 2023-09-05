// ===========================================================================
// ===========================================================================
//	
// File: Main.c
//
// Main implementation file for the DS3 demonstration program
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

// FIR DS3 includes
#include "FIRDS3.h"

// Main file includes
#include "Main.h"



// ===========================================================================
//
// Defines
//
// ===========================================================================


// Default values
// --------------
#define			INPUT_FILE_DEFAULT								"input.dat"
#define			OUTPUT_FILE_DEFAULT								"output.dat"


// ===========================================================================
//
// Variables
//
// ===========================================================================

// User parameters
// ---------------
char*			pzInFileName;									// Input data file name
char*			pzOutFileName;									// Output data file name



// DS3 instances variables
// -----------------------
// State and Control structures (one for each channel)
int					iFIRDS3Delay[FIRDS3_N_COEFS<<1];
FIRDS3Ctrl_t		sFIRDS3Ctrl;
	

// Data I/O
// --------
// Input and output data buffers and file pointers (stereo, 32bits integer, time-domain interleaved)
int				iIn[3];
int				iOut;
FILE			*InFileDat;
FILE			*OutFileDat;


// Error message string
// --------------------
char			pzError[1024];




// ===========================================================================
//
// Functions
//
// ===========================================================================




void main(int argc, char *argv[])
{
	int				iDone;
	unsigned int	ui;


	//	Display application information 
	fprintf(stdout, "\n%s  \nVersion: %s\n%s\n\n",
		APPLICATION, VERSION, COPYRIGHT);
	
	// Initialize default values
	pzInFileName		= INPUT_FILE_DEFAULT;					// Input data 		
	pzOutFileName		= OUTPUT_FILE_DEFAULT;					// Output data 
	

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
	printf("DS3 Settings\n");
	printf("------------\n");
	printf("Input file: %s\n", pzInFileName);
	printf("Output file: %s\n\n", pzOutFileName);


	// Open i/o files 
	if ((InFileDat = fopen(pzInFileName, "rt")) == NULL)
	{
		sprintf(pzError, "Error while opening input file, %s", pzInFileName);
		HandleError(pzError, FATAL);
	}
	if ((OutFileDat = fopen(pzOutFileName, "wt")) == NULL)
	{
		sprintf(pzError, "Error while creating output file, %s", pzOutFileName);
		HandleError(pzError, FATAL);
	}

	
	// Process init
	// ------------
	// Set delay line base to ctrl structure
	sFIRDS3Ctrl.piDelayB		= iFIRDS3Delay;
		
	// Init instance
	if(FIRDS3_init(&sFIRDS3Ctrl) != FIRDS3_NO_ERROR)
	{
		sprintf(pzError, "Error at initialization");
		HandleError(pzError, FATAL);
	}

	// Sync
	// ----
	// This is just to show that the function works and returns success
	if(FIRDS3_sync(&sFIRDS3Ctrl) != FIRDS3_NO_ERROR)
	{
		sprintf(pzError, "Error at sync");
		HandleError(pzError, FATAL);
	}


	// Process data
	// ------------

	// Clear end of file mark
	iDone			= 0;


	// Run through input file, reading 3 samples at a time
	while(iDone == 0)
	{		
		// Read 3 samples
		if(fscanf( InFileDat, "%i\n%i\n%i\n", &iIn[0], &iIn[1], &iIn[2]) != EOF)
		{
			// Set input and output data pointers
			sFIRDS3Ctrl.piIn			= iIn;
			sFIRDS3Ctrl.piOut			= &iOut;
			if(FIRDS3_proc(&sFIRDS3Ctrl) != FIRDS3_NO_ERROR)
			{
				sprintf(pzError, "Error at process");
				HandleError(pzError, FATAL);
			}

			// Write output data to file
			if(fprintf(OutFileDat, "%i\n", iOut) < 0)
			{
				sprintf(pzError, "Error while writing to output file, %s", pzOutFileName);
				HandleError(pzError, FATAL);
			}
		}
		else
			// Mark end of file
			iDone = 1;
	}
	

	//Close i/o files
	// ---------------
	if (fclose(InFileDat))
	{
		sprintf(pzError, "Error while closing input file, %s", pzInFileName);
		HandleError(pzError, FATAL);
	}
	if (fclose(OutFileDat))
	{
		sprintf(pzError, "Error while closing output file, %s", pzOutFileName);
		HandleError(pzError, FATAL);
	}

	fprintf(stdout, "Application run finished, press any key to exit\n\n");
	getchar();
	
	exit(0);
}



void ParseCmdLine(char *input)
{
	switch (*input)
	{
		case 'h':
		case 'H':
			ShowUsage();
			exit(0);
			break;
		
		case 'i':
		case 'I':
			pzInFileName = input + 1;
			break;

		case 'o':
		case 'O':
			pzOutFileName = input + 1;
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
		"Usage: DS3 [-h][-i<Input file.ext>][-o<Output file.ext>]\n\n"
		"         -h     Show this usage message and abort\n\n"
		"         -i     DSD input binary file name (default: input_dsd.bin)\n\n"
		"         -o	 PCM output text file name (default: output_pcm.dat)\n\n"
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


