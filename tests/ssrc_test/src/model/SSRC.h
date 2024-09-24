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

	// ===========================================================================
	//
	// Defines
	//
	// ===========================================================================
	
	// General defines
	// ---------------
	#define		SSRC_N_CHANNELS						2						// Number of channels in SSRC instance
	#define		SSRC_CHANNEL_0						0						// SSRC channel 0
	#define		SSRC_CHANNEL_1						1						// SSRC channel 1

	#define		SSRC_N_IN_OUT_RATIO_MAX				5						// Maximum ratio (as integer) between input and output number of samples

	#define		SSRC_STACK_LENGTH_MULT				(SSRC_N_CHANNELS * 4)	// Multiplier for stack length (stack length = this value x the number of input samples to process)


	// Parameter values
	// ----------------
	#define		SSRC_ON								1
	#define		SSRC_OFF							0
	
	#define		SSRC_DITHER_OFF						SSRC_OFF
	#define		SSRC_DITHER_ON						SSRC_ON
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
			unsigned int							uiRndSeed[SSRC_N_CHANNELS];			// Dither random seeds current values
			int										iDelayFIRLong[SSRC_N_CHANNELS][2 * FILTER_DEFS_FIR_MAX_TAPS_LONG];		// Doubled length for circular buffer simulation
			int										iDelayFIRShort[SSRC_N_CHANNELS][2 * FILTER_DEFS_FIR_MAX_TAPS_SHORT];	// Doubled length for circular buffer simulation
			int										iDelayPPFIR[SSRC_N_CHANNELS][2 * FILTER_DEFS_PPFIR_PHASE_MAX_TAPS];		// Doubled length for circular buffer simulation
		} SSRCState_t;
	

		// SSRC Control structure
		// ----------------------
		typedef struct _SSRCCtrl											
		{
			int*									piIn;								// Input buffer pointer (PCM, 32bits, time-domain interleaved L/R)
			unsigned int							uiNInSamples;						// Number of input samples pairs to process in one call to the processing function
			SSRCFs_t								eInFs;								// Input sampling rate code
			int*									piOut;								// Output buffer poin ter (PCM, 32bits, time-domain interleaved L/R)							
			unsigned int*							puiNOutSamples;						// Pointer to number of output sample pairs (L/R) produced during last call to the processing function
			SSRCFs_t								eOutFs;								// Output sampling rate code

			int**									ppiOut[SSRC_N_CHANNELS];			// Pointers to (PP)FIR output data pointer for last filter in the chain

			FIRCtrl_t								sFIRF1Ctrl[SSRC_N_CHANNELS];		// F1 FIR controllers
			FIRCtrl_t								sFIRF2Ctrl[SSRC_N_CHANNELS];		// F2 FIR controllers
			PPFIRCtrl_t								sPPFIRF3Ctrl[SSRC_N_CHANNELS];		// F3 PPFIR controllers
		
			unsigned int							uiDitherOnOff;						// Dither on/off flag
			unsigned int							uiRndSeedInit[SSRC_N_CHANNELS];		// Dither random seeds initial values

			SSRCState_t*							psState;							// Pointer to state structure
			int*									piStack;							// Pointer to stack buffer

			float									fCycleCount;						// Variable to hold cycle count for MIPS estimations
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