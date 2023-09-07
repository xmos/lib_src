// ===========================================================================
// ===========================================================================
//	
// File: SSRC.c
//
// Top level implementation file for the SSRC
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
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

// Integer arithmetic include
#include "IntArithmetic.h"
// SSRC include
#include "SSRC.h"

// ===========================================================================
//
// Defines
//
// ===========================================================================


// State init value
#define		SSRC_STATE_INIT						0

// Random number generator / dithering
#define		SSRC_R_CONS							32767
#define		SSRC_R_BASE							1664525	
#define		SSRC_RPDF_BITS_SHIFT				16						// Shift to select bits in pseudo-random number
#define		SSRC_RPDF_MASK						0x0000007F				// For dithering at 24bits (in 2.30)
#define		SSRC_DATA24_MASK					0xFFFFFF00				// Mask for 24bits data (once rescaled to 1.31)
#define		SSRC_DITHER_BIAS					0xFFFFFFC0				// TPDF dither bias for compensating masking at 24bits but expressed in 2.30

// Cycle counter 
#define		SSRC_FIR_OS2_OVERHEAD_CYCLE_COUNT	(15.0)
#define		SSRC_FIR_OS2_TAP_CYCLE_COUNT		(1.875)
#define		SSRC_FIR_DS2_OVERHEAD_CYCLE_COUNT	(15.0)
#define		SSRC_FIR_DS2_TAP_CYCLE_COUNT		(2.125)
#define		SSRC_FIR_SYNC_OVERHEAD_CYCLE_COUNT	(15.0)
#define		SSRC_FIR_SYNC_TAP_CYCLE_COUNT		(2.125)
#define		SSRC_FIR_PP_OVERHEAD_CYCLE_COUNT	(15.0 + SSRC_FIR_SYNC_OVERHEAD_CYCLE_COUNT)
#define		SSRC_FIR_PP_TAP_CYCLE_COUNT			(2.125)
#define		SSRC_DITHER_SAMPLE_COUNT			(20.0)						



// ===========================================================================
//
// Variables
//
// ===========================================================================

SSRCFiltersIDs_t		sFiltersIDs[SSRC_N_FS][SSRC_N_FS] =				// Filter configuration table [Fsin][Fsout]
{
	{	// Fsin = 44.1kHz
		// F1							F2								F3								Phase step
		{FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS320_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_294},		// Fsout = 48kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS320_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_147},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_OS_ID,			FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_OS_ID,			FILTER_DEFS_PPFIR_HS320_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_147}		// Fsout = 192kHz
	},
	{	// Fsin = 48kHz
		// F1							F2								F3								Phase step
		{FILTER_DEFS_FIR_UP4844_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS294_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_320},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 48kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS294_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_160},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_OS_ID,			FILTER_DEFS_PPFIR_HS294_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_160},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_OS_ID,			FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0}			// Fsout = 192kHz
	},
	{	// Fsin = 88.2kHz
		// F1							F2								F3								Phase step
		{FILTER_DEFS_FIR_BL_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_BL8848_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS320_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_294},		// Fsout = 48kHz
		{FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS320_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_294},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS320_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_147}		// Fsout = 192kHz
	},
	{	// Fsin = 96kHz
		// F1							F2								F3								Phase step
		{FILTER_DEFS_FIR_BL9644_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS294_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_320},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_BL_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 48kHz
		{FILTER_DEFS_FIR_UP4844_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS294_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_320},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS294_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_160},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0}			// Fsout = 192kHz
	},
	{	// Fsin = 176.4kHz
		// F1							F2								F3								Phase step
		{FILTER_DEFS_FIR_DS_ID,			FILTER_DEFS_FIR_BL_ID,			FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_DS_ID,			FILTER_DEFS_FIR_BL8848_ID,		FILTER_DEFS_PPFIR_HS320_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_294},		// Fsout = 48kHz
		{FILTER_DEFS_FIR_BL_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_BL17696_ID,	FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS320_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_294},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UPF_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS320_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_294}		// Fsout = 192kHz
	},
	{	// Fsin = 192kHz
		// F1							F2								F3								Phase step
		{FILTER_DEFS_FIR_DS_ID,			FILTER_DEFS_FIR_BL9644_ID,		FILTER_DEFS_PPFIR_HS294_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_320},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_DS_ID,			FILTER_DEFS_FIR_BL_ID,			FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 48kHz
		{FITLER_DEFS_FIR_BL19288_ID,	FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS294_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_320},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_BL_ID,			FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP192176_ID,	FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_HS294_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_320},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_FIR_NONE_ID,		FILTER_DEFS_PPFIR_NONE_ID,		FILTER_DEFS_PPFIR_PHASE_STEP_0}			// Fsout = 192kHz
	}
};



// ===========================================================================
//
// Local Functions prototypes
//
// ===========================================================================


SSRCReturnCodes_t				SSRC_proc_F1_F2(SSRCCtrl_t* psSSRCCtrl,	unsigned int uiChannel);
SSRCReturnCodes_t				SSRC_proc_F3(SSRCCtrl_t* psSSRCCtrl,	unsigned int uiChannel);
SSRCReturnCodes_t				SSRC_proc_dither(SSRCCtrl_t* psSSRCCtrl,	unsigned int uiChannel);


// ===========================================================================
//
// Functions implementations
//
// ===========================================================================



// ==================================================================== //
// Function:		SSRC_init											//
// Arguments:		SSRCCtrl_t 	*psSSRCCtrl: Ctrl strct.				//
// Return values:	SSRC_NO_ERROR on success							//
//					SSRC_ERROR on failure								//
// Description:		Inits the SSRC passed as argument					//
// ==================================================================== //
SSRCReturnCodes_t				SSRC_init(SSRCCtrl_t* psSSRCCtrl)	
{
	SSRCFiltersIDs_t*			psFiltersID;
	FIRDescriptor_t*			psFIRDescriptor;
	PPFIRDescriptor_t*			psPPFIRDescriptor;
	unsigned int				ui;

	// Check if state is allocated
	if(psSSRCCtrl->psState == 0)
		return SSRC_ERROR;

	// Check if stack is allocated
	if(psSSRCCtrl->piStack == 0)
		return SSRC_ERROR;

	// Check if valid Fsin and Fsout have been provided
	if( (psSSRCCtrl->eInFs < SSRC_FS_MIN) || (psSSRCCtrl->eInFs > SSRC_FS_MAX))
		return SSRC_ERROR;
	if( (psSSRCCtrl->eOutFs < SSRC_FS_MIN) || (psSSRCCtrl->eOutFs > SSRC_FS_MAX))
		return SSRC_ERROR;

	// Check that number of input samples is allocated and is a multiple of 4
	if(psSSRCCtrl->uiNInSamples == 0)
		return SSRC_ERROR;
	if((psSSRCCtrl->uiNInSamples & 0x3) != 0x0)
		return SSRC_ERROR;

	// Load filters ID and number of samples
	psFiltersID		= &sFiltersIDs[psSSRCCtrl->eInFs][psSSRCCtrl->eOutFs];
	
	// Configure filters from filters ID and number of samples
	// Filter F1
	psFIRDescriptor								= &sFirDescriptor[psFiltersID->uiFID[SSRC_F1_INDEX]];
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
	{
		// Set number of input samples and input samples step
		psSSRCCtrl->sFIRF1Ctrl[ui].uiNInSamples		= psSSRCCtrl->uiNInSamples;
		psSSRCCtrl->sFIRF1Ctrl[ui].uiInStep			= SSRC_N_CHANNELS;
		
		// Set delay line base pointer
		if( (psFiltersID->uiFID[SSRC_F1_INDEX] == FILTER_DEFS_FIR_DS_ID) || (psFiltersID->uiFID[SSRC_F1_INDEX] == FILTER_DEFS_FIR_OS_ID) )
			psSSRCCtrl->sFIRF1Ctrl[ui].piDelayB		= psSSRCCtrl->psState->iDelayFIRShort[ui];
		else
			psSSRCCtrl->sFIRF1Ctrl[ui].piDelayB		= psSSRCCtrl->psState->iDelayFIRLong[ui];

		// Set output buffer step
		if(psFiltersID->uiFID[SSRC_F2_INDEX] == FILTER_DEFS_FIR_OS_ID)
			// F2 in use in over-sampling by 2 mode
			psSSRCCtrl->sFIRF1Ctrl[ui].uiOutStep	= 2 * SSRC_N_CHANNELS;
		else
			psSSRCCtrl->sFIRF1Ctrl[ui].uiOutStep	= SSRC_N_CHANNELS;

		// Call init for FIR F1
		if(FIR_init_from_desc(&psSSRCCtrl->sFIRF1Ctrl[ui], psFIRDescriptor) != FIR_NO_ERROR)
			return FIR_ERROR;
	}

	// Filter F2
	psFIRDescriptor								= &sFirDescriptor[psFiltersID->uiFID[SSRC_F2_INDEX]];
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
	{
		// Set number of input samples and input samples step
		psSSRCCtrl->sFIRF2Ctrl[ui].uiNInSamples		= psSSRCCtrl->sFIRF1Ctrl[ui].uiNOutSamples;
		psSSRCCtrl->sFIRF2Ctrl[ui].uiInStep			= psSSRCCtrl->sFIRF1Ctrl[ui].uiOutStep;
		
		// Set delay line base pointer
		if( (psFiltersID->uiFID[SSRC_F2_INDEX] == FILTER_DEFS_FIR_DS_ID) || (psFiltersID->uiFID[SSRC_F2_INDEX] == FILTER_DEFS_FIR_OS_ID) )
			psSSRCCtrl->sFIRF2Ctrl[ui].piDelayB		= psSSRCCtrl->psState->iDelayFIRShort[ui];
		else
			psSSRCCtrl->sFIRF2Ctrl[ui].piDelayB		= psSSRCCtrl->psState->iDelayFIRLong[ui];

		// Set output buffer step
		psSSRCCtrl->sFIRF2Ctrl[ui].uiOutStep	= SSRC_N_CHANNELS;
		
		// Call init for FIR F1
		if(FIR_init_from_desc(&psSSRCCtrl->sFIRF2Ctrl[ui], psFIRDescriptor) != FIR_NO_ERROR)
			return FIR_ERROR;
	}

	// Filter F3
	psPPFIRDescriptor								= &sPPFirDescriptor[psFiltersID->uiFID[SSRC_F3_INDEX]];
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
	{
		// Set number of input samples and input samples step
		if(psFiltersID->uiFID[SSRC_F2_INDEX] == FILTER_DEFS_FIR_NONE_ID)
			psSSRCCtrl->sPPFIRF3Ctrl[ui].uiNInSamples	= psSSRCCtrl->sFIRF1Ctrl[ui].uiNOutSamples;
		else
			psSSRCCtrl->sPPFIRF3Ctrl[ui].uiNInSamples	= psSSRCCtrl->sFIRF2Ctrl[ui].uiNOutSamples;	
		psSSRCCtrl->sPPFIRF3Ctrl[ui].uiInStep		= psSSRCCtrl->sFIRF2Ctrl[ui].uiOutStep;
		
		// Set delay line base pointer
		psSSRCCtrl->sPPFIRF3Ctrl[ui].piDelayB		= psSSRCCtrl->psState->iDelayPPFIR[ui];
		
		// Set output buffer step
		psSSRCCtrl->sPPFIRF3Ctrl[ui].uiOutStep		= SSRC_N_CHANNELS;

		// Set phase step 
		psSSRCCtrl->sPPFIRF3Ctrl[ui].uiPhaseStep	= psFiltersID->uiPPFIRPhaseStep;
		
		// Call init for PPFIR F3
		if(PPFIR_init_from_desc(&psSSRCCtrl->sPPFIRF3Ctrl[ui], psPPFIRDescriptor) != FIR_NO_ERROR)
			return FIR_ERROR;
	}

	// Setup input/output buffers
	// We first set them all to stack + channel (some will be overwritten depending on configuration)
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
	{
		// F1 input is never from stack, so don't set it
		psSSRCCtrl->sFIRF2Ctrl[ui].piIn			= psSSRCCtrl->piStack + ui;
		psSSRCCtrl->sPPFIRF3Ctrl[ui].piIn		= psSSRCCtrl->piStack + ui;

		psSSRCCtrl->sFIRF1Ctrl[ui].piOut		= psSSRCCtrl->piStack + ui;
		psSSRCCtrl->sFIRF2Ctrl[ui].piOut		= psSSRCCtrl->piStack + ui;
		// F3 output is never to stack, so don't set it
	}

	// Finally setup pointer to output buffer that needs to be modified for data output depending on filter configuration
	// Also set pointer to number of output samples
	if(psFiltersID->uiFID[SSRC_F3_INDEX] != FILTER_DEFS_PPFIR_NONE_ID)
	{
		// F3 is in use so take output from F3 output
		for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
			psSSRCCtrl->ppiOut[ui]		= &psSSRCCtrl->sPPFIRF3Ctrl[ui].piOut;
		psSSRCCtrl->puiNOutSamples	= &psSSRCCtrl->sPPFIRF3Ctrl[0].uiNOutSamples;
	}
	else
	{
		if(psFiltersID->uiFID[SSRC_F2_INDEX] != FILTER_DEFS_FIR_NONE_ID)
		{
			// F3 not in use but F2 in use, take output from F2 output
			for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
				psSSRCCtrl->ppiOut[ui]		= &psSSRCCtrl->sFIRF2Ctrl[ui].piOut;
			psSSRCCtrl->puiNOutSamples	= &psSSRCCtrl->sFIRF2Ctrl[0].uiNOutSamples;
		}
		else
		{
			// F3 and F2 not in use but F1 in use or not. Set output from F1 output
			// Note that we also set it to F1 output, even if F1 is not in use (Fsin = Fsout case, this won't cause any problem)
			for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
				psSSRCCtrl->ppiOut[ui]		= &psSSRCCtrl->sFIRF1Ctrl[ui].piOut;
			
			if(psFiltersID->uiFID[SSRC_F1_INDEX] != FILTER_DEFS_FIR_NONE_ID)
				// F1 in use so set number of output sample pointer to number of output sample field of F1
				psSSRCCtrl->puiNOutSamples	= &psSSRCCtrl->sFIRF1Ctrl[0].uiNOutSamples;
			else
				// F1 not in use so set number of output sample pointer to number of input sample field
				psSSRCCtrl->puiNOutSamples	= &psSSRCCtrl->uiNInSamples;
		}
	}
//psSSRCCtrl->puiNOutSamples	= &psSSRCCtrl->uiNInSamples;
	// Call sync function
	if(SSRC_sync(psSSRCCtrl) != SSRC_NO_ERROR)
		return SSRC_ERROR;

	return SSRC_NO_ERROR;
}


// ==================================================================== //
// Function:		SSRC_sync											//
// Arguments:		SSRCCtrl_t 	*psSSRCCtrl: Ctrl strct.				//
// Return values:	SSRC_NO_ERROR on success							//
//					SSRC_ERROR on failure								//
// Description:		Syncs the SSRC passed as argument					//
// ==================================================================== //
SSRCReturnCodes_t				SSRC_sync(SSRCCtrl_t* psSSRCCtrl)
{	
	unsigned int		ui;

	// Sync the FIR and PPFIR
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
	{
		if(FIR_sync(&psSSRCCtrl->sFIRF1Ctrl[ui]) != FIR_NO_ERROR)
			return SSRC_ERROR;
		if(FIR_sync(&psSSRCCtrl->sFIRF2Ctrl[ui]) != FIR_NO_ERROR)
			return SSRC_ERROR;
		if(PPFIR_sync(&psSSRCCtrl->sPPFIRF3Ctrl[ui]) != FIR_NO_ERROR)
			return SSRC_ERROR;
	}

	// Reset random seeds to initial values
	for(ui = 0; ui < SSRC_N_CHANNELS; ui++)
		psSSRCCtrl->psState->uiRndSeed[ui]	= psSSRCCtrl->uiRndSeedInit[ui];

	// Reset cycle counter
	psSSRCCtrl->fCycleCount = 0;

	return SSRC_NO_ERROR;
}
	

// ==================================================================== //
// Function:		SSRC_proc											//
// Arguments:		SSRCCtrl_t 	*psSSRCCtrl: Ctrl strct.				//
// Return values:	SSRC_NO_ERROR on success							//
//					SSRC_ERROR on failure								//
// Description:		Processes the SSRC passed as argument				//
// ==================================================================== //
SSRCReturnCodes_t				SSRC_proc(SSRCCtrl_t* psSSRCCtrl)		
{
	// Setup input / output buffers
	// ----------------------------
	psSSRCCtrl->sFIRF1Ctrl[SSRC_CHANNEL_0].piIn			= psSSRCCtrl->piIn + SSRC_CHANNEL_0;
	*(psSSRCCtrl->ppiOut[SSRC_CHANNEL_0])				= psSSRCCtrl->piOut + SSRC_CHANNEL_0;
	
	psSSRCCtrl->sFIRF1Ctrl[SSRC_CHANNEL_1].piIn			= psSSRCCtrl->piIn + SSRC_CHANNEL_1;
	*(psSSRCCtrl->ppiOut[SSRC_CHANNEL_1])				= psSSRCCtrl->piOut + SSRC_CHANNEL_1;
	

	// F1 and F2 process
	// -----------------
	// Channel 0 on Core 1 for 62.5MHz cores
	if( SSRC_proc_F1_F2(psSSRCCtrl, SSRC_CHANNEL_0) != SSRC_NO_ERROR)
		return SSRC_ERROR;
		
	// Channel 1 on Core 2 for 62.5MHz cores
	if( SSRC_proc_F1_F2(psSSRCCtrl, SSRC_CHANNEL_1) != SSRC_NO_ERROR)
		return SSRC_ERROR;
	

	// F3 process
	// ----------
	// Channel 0 on Core 1 for 62.5MHz cores
	if( SSRC_proc_F3(psSSRCCtrl, SSRC_CHANNEL_0) != SSRC_NO_ERROR)
		return SSRC_ERROR;
		
	// Channel 1 on Core 2 for 62.5MHz cores
	if( SSRC_proc_F3(psSSRCCtrl, SSRC_CHANNEL_1) != SSRC_NO_ERROR)
		return SSRC_ERROR;
	

	// Dither process
	// --------------
	// Channel 0 on Core 1 for 62.5MHz cores
	if( SSRC_proc_dither(psSSRCCtrl, SSRC_CHANNEL_0) != SSRC_NO_ERROR)
		return SSRC_ERROR;
		
	// Channel 1 on Core 2 for 62.5MHz cores
	if( SSRC_proc_dither(psSSRCCtrl, SSRC_CHANNEL_1) != SSRC_NO_ERROR)
		return SSRC_ERROR;	

		
	return SSRC_NO_ERROR;
}


// ==================================================================== //
// Function:		SSRC_proc_F1_F2										//
// Arguments:		SSRCCtrl_t 	*psSSRCCtrl: Ctrl strct.				//
//					unsigned int	uiChannel: Channel to process		//
// Return values:	SSRC_NO_ERROR on success							//
//					SSRC_ERROR on failure								//
// Description:		Processes F1 and F2 for a channel					//
// ==================================================================== //
SSRCReturnCodes_t				SSRC_proc_F1_F2(SSRCCtrl_t* psSSRCCtrl,		unsigned int uiChannel)
{
	int*			piIn		= psSSRCCtrl->piIn + uiChannel;
	int*			piOut		= psSSRCCtrl->piOut + uiChannel;
	unsigned int	ui;


	// Check if F1 is disabled, in which case we just copy input to output as all filters are disabled
	if(psSSRCCtrl->sFIRF1Ctrl[uiChannel].eEnable == FIR_OFF)
	{
		// F1 is not enabled, which means that we are in 1:1 rate, so just copy input to output
		for(ui = 0; ui < SSRC_N_CHANNELS * psSSRCCtrl->uiNInSamples; ui+= SSRC_N_CHANNELS)
			piOut[ui]		= piIn[ui];

		return SSRC_NO_ERROR;
	}

	// F1 is enabled, so call F1
	if(psSSRCCtrl->sFIRF1Ctrl[uiChannel].pvProc(&psSSRCCtrl->sFIRF1Ctrl[uiChannel]) != FIR_NO_ERROR)
		return SSRC_ERROR;

	// Cycle counter estimation
	if(uiChannel == 0)
	{
		if(psSSRCCtrl->sFIRF1Ctrl[uiChannel].pvProc == FIR_proc_os2)
			psSSRCCtrl->fCycleCount		+= psSSRCCtrl->uiNInSamples * ( SSRC_FIR_OS2_OVERHEAD_CYCLE_COUNT +
										 (psSSRCCtrl->sFIRF1Ctrl[uiChannel].uiNCoefs * SSRC_FIR_OS2_TAP_CYCLE_COUNT) );

		if(psSSRCCtrl->sFIRF1Ctrl[uiChannel].pvProc == FIR_proc_ds2)
			psSSRCCtrl->fCycleCount		+= (psSSRCCtrl->uiNInSamples>>1) * ( SSRC_FIR_DS2_OVERHEAD_CYCLE_COUNT +
										 (psSSRCCtrl->sFIRF1Ctrl[uiChannel].uiNCoefs * SSRC_FIR_DS2_TAP_CYCLE_COUNT) );

		if(psSSRCCtrl->sFIRF1Ctrl[uiChannel].pvProc == FIR_proc_sync)
			psSSRCCtrl->fCycleCount		+= psSSRCCtrl->uiNInSamples * ( SSRC_FIR_SYNC_OVERHEAD_CYCLE_COUNT +
										 (psSSRCCtrl->sFIRF1Ctrl[uiChannel].uiNCoefs * SSRC_FIR_SYNC_TAP_CYCLE_COUNT) );
	}
		
	// Check if F2 is enabled
	if(psSSRCCtrl->sFIRF2Ctrl[uiChannel].eEnable == FIR_ON)
	{
		// F2 is enabled, so call F2
		if(psSSRCCtrl->sFIRF2Ctrl[uiChannel].pvProc(&psSSRCCtrl->sFIRF2Ctrl[uiChannel]) != FIR_NO_ERROR)
			return SSRC_ERROR;

		// Cycle counter estimation
		if(uiChannel == 0)
		{
			if(psSSRCCtrl->sFIRF2Ctrl[uiChannel].pvProc == FIR_proc_os2)
				psSSRCCtrl->fCycleCount		+= psSSRCCtrl->sFIRF2Ctrl[uiChannel].uiNInSamples * ( SSRC_FIR_OS2_OVERHEAD_CYCLE_COUNT +
											 (psSSRCCtrl->sFIRF2Ctrl[uiChannel].uiNCoefs * SSRC_FIR_OS2_TAP_CYCLE_COUNT) );

			if(psSSRCCtrl->sFIRF2Ctrl[uiChannel].pvProc == FIR_proc_ds2)
				psSSRCCtrl->fCycleCount		+= (psSSRCCtrl->sFIRF2Ctrl[uiChannel].uiNInSamples>>1) * ( SSRC_FIR_DS2_OVERHEAD_CYCLE_COUNT +
											 (psSSRCCtrl->sFIRF2Ctrl[uiChannel].uiNCoefs * SSRC_FIR_DS2_TAP_CYCLE_COUNT) );

			if(psSSRCCtrl->sFIRF2Ctrl[uiChannel].pvProc == FIR_proc_sync)
				psSSRCCtrl->fCycleCount		+= psSSRCCtrl->sFIRF2Ctrl[uiChannel].uiNInSamples * ( SSRC_FIR_SYNC_OVERHEAD_CYCLE_COUNT +
											 (psSSRCCtrl->sFIRF2Ctrl[uiChannel].uiNCoefs * SSRC_FIR_SYNC_TAP_CYCLE_COUNT) );
		}
	}

	return SSRC_NO_ERROR;
}


// ==================================================================== //
// Function:		SSRC_proc_F3										//
// Arguments:		SSRCCtrl_t 	*psSSRCCtrl: Ctrl strct.				//
//					unsigned int	uiChannel: Channel to process		//
// Return values:	SSRC_NO_ERROR on success							//
//					SSRC_ERROR on failure								//
// Description:		Processes F3 for a channel							//
// ==================================================================== //
SSRCReturnCodes_t				SSRC_proc_F3(SSRCCtrl_t* psSSRCCtrl,		unsigned int uiChannel)
{

	// Check if F3 is enabled
	if(psSSRCCtrl->sPPFIRF3Ctrl[uiChannel].eEnable == FIR_ON)
	{
		// F3 is enabled, so call F3
		if(PPFIR_proc(&psSSRCCtrl->sPPFIRF3Ctrl[uiChannel]) != FIR_NO_ERROR)
			return SSRC_ERROR;

		// Cycle counter estimation
		if(uiChannel == 0)
		{
			psSSRCCtrl->fCycleCount		+= (*psSSRCCtrl->puiNOutSamples) * ( SSRC_FIR_PP_OVERHEAD_CYCLE_COUNT +
											 ((float)psSSRCCtrl->sPPFIRF3Ctrl[uiChannel].uiNCoefs/(float)psSSRCCtrl->sPPFIRF3Ctrl[uiChannel].uiNPhases * SSRC_FIR_PP_TAP_CYCLE_COUNT) );
		}
	}

	return SSRC_NO_ERROR;
}


