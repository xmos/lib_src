// ===========================================================================
// ===========================================================================
//	
// File: SSRC.h
//
// Top level definition file for the SSRC
//
// Target:	MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================

#ifndef _SSRC_H_
#define _SSRC_H_

	// ===========================================================================
	//
	// Includes
	//
	// ===========================================================================
	#include "FIR.h"
	#include "FilterDefs.h"
	#include "src.h"

	// ===========================================================================
	//
	// Defines
	//
	// ===========================================================================
	
	// General defines
	// ---------------
	#define		SSRC_STACK_LENGTH_MULT				(SSRC_N_CHANNELS * SSRC_N_IN_SAMPLES)	// Multiplier for stack length (stack length = this value x the number of input samples to process)


	// Parameter values
	// ----------------
	#define		SSRC_DITHER_OFF					SRC_DITHER_OFF
	#define		SSRC_DITHER_ON					SRC_DITHER_ON
	#define		SSRC_DITHER_ON_OFF_MIN				SSRC_DITHER_OFF
	#define		SSRC_DITHER_ON_OFF_MAX				SSRC_DITHER_ON




	// ===========================================================================
	//
	// Defines
	//
	// ===========================================================================
	

	// ===========================================================================
	//
	// TypeDefs
	//
	// ===========================================================================

	// To avoid C type definitions when including this file from assembler
	#ifndef INCLUDE_FROM_ASM

		// SSRC Return Codes
		// -----------------
		typedef enum _SSRCReturnCodes									
		{
			SSRC_NO_ERROR							= 0,
			SSRC_ERROR								= 1
		} SSRCReturnCodes_t;
		

		// Sampling rate codes
		// -------------------
		typedef enum _SSRCFs											
		{
			SSRC_FS_44								= 0,								// Fs = 44.1kHz code
			SSRC_FS_48								= 1,								// Fs = 48kHz code
			SSRC_FS_88								= 2,								// Fs = 88.2kHz code
			SSRC_FS_96								= 3,								// Fs = 96kHz code
			SSRC_FS_176								= 4,								// Fs = 176.4kHz code
			SSRC_FS_192								= 5,								// Fs = 192kHz code
		} SSRCFs_t;
		#define		SSRC_N_FS						(SSRC_FS_192 + 1)
		#define		SSRC_FS_MIN						SSRC_FS_44
		#define		SSRC_FS_MAX						SSRC_FS_192

		
		// SSRC Filters IDs structure
		// --------------------------
		#define		SSRC_F1_INDEX					0
		#define		SSRC_F2_INDEX					1
		#define		SSRC_F3_INDEX					2
		#define		SSRC_N_F						(SSRC_F3_INDEX + 1)

		typedef struct _SSRCFiltersIDs										
		{
			unsigned int		uiFID[SSRC_N_F];
			unsigned int		uiPPFIRPhaseStep;
		} SSRCFiltersIDs_t;


		// SSRC State structure
		// --------------------
		typedef struct _SSRCState							
		{
			long long                               pad_to_64b_alignment;
			int										iDelayFIRLong[2 * FILTER_DEFS_FIR_MAX_TAPS_LONG];		// Doubled length for circular buffer simulation
			int										iDelayFIRShort[2 * FILTER_DEFS_FIR_MAX_TAPS_SHORT];		// Doubled length for circular buffer simulation
			int										iDelayPPFIR[2 * FILTER_DEFS_PPFIR_PHASE_MAX_TAPS];		// Doubled length for circular buffer simulation
            unsigned int                            uiRndSeed;                                              // Dither random seeds current values

		} SSRCState_t;
	

		// SSRC Control structure
		// ----------------------
		typedef struct _SSRCCtrl											
		{
			int*									piIn;								// Input buffer pointer (PCM, 32bits, 2 channels time domain interleaved data)
			unsigned int							uiNInSamples;						// Number of input samples to process in one call to the processing function
			SSRCFs_t								eInFs;								// Input sampling rate code
			int*									piOut;								// Output buffer poin ter (PCM, 32bits, 2 channels time domain interleaved data)							
			unsigned int*							puiNOutSamples;						// Pointer to number of output samples produced during last call to the processing function
			SSRCFs_t								eOutFs;								// Output sampling rate code

			int**									ppiOut;								// Pointer to (PP)FIR output data pointer for last filter in the chain

			FIRCtrl_t								sFIRF1Ctrl;							// F1 FIR controller
			FIRCtrl_t								sFIRF2Ctrl;							// F2 FIR controller
			PPFIRCtrl_t								sPPFIRF3Ctrl;						// F3 PPFIR controller
		
			unsigned int							uiDitherOnOff;						// Dither on/off flag
			unsigned int							uiRndSeedInit;						// Dither random seed initial value

			SSRCState_t*							psState;							// Pointer to state structure
			int*									piStack;							// Pointer to stack buffer
		} SSRCCtrl_t;


		// ===========================================================================
		//
		// Function prototypes
		//
		// ===========================================================================

		// ==================================================================== //
		// Function:		SSRC_init											//
		// Arguments:		SSRCCtrl_t 	*psSSRCCtrl: Ctrl strct.				//
		// Return values:	SSRC_NO_ERROR on success							//
		//					SSRC_ERROR on failure								//
		// Description:		Inits the SSRC passed as argument					//
		// ==================================================================== //
		SSRCReturnCodes_t				SSRC_init(SSRCCtrl_t* psSSRCCtrl);	

		// ==================================================================== //
		// Function:		SSRC_sync											//
		// Arguments:		SSRCCtrl_t 	*psSSRCCtrl: Ctrl strct.				//
		// Return values:	SSRC_NO_ERROR on success							//
		//					SSRC_ERROR on failure								//
		// Description:		Syncs the SSRC passed as argument					//
		// ==================================================================== //
		SSRCReturnCodes_t				SSRC_sync(SSRCCtrl_t* psSSRCCtrl);

		// ==================================================================== //
		// Function:		SSRC_proc											//
		// Arguments:		SSRCCtrl_t 	*psSSRCCtrl: Ctrl strct.				//
		// Return values:	SSRC_NO_ERROR on success							//
		//					SSRC_ERROR on failure								//
		// Description:		Processes the SSRC passed as argument				//
		// ==================================================================== //
		SSRCReturnCodes_t				SSRC_proc(SSRCCtrl_t* psSSRCCtrl);							
		
	#endif // nINCLUDE_FROM_ASM

#endif // _SSRC_H_
