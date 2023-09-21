// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// File: ASRC.h
//
// Top level definition file for the ASRC
//
// Target:	MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================

#ifndef _ASRC_H_
#define _ASRC_H_

// ===========================================================================
//
// Includes
//
// ===========================================================================
#include "FIR.h"
#include "FilterDefs.h"
#include "ASRC_wrapper.h"

// ===========================================================================
//
// Defines
//
// ===========================================================================

// General defines
// ---------------
#define ASRC_MAX_N_CHANNELS 1 // Max Number of channels processed by ASRC instance

#define ASRC_STACK_LENGTH_MULT (ASRC_MAX_N_CHANNELS * 4)		   // Multiplier for stack length (stack length = this value x the number of input samples to process)
#define ASRC_ADFIR_COEFS_LENGTH FILTER_DEFS_ADFIR_PHASE_N_TAPS // Length of AD FIR coefficients buffer

// Nominal Fs Ratio scale value
#define ASRC_NOMINAL_FS_SCALE (268435456.0) // 2^28

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

// ASRC Return Codes
// -----------------
typedef enum _ASRCReturnCodes
{
	ASRC_NO_ERROR = 0,
	ASRC_ERROR = 1
} ASRCReturnCodes_t;

// ASRC Filters IDs structure
// --------------------------
#define ASRC_F1_INDEX 0
#define ASRC_F2_INDEX 1
#define ASRC_N_F (ASRC_F2_INDEX + 1)

typedef struct _ASRCFiltersIDs
{
	unsigned int uiFID[ASRC_N_F];
} ASRCFiltersIDs_t;

// ASRC time ratio configurations
// ------------------------------
typedef struct _ASRCFsRatioConfigs
{
	unsigned int uiNominalFsRatio;
	unsigned int uiNominalFsRatio_lo;
	unsigned int uiMinFsRatio;
	unsigned int uiMaxFsRatio;
	int iFsRatioShift;
} ASRCFsRatioConfigs_t;

// ASRC State structure
// --------------------
typedef struct _ASRCState
{
	unsigned int uiRndSeed;									// Dither random seeds current values
	int iDelayFIRLong[2 * FILTER_DEFS_FIR_MAX_TAPS_LONG];	// Doubled length for circular buffer simulation
	int iDelayFIRShort[2 * FILTER_DEFS_FIR_MAX_TAPS_SHORT]; // Doubled length for circular buffer simulation
	int iDelayADFIR[2 * FILTER_DEFS_ADFIR_PHASE_N_TAPS];	// Doubled length for circular buffer simulation
} ASRCState_t;

// ASRC Control structure
// ----------------------
typedef struct _ASRCCtrl
{
	int *piIn;						// Input buffer pointer (PCM, 32bits, 2 channels time domain interleaved data)
	unsigned int uiNInSamples;		// Number of input samples to process in one call to the processing function
	unsigned int uiNSyncSamples;	// Number of synchronous samples produced in one call to the processing function
	ASRCFs_t eInFs;					// Input sampling rate code
	int *piOut;						// Output buffer poin ter (PCM, 32bits, 2 channels time domain interleaved data)
	unsigned int uiNASRCOutSamples; // Number of output samples produced during last call to the asynchronous processing function
	ASRCFs_t eOutFs;				// Output sampling rate code

	FIRCtrl_t sFIRF1Ctrl;	  // F1 FIR controller
	FIRCtrl_t sFIRF2Ctrl;	  // F2 FIR controller
	ADFIRCtrl_t sADFIRF3Ctrl; // F3 ADFIR controller

	unsigned int uiFsRatio; // Fs ratio: Fsin / Fsout
	unsigned int uiFsRatio_lo;

	int iTimeInt;				  // Integer part of time
	unsigned int uiTimeFract;	  // Fractional part of time
	int iTimeStepInt;			  // Integer part of time step
	unsigned int uiTimeStepFract; // Fractional part of time step

	unsigned int uiDitherOnOff; // Dither on/off flag
	unsigned int uiRndSeedInit; // Dither random seed initial value

	ASRCState_t *psState; // Pointer to state structure
	int *piStack;		  // Pointer to stack buffer
	int *piADCoefs;		  // Pointer to AD coefficients

	unsigned int uiInStep;
	unsigned int uiOutStep;

	ASRCCtrl_profile_only_t sProfilingInfo;
} ASRCCtrl_t;

// ===========================================================================
//
// Function prototypes
//
// ===========================================================================

// ==================================================================== //
// Function:        ASRC_prepare_coefs                                  //
// Arguments:       ASRCCtrl_t  *psASRCCtrl: Ctrl strct.                //
// Return values:   ASRC_NO_ERROR on success                            //
//                  ASRC_ERROR on failure                               //
// Description:     Prepares the ASRC coefficients from the prototype   //
//                  Needs to be called only once                        //
// ==================================================================== //
ASRCReturnCodes_t ASRC_prepare_coefs(void);

// ==================================================================== //
// Function:		ASRC_init											//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Inits the ASRC passed as argument					//
// ==================================================================== //
ASRCReturnCodes_t ASRC_init(ASRCCtrl_t *psASRCCtrl);

// ==================================================================== //
// Function:		ASRC_sync											//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Syncs the ASRC passed as argument					//
// ==================================================================== //
ASRCReturnCodes_t ASRC_sync(ASRCCtrl_t *psASRCCtrl);

// ==================================================================== //
// Function:		ASRC_proc_F1_F2										//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Processes F1 and F2 for a channel					//
// ==================================================================== //
ASRCReturnCodes_t ASRC_proc_F1_F2(ASRCCtrl_t *psASRCCtrl);

// ==================================================================== //
// Function:		ASRC_update_fs_ratio								//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Updates the ASRC with the new Fs ratio				//
// ==================================================================== //
ASRCReturnCodes_t ASRC_update_fs_ratio(ASRCCtrl_t *psASRCCtrl);

// ==================================================================== //
// Function:		ASRC_proc_F3_in_spl									//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
//					int iInSample: new input sample						//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Writes new input sample to F3 delay line			//
// ==================================================================== //
ASRCReturnCodes_t ASRC_proc_F3_in_spl(ASRCCtrl_t *psASRCCtrl, int iInSample);

// ==================================================================== //
// Function:		ASRC_proc_F3_time									//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR if an output sample must be produced	//
//					ASRC_ERROR if no output sample needs to be produced	//
// Description:		Processes F3 time									//
// ==================================================================== //
ASRCReturnCodes_t ASRC_proc_F3_time(ASRCCtrl_t *psASRCCtrl);

// ==================================================================== //
// Function:		ASRC_proc_F3_macc									//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
//					int* piOutSample: Address of output sample			//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Processes F3 for a channel							//
// ==================================================================== //
ASRCReturnCodes_t ASRC_proc_F3_macc(ASRCCtrl_t *psASRCCtrl, int *piOutSample);

// ==================================================================== //
// Function:		ASRC_proc_dither									//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Processes dither for a channel						//
// ==================================================================== //
ASRCReturnCodes_t ASRC_proc_dither(ASRCCtrl_t *psASRCCtrl);

#endif // nINCLUDE_FROM_ASM

#endif // _ASRC_H_