// ==================================================================== //
// Function:		SSRC_proc_dither									//
// Arguments:		SSRCCtrl_t 	*psSSRCCtrl: Ctrl strct.				//
//					unsigned int	uiChannel: Channel to process		//
// Return values:	SSRC_NO_ERROR on success							//
//					SSRC_ERROR on failure								//
// Description:		Processes dither for a channel						//
// ==================================================================== //
SSRCReturnCodes_t				SSRC_proc_dither(SSRCCtrl_t* psSSRCCtrl,	unsigned int uiChannel)
{
	int*			piData;
	unsigned int	uiR;
	int				iDither;
	__int64			i64Acc;
	unsigned int	ui;


	// Apply dither if required
	if(psSSRCCtrl->uiDitherOnOff == SSRC_DITHER_ON)
	{
		// Get data buffer
		piData	= psSSRCCtrl->piOut + uiChannel;
		// Get random seed
		uiR		= psSSRCCtrl->psState->uiRndSeed[uiChannel];

		// Loop through samples
		for(ui = 0; ui < *(psSSRCCtrl->puiNOutSamples) * SSRC_N_CHANNELS; ui += SSRC_N_CHANNELS)
		{
			// Compute dither sample (TPDF)
			iDither		= SSRC_DITHER_BIAS;

			uiR			= (unsigned int)(SSRC_R_BASE * uiR);
			uiR			= (unsigned int)(SSRC_R_CONS + uiR);
			iDither		+= ((uiR>>SSRC_RPDF_BITS_SHIFT) & SSRC_RPDF_MASK);

			uiR			= (unsigned int)(SSRC_R_BASE * uiR);
			uiR			= (unsigned int)(SSRC_R_CONS + uiR);
			iDither		+= ((uiR>>SSRC_RPDF_BITS_SHIFT) & SSRC_RPDF_MASK);

			// Use MACC instruction to saturate and dither + signal
			i64Acc		= ((__int64)iDither <<32);	// On XMOS this is not necessary, just load dither in the top word of the ACC register
			MACC(&i64Acc, piData[ui], 0x7FFFFFFF);
			LSAT30(&i64Acc);
			// Extract 32bits result
			EXT30(&piData[ui], i64Acc);

			// Mask to 24bits
			piData[ui]	&= SSRC_DATA24_MASK;

			// Increase cycle counter
			if(uiChannel == 0)
				psSSRCCtrl->fCycleCount += SSRC_DITHER_SAMPLE_COUNT;
		}

		// Write random seed back
		psSSRCCtrl->psState->uiRndSeed[uiChannel]	= uiR;
	}

	return SSRC_NO_ERROR;
}
