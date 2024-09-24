// ===========================================================================
// ===========================================================================
//	
// File: FIRDS3.h
//
// FIRDS3 functions definition file 
//
// Target:	MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================

#ifndef _FIRDS3_H_
#define _FIRDS3_H_

	// ===========================================================================
	//
	// Defines
	//
	// ===========================================================================
	
	// General defines
	// ---------------
	// Filter related

	// These two filters have about 20dB attenuation at 8kHz
	//#define		FIRDS3_N_COEFS							126						// Number of coefficients must be a multiple of 6
	//#define		FIRDS3_COEFS_FILE						"FilterData/firds3_126.dat"

	//#define		FIRDS3_N_COEFS							144						// Number of coefficients must be a multiple of 6
	//#define		FIRDS3_COEFS_FILE						"FilterData/firds3_144.dat"

	// These two filters have more than 60dB attenuation at 8kHz
	//#define		FIRDS3_N_COEFS							126						// Number of coefficients must be a multiple of 6
	//#define		FIRDS3_COEFS_FILE						"FilterData/firds3_b_126.dat"

	#define		FIRDS3_N_COEFS							144						// Number of coefficients must be a multiple of 6
	#define		FIRDS3_COEFS_FILE						"FilterData/firds3_b_144.dat"



	// ===========================================================================
	//
	// TypeDefs
	//
	// ===========================================================================

	// To avoid C type definitions when including this file from assembler
	#ifndef INCLUDE_FROM_ASM

		// FIRDS3 Return Codes
		// -------------------
		typedef enum _FIRDS3ReturnCodes									
		{
			FIRDS3_NO_ERROR							= 0,
			FIRDS3_ERROR							= 1
		} FIRDS3ReturnCodes_t;

		// FIRDS3 Ctrl
		// -----------
		typedef struct _FIRDS3Ctrl										
		{
			int*									piIn;			// Pointer to input data (3 samples)
			int*									piOut;			// Pointer to output data (1 sample)

			int*									piDelayB;		// Pointer to delay line base
			unsigned int							uiDelayL;		// Total length of delay line
			int*									piDelayI;		// Pointer to current position in delay line
			int*									piDelayW;		// Delay buffer wrap around address (for circular buffer simulation)
			unsigned int							uiDelayO;		// Delay line offset for second write (for circular buffer simulation)

			unsigned int							uiNLoops;		// Number of inner loop iterations
			unsigned int							uiNCoefs;		// Number of coefficients
			int*									piCoefs;		// Pointer to coefficients
		} FIRDS3Ctrl_t;

		
		// ===========================================================================
		//
		// Function prototypes
		//
		// ===========================================================================

		// ==================================================================== //
		// Function:		FIRDS3_init											//
		// Arguments:		FIRDS3Ctrl_t 	*psFIRDS2Ctrl: Ctrl strct.			//
		// Return values:	FIRDS3_NO_ERROR on success							//
		//					FIRDS3_ERROR on failure								//
		// Description:		Inits the FIRDS3 									//
		// ==================================================================== //
		FIRDS3ReturnCodes_t				FIRDS3_init(FIRDS3Ctrl_t* psFIRDS3Ctrl);	

		// ==================================================================== //
		// Function:		FIRDS3_sync											//
		// Arguments:		FIRDS3Ctrl_t 	*psFIRDS2Ctrl: Ctrl strct.			//
		// Return values:	FIRDS3_NO_ERROR on success							//
		//					FIRDS3_ERROR on failure								//
		// Description:		Syncs the FIRDS3 									//
		// ==================================================================== //
		FIRDS3ReturnCodes_t				FIRDS3_sync(FIRDS3Ctrl_t* psFIRDS3Ctrl);

		// ==================================================================== //
		// Function:		FIRDS3_proc											//
		// Arguments:		FIRDS3Ctrl_t 	*psFIRDS3Ctrl: Ctrl strct.			//
		// Return values:	FIRDS3_NO_ERROR on success							//
		//					FIRDS3_ERROR on failure								//
		// Description:		Processes the FIRDS3								//
		// ==================================================================== //
		FIRDS3ReturnCodes_t				FIRDS3_proc(FIRDS3Ctrl_t* psFIRDS3Ctrl);	

	#endif // nINCLUDE_FROM_ASM

#endif // _FIRDS3_H_