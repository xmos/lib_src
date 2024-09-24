// ===========================================================================
// ===========================================================================
//	
// File: FIROS3.h
//
// FIROS3 functions definition file 
//
// Target:	MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================
 
#ifndef _FIROS3_H_
#define _FIROS3_H_

	// ===========================================================================
	//
	// Defines
	//
	// ===========================================================================
	
	// General defines
	// ---------------
	// Filter related

	#define		FIROS3_N_COEFS							144						// Number of coefficients must be a multiple of 6
	#define		FIROS3_N_PHASES							3						// Number of output phases (3 as OS3 over-sample by 3)

	// This filter has about 20dB attenuation at 8kHz -> For OS3 this is the recommended filter
	#define		FIROS3_COEFS_FILE						"FilterData/firos3_144.dat"
	// This filter has more than 60dB attenuation at 8kHz
	// #define		FIROS3_COEFS_FILE						"FilterData/firos3_b_144.dat"
	


	// ===========================================================================
	//
	// TypeDefs
	//
	// ===========================================================================

	// To avoid C type definitions when including this file from assembler
	#ifndef INCLUDE_FROM_ASM

		// FIROS3 Return Codes
		// -------------------
		typedef enum _FIROS3ReturnCodes									
		{
			FIROS3_NO_ERROR							= 0,
			FIROS3_ERROR							= 1
		} FIROS3ReturnCodes_t;

		// FIROS3 Ctrl
		// -----------
		typedef struct _FIROS3Ctrl										
		{
			int										iIn;			// Input data (to be updated every 3 output samples, i.e. when iPhase == 0)
			int										iOut;			// Output data (1 sample)
			int										iPhase;			// Current output phase (when reaching '0', a new input sample is required)

			int*									piDelayB;		// Pointer to delay line base
			unsigned int							uiDelayL;		// Total length of delay line
			int*									piDelayI;		// Pointer to current position in delay line
			int*									piDelayW;		// Delay buffer wrap around address (for circular buffer simulation)
			unsigned int							uiDelayO;		// Delay line offset for second write (for circular buffer simulation)

			unsigned int							uiNLoops;		// Number of inner loop iterations
			unsigned int							uiNCoefs;		// Number of coefficients
			int*									piCoefs;		// Pointer to coefficients
		} FIROS3Ctrl_t;

		
		// ===========================================================================
		//
		// Function prototypes
		//
		// ===========================================================================

		// ==================================================================== //
		// Function:		FIROS3_init											//
		// Arguments:		FIROS3Ctrl_t 	*psFIRDS2Ctrl: Ctrl strct.			//
		// Return values:	FIROS3_NO_ERROR on success							//
		//					FIROS3_ERROR on failure								//
		// Description:		Inits the FIROS3 									//
		// ==================================================================== //
		FIROS3ReturnCodes_t				FIROS3_init(FIROS3Ctrl_t* psFIROS3Ctrl);	

		// ==================================================================== //
		// Function:		FIROS3_sync											//
		// Arguments:		FIROS3Ctrl_t 	*psFIRDS2Ctrl: Ctrl strct.			//
		// Return values:	FIROS3_NO_ERROR on success							//
		//					FIROS3_ERROR on failure								//
		// Description:		Syncs the FIROS3 									//
		// ==================================================================== //
		FIROS3ReturnCodes_t				FIROS3_sync(FIROS3Ctrl_t* psFIROS3Ctrl);

		// ==================================================================== //
		// Function:		FIROS3_input										//
		// Arguments:		FIROS3Ctrl_t 	*psFIROS3Ctrl: Ctrl strct.			//
		// Return values:	FIROS3_NO_ERROR on success							//
		//					FIROS3_ERROR on failure								//
		// Description:		Writes new input sample to FIROS3 delay line		//
		// ==================================================================== //
		FIROS3ReturnCodes_t				FIROS3_input(FIROS3Ctrl_t* psFIROS3Ctrl);

		// ==================================================================== //
		// Function:		FIROS3_proc											//
		// Arguments:		FIROS3Ctrl_t 	*psFIROS3Ctrl: Ctrl strct.			//
		// Return values:	FIROS3_NO_ERROR on success							//
		//					FIROS3_ERROR on failure								//
		// Description:		Processes the FIROS3								//
		// ==================================================================== //
		FIROS3ReturnCodes_t				FIROS3_proc(FIROS3Ctrl_t* psFIROS3Ctrl);	

	#endif // nINCLUDE_FROM_ASM

#endif // _FIROS3_H_