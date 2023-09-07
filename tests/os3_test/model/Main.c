// ===========================================================================
// ===========================================================================
//	
// File: Main.c
//
// Main implementation file for the OS3 demonstration program
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

// FIR OS3 includes
#include "FIROS3.h"

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
char*			pzInFileName;											// Input data file name
char*			pzOutFileName;											// Output data file name



// OS3 instances variables
// -----------------------
// State and Control structures (one per channel)
int				iFIROS3Delay[(FIROS3_N_COEFS/FIROS3_N_PHASES)<<1];		// Delay line length is 1/3rd of number of coefs as over-sampler by 3 (and double for circular buffer simulation)
FIROS3Ctrl_t	sFIROS3Ctrl;
	

// Data I/O
// --------
// Input and output data file pointers (data files are supposed to be 32bits integer)
FILE			*InFileDat;
FILE			*OutFileDat;


// Error message string
// --------------------
char			pzError[1024];


int				iCntr;

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
	printf("OS3 Settings\n");
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
	sFIROS3Ctrl.piDelayB		= iFIROS3Delay;
		
	// Init instance
	if(FIROS3_init(&sFIROS3Ctrl) != FIROS3_NO_ERROR)
	{
		sprintf(pzError, "Error at initialization");
		HandleError(pzError, FATAL);
	}

	// Sync
	// ----
	// This is just to show that the function works and returns success
	if(FIROS3_sync(&sFIROS3Ctrl) != FIROS3_NO_ERROR)
	{
		sprintf(pzError, "Error at sync");
		HandleError(pzError, FATAL);
	}


	// Process data
	// ------------

	// Clear end of file mark
	iDone			= 0;

	// Run through output samples, we read 1 input sample every 3 output samples
	// This is given when the iPhase member of the FIROS control structure reaches '0'
	while(iDone == 0)
	{	
		// Check if a new input sample is needed
		// If it is needed read it into the control structure and call function to write to delay line
		// If we have reached end of file, signal it.
		if(sFIROS3Ctrl.iPhase == 0)
		{
			if(fscanf( InFileDat, "%i\n", &sFIROS3Ctrl.iIn) != EOF)
			{	
				// Try to read new input data to control structure
				if(FIROS3_input(&sFIROS3Ctrl) != FIROS3_NO_ERROR)
				{
					sprintf(pzError, "Error at writing new input sample to delay line");
					HandleError(pzError, FATAL);
				}
			}
			else
				iDone = 1;
		}

		// If we are not at end of file, do the processing
		if(iDone == 0)
		{
			// Call processing function to get an output sample
			if(FIROS3_proc(&sFIROS3Ctrl) != FIROS3_NO_ERROR)				
			{
				sprintf(pzError, "Error at process");
				HandleError(pzError, FATAL);
			}
			// Write output data to file
			if(fprintf(OutFileDat, "%i\n", sFIROS3Ctrl.iOut) < 0)
			{
				sprintf(pzError, "Error while writing to output file, %s", pzOutFileName);
				HandleError(pzError, FATAL);
			}
		}
		
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
		"Usage: OS3 [-h][-i<Input file.ext>][-o<Output file.ext>]\n\n"
		"         -h     Show this usage message and abort\n\n"
		"         -i     PCM input text file name (default: input.dat)\n\n"
		"         -o	 PCM output text file name (default: output.dat)\n\n"
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


