// ===========================================================================
// ===========================================================================
//	
// File: FIR.c
//
// FIR functions implementation file for the SSRC
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
// FIR includes
#include "FIR.h"

// ===========================================================================
//
// Defines
//
// ===========================================================================


// State init value
#define		FIR_STATE_INIT						0





// ===========================================================================
//
// Variables
//
// ===========================================================================



// ===========================================================================
//
// Local Functions prototypes
//
// ===========================================================================



// ===========================================================================
//
// Functions implementations
//
// ===========================================================================

// ==================================================================== //
// Function:		FIR_init_from_desc									//
// Arguments:		FIRCtrl_t 	*psFIRCtrl: Ctrl strct.					//
//					FIRDescriptor_t *psFIRDescriptor: Desc. strct.		//
// Return values:	FIR_NO_ERROR on success								//
//					FIR_ERROR on failure								//
// Description:		Inits the FIR from the Descriptor					//
// ==================================================================== //
FIRReturnCodes_t				FIR_init_from_desc(FIRCtrl_t* psFIRCtrl, FIRDescriptor_t* psFIRDescriptor)
{
	
	// Check if FIR is disabled (this is given by the number of coefficients being zero)
	if( psFIRDescriptor->uiNCoefs == 0)
	{
		// Number of coefficients is zero, so disable FIR
		psFIRCtrl->eEnable			= FIR_OFF;
		psFIRCtrl->uiNOutSamples	= 0;						
		psFIRCtrl->pvProc			= 0;
		psFIRCtrl->uiDelayL			= 0;						
		psFIRCtrl->piDelayW			= 0;	
		psFIRCtrl->uiDelayO			= 0;
		psFIRCtrl->uiNLoops			= 0;						
		psFIRCtrl->uiNCoefs			= 0;
		psFIRCtrl->piCoefs			= 0;

		return FIR_NO_ERROR;
	}

	// FIR is not disabled
	// Check that delay line base has been set
	if(psFIRCtrl->piDelayB == 0)
		return FIR_ERROR;

	// Check that number of samples has been set and is a multiple of 2
	if(psFIRCtrl->uiNInSamples == 0)
		return FIR_ERROR;
	if((psFIRCtrl->uiNInSamples & 0x1) != 0x0)
		return FIR_ERROR;

	// Check the input and output samples steps have been set
	if(psFIRCtrl->uiInStep == 0)
		return FIR_ERROR;
	if(psFIRCtrl->uiOutStep == 0)
		return FIR_ERROR;

	// Setup depending on FIR descriptor
	switch(psFIRDescriptor->eType)
	{
		// Over-sampler by 2 type
		case FIR_TYPE_OS2:
			if( (psFIRDescriptor->uiNCoefs & 0x3) != 0x0)									// Check that number of coefficients is a multiple of 4
				return FIR_ERROR;
			psFIRCtrl->eEnable			= FIR_ON;
			psFIRCtrl->uiNOutSamples	= (psFIRCtrl->uiNInSamples)<<1;						// Os2 FIR doubles the number of samples
			psFIRCtrl->pvProc			= FIR_proc_os2;
			psFIRCtrl->uiDelayL			= psFIRDescriptor->uiNCoefs;						// Double length for circular buffer simulation, but only half length due to OS2
			psFIRCtrl->piDelayW			= psFIRCtrl->piDelayB + (psFIRDescriptor->uiNCoefs>>1);	
			psFIRCtrl->uiDelayO			= psFIRDescriptor->uiNCoefs>>1;
			psFIRCtrl->uiNLoops			= psFIRDescriptor->uiNCoefs>>2;						// Due to 2 x 32bits read for data and 4 x 32bits for coefs per inner loop
			psFIRCtrl->uiNCoefs			= psFIRDescriptor->uiNCoefs;
			psFIRCtrl->piCoefs			= psFIRDescriptor->piCoefs;
		break;

		// Synchronous type
		case FIR_TYPE_SYNC:
			if( (psFIRDescriptor->uiNCoefs & 0x1) != 0x0)									// Check that number of coefficients is a multiple of 2
				return FIR_ERROR;
			// Non zero coefficients number, so it is a true filter
			psFIRCtrl->eEnable			= FIR_ON;
			psFIRCtrl->uiNOutSamples	= psFIRCtrl->uiNInSamples;							// Sync FIR does not change number of samples
			psFIRCtrl->pvProc			= FIR_proc_sync;
			psFIRCtrl->uiDelayL			= psFIRDescriptor->uiNCoefs<<1;						// Double length for circular buffer simulation
			psFIRCtrl->piDelayW			= psFIRCtrl->piDelayB + psFIRDescriptor->uiNCoefs;	
			psFIRCtrl->uiDelayO			= psFIRDescriptor->uiNCoefs;
			psFIRCtrl->uiNLoops			= psFIRDescriptor->uiNCoefs>>1;						// Due to 2 x 32bits read for data and coefs per inner loop
			psFIRCtrl->uiNCoefs			= psFIRDescriptor->uiNCoefs;
			psFIRCtrl->piCoefs			= psFIRDescriptor->piCoefs;
		break;

		// Down-sample by 2 type
		case FIR_TYPE_DS2:
			if( (psFIRDescriptor->uiNCoefs & 0x1) != 0x0)									// Check that number of coefficients is a multiple of 2
				return FIR_ERROR;
			psFIRCtrl->eEnable			= FIR_ON;
			psFIRCtrl->uiNOutSamples	= psFIRCtrl->uiNInSamples>>1;						// Ds2 FIR divides the number of samples by two
			psFIRCtrl->pvProc			= FIR_proc_ds2;
			psFIRCtrl->uiDelayL			= psFIRDescriptor->uiNCoefs<<1;						// Double length for circular buffer simulation
			psFIRCtrl->piDelayW			= psFIRCtrl->piDelayB + psFIRDescriptor->uiNCoefs;	
			psFIRCtrl->uiDelayO			= psFIRDescriptor->uiNCoefs;
			psFIRCtrl->uiNLoops			= psFIRDescriptor->uiNCoefs>>1;						// Due to 2 x 32bits read for data and coefs per inner loop
			psFIRCtrl->uiNCoefs			= psFIRDescriptor->uiNCoefs;
			psFIRCtrl->piCoefs			= psFIRDescriptor->piCoefs;
		break;

		// Unrecognized type
		default:
			return FIR_ERROR;
	}

	// Sync the FIR
	if(FIR_sync(psFIRCtrl) != FIR_NO_ERROR)
		return FIR_ERROR;

	return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:		FIR_sync											//
// Arguments:		FIRCtrl_t 	*psFIRCtrl: Ctrl strct.					//
// Return values:	FIR_NO_ERROR on success								//
//					FIR_ERROR on failure								//
// Description:		Syncs the FIR 										//
// ==================================================================== //
FIRReturnCodes_t				FIR_sync(FIRCtrl_t* psFIRCtrl)
{
	unsigned int		ui;

	if(psFIRCtrl->eEnable == FIR_ON)
	{
		// Set delay line index back to base
		psFIRCtrl->piDelayI	= psFIRCtrl->piDelayB;

		// Clear delay line
		for(ui = 0; ui < psFIRCtrl->uiDelayL; ui++)
			psFIRCtrl->piDelayB[ui]	= FIR_STATE_INIT;
	}

	return FIR_NO_ERROR;
}

 
// ==================================================================== //
// Function:		FIR_proc_os2										//
// Arguments:		FIRCtrl_t 	*psFIRCtrl: Ctrl strct.					//
// Return values:	FIR_NO_ERROR on success								//
//					FIR_ERROR on failure								//
// Description:		Processes the FIR in over-sample by 2 mode			//
// ==================================================================== //
FIRReturnCodes_t				FIR_proc_os2(FIRCtrl_t* psFIRCtrl)
{
	int*			piIn		= psFIRCtrl->piIn;
	unsigned int	uiInStep	= psFIRCtrl->uiInStep;
	int*			piOut		= psFIRCtrl->piOut;
	unsigned int	uiOutStep	= psFIRCtrl->uiOutStep;
	int*			piDelayB	= psFIRCtrl->piDelayB;
	int*			piDelayI	= psFIRCtrl->piDelayI;
	int*			piDelayW	= psFIRCtrl->piDelayW;
	unsigned int	uiDelayO	= psFIRCtrl->uiDelayO;
	int*			piCoefsB	= psFIRCtrl->piCoefs;
	unsigned int	uiNLoops	= psFIRCtrl->uiNLoops;
	int*			piData;
	int*			piCoefs;
	int				iData0, iData1;
	int				iCoef0, iCoef1;
	__int64			i64Acc0, i64Acc1;
	unsigned		ui, uj;

	for(ui = 0; ui < psFIRCtrl->uiNInSamples; ui++)
	{
		// Get new data sample to delay line (double write for circular buffer simulation) with step
		iData0					= *piIn;
		piIn					+= uiInStep;
		// Double write to simulate circular buffer
		*piDelayI				= iData0;
		*(piDelayI + uiDelayO)	= iData0;
		// Step delay (with circular simulation)
		piDelayI++;
		if(piDelayI >= piDelayW)
			piDelayI				= piDelayB;

		// Clear accumulators and set access pointers
		piData					= piDelayI;
		piCoefs					= piCoefsB;
		i64Acc0					= 0;
		i64Acc1					= 0;
		for(uj = 0; uj < uiNLoops; uj++)
		{
			// DUAL data read
			iData0					= *piData++;
			iData1					= *piData++;
			// DUAL coefs read (two phases for first data)
			iCoef0					= *piCoefs++;
			iCoef1					= *piCoefs++;
			// MACCs (for first data)
			MACC(&i64Acc0, iData0, iCoef0);
			MACC(&i64Acc1, iData0, iCoef1);
			// DUAL coefs read (two phases for second data)
			iCoef0					= *piCoefs++;
			iCoef1					= *piCoefs++;
			// MACCs (for second data)
			MACC(&i64Acc0, iData1, iCoef0);
			MACC(&i64Acc1, iData1, iCoef1);
		}

		// Saturate MACCs result
		LSAT30(&i64Acc0);
		LSAT30(&i64Acc1);
		// Extract 32bits result
		EXT30(&iData0, i64Acc0);
		EXT30(&iData1, i64Acc1);
	

		// Write output with step
		// NOTE OUTPUT WRITE ORDER: First iData1, then iData0
		*piOut					= iData1;
		piOut					+= uiOutStep;
		*piOut					= iData0;
		piOut					+= uiOutStep;
	}

	// Write delay line index back for next round
	psFIRCtrl->piDelayI		= piDelayI;

	return FIR_NO_ERROR;
}
  

// ==================================================================== //
// Function:		FIR_proc_sync										//
// Arguments:		FIRCtrl_t 	*psFIRCtrl: Ctrl strct.					//
// Return values:	FIR_NO_ERROR on success								//
//					FIR_ERROR on failure								//
// Description:		Processes the FIR in synchronous mode				//
// ==================================================================== //
FIRReturnCodes_t				FIR_proc_sync(FIRCtrl_t* psFIRCtrl)
{
	int*			piIn		= psFIRCtrl->piIn;
	unsigned int	uiInStep	= psFIRCtrl->uiInStep;
	int*			piOut		= psFIRCtrl->piOut;
	unsigned int	uiOutStep	= psFIRCtrl->uiOutStep;
	int*			piDelayB	= psFIRCtrl->piDelayB;
	int*			piDelayI	= psFIRCtrl->piDelayI;
	int*			piDelayW	= psFIRCtrl->piDelayW;
	unsigned int	uiDelayO	= psFIRCtrl->uiDelayO;
	int*			piCoefsB	= psFIRCtrl->piCoefs;
	unsigned int	uiNLoops	= psFIRCtrl->uiNLoops;
	int*			piData;
	int*			piCoefs;
	int				iData0, iData1;
	int				iCoef0, iCoef1;
	__int64			i64Acc;
	unsigned		ui, uj;

	for(ui = 0; ui < psFIRCtrl->uiNInSamples; ui++)
	{
		// Get new data sample to delay line (double write for circular buffer simulation) with step
		iData0					= *piIn;
		piIn					+= uiInStep;
		// Double write to simulate circular buffer
		*piDelayI				= iData0;
		*(piDelayI + uiDelayO)	= iData0;
		// Step delay (with circular simulation)
		piDelayI++;
		if(piDelayI >= piDelayW)
			piDelayI				= piDelayB;

		// Clear accumulator and set access pointers
		piData					= piDelayI;
		piCoefs					= piCoefsB;
		i64Acc					= 0;
		for(uj = 0; uj < uiNLoops; uj++)
		{
			// DUAL data read
			iData0					= *piData++;
			iData1					= *piData++;
			// DUAL coefs read
			iCoef0					= *piCoefs++;
			iCoef1					= *piCoefs++;
			// MACCs
			MACC(&i64Acc, iData0, iCoef0);
			MACC(&i64Acc, iData1, iCoef1);
		}

		// Saturate MACCs result
		LSAT30(&i64Acc);
		// Extract 32bits result
		EXT30(&iData0, i64Acc);

		// Write output with step
		*piOut					= iData0;
		piOut					+= uiOutStep;
	}

	// Write delay line index back for next round
	psFIRCtrl->piDelayI		= piDelayI;

	return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:		FIR_proc_ds2										//
// Arguments:		FIRCtrl_t 	*psFIRCtrl: Ctrl strct.					//
// Return values:	FIR_NO_ERROR on success								//
//					FIR_ERROR on failure								//
// Description:		Processes the FIR in down-sample by 2 mode			//
// ==================================================================== //
FIRReturnCodes_t				FIR_proc_ds2(FIRCtrl_t* psFIRCtrl)
{
	int*			piIn		= psFIRCtrl->piIn;
	unsigned int	uiInStep	= psFIRCtrl->uiInStep;
	int*			piOut		= psFIRCtrl->piOut;
	unsigned int	uiOutStep	= psFIRCtrl->uiOutStep;
	int*			piDelayB	= psFIRCtrl->piDelayB;
	int*			piDelayI	= psFIRCtrl->piDelayI;
	int*			piDelayW	= psFIRCtrl->piDelayW;
	unsigned int	uiDelayO	= psFIRCtrl->uiDelayO;
	int*			piCoefsB	= psFIRCtrl->piCoefs;
	unsigned int	uiNLoops	= psFIRCtrl->uiNLoops;
	int*			piData;
	int*			piCoefs;
	int				iData0, iData1;
	int				iCoef0, iCoef1;
	__int64			i64Acc;
	unsigned		ui, uj;

	for(ui = 0; ui < psFIRCtrl->uiNInSamples>>1; ui++)
	{
		// Get two new data samples to delay line (double write for circular buffer simulation), with input buffer step
		iData0					= *piIn;
		piIn					+= uiInStep;
		iData1					= *piIn;
		piIn					+= uiInStep;
		// Double write to simulate circular buffer with DUAL store instruction
		*piDelayI				= iData0;
		*(piDelayI + 1)			= iData1;
		*(piDelayI + uiDelayO)	= iData0;
		*(piDelayI + uiDelayO + 1)	= iData1;
		// Step delay with circular simulation
		piDelayI				+= 2;
		if(piDelayI >= piDelayW)						
			piDelayI				= piDelayB;	
		
		// Clear accumulator and set access pointers
		piData					= piDelayI;
		piCoefs					= piCoefsB;
		i64Acc					= 0;
		for(uj = 0; uj < uiNLoops; uj++)
		{
			// DUAL data read
			iData0					= *piData++;
			iData1					= *piData++;
			// DUAL coefs read
			iCoef0					= *piCoefs++;
			iCoef1					= *piCoefs++;
			// MACCs
			MACC(&i64Acc, iData0, iCoef0);
			MACC(&i64Acc, iData1, iCoef1);
		}

		// Saturate MACCs result
		LSAT30(&i64Acc);
		// Extract 32bits result
		EXT30(&iData0, i64Acc);

		// Write output with step
		*piOut					= iData0;
		piOut					+= uiOutStep;
	}

	// Write delay line index back for next round
	psFIRCtrl->piDelayI		= piDelayI;

	return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:		PPFIR_init_from_desc								//
// Arguments:		PPFIRCtrl_t 	*psPPFIRCtrl: Ctrl strct.			//
//					PPFIRDescriptor_t *psPPFIRDescriptor: Desc. strct.	//
// Return values:	FIR_NO_ERROR on success								//
//					FIR_ERROR on failure								//
// Description:		Inits the PPFIR from the Descriptor					//
// ==================================================================== //
FIRReturnCodes_t				PPFIR_init_from_desc(PPFIRCtrl_t* psPPFIRCtrl, PPFIRDescriptor_t* psPPFIRDescriptor)
{
	unsigned int		uiPhaseLength;

	// Check if PPFIR is disabled (this is given by the number of coefficients being zero)
	if( psPPFIRDescriptor->uiNCoefs == 0)
	{
		// Number of coefficients is zero, so disable FIR
		psPPFIRCtrl->eEnable			= FIR_OFF;
		psPPFIRCtrl->uiNOutSamples		= 0;						
		psPPFIRCtrl->uiDelayL			= 0;						
		psPPFIRCtrl->piDelayW			= 0;	
		psPPFIRCtrl->uiDelayO			= 0;
		psPPFIRCtrl->uiNLoops			= 0;						
		psPPFIRCtrl->uiNCoefs			= 0;
		psPPFIRCtrl->piCoefs			= 0;
		psPPFIRCtrl->uiNPhases			= 0;
		psPPFIRCtrl->uiPhaseStep		= 0;
		psPPFIRCtrl->uiCoefsPhaseStep	= 0;
		psPPFIRCtrl->uiCoefsPhase		= 0;
		return FIR_NO_ERROR;
	}

	// Check that delay line base has been set
	if(psPPFIRCtrl->piDelayB == 0)
		return FIR_ERROR;

	// Check that number of samples has been set and is a multiple of 2
	if(psPPFIRCtrl->uiNInSamples == 0)
		return FIR_ERROR;

	// Check the input and output samples steps have been set
	if(psPPFIRCtrl->uiInStep == 0)
		return FIR_ERROR;
	if(psPPFIRCtrl->uiOutStep == 0)
		return FIR_ERROR;

	// Check that phase step has been set properly
	if(psPPFIRCtrl->uiPhaseStep == 0)
		return FIR_ERROR;

	// Check that number of phases and number of coefficients are compatible
	uiPhaseLength					= (psPPFIRDescriptor->uiNCoefs / psPPFIRDescriptor->uiNPhases);
	if(uiPhaseLength == 0)
		return FIR_ERROR;
	if((uiPhaseLength * psPPFIRDescriptor->uiNPhases) != psPPFIRDescriptor->uiNCoefs)
		return FIR_ERROR;
	if((uiPhaseLength & 0x1) != 0)
		return FIR_ERROR;

	// Setup PPFIR
	psPPFIRCtrl->eEnable			= FIR_ON;
	psPPFIRCtrl->uiDelayL			= uiPhaseLength<<1;									// Double length for circular buffer simulation					
	psPPFIRCtrl->piDelayW			= psPPFIRCtrl->piDelayB + uiPhaseLength;	
	psPPFIRCtrl->uiDelayO			= uiPhaseLength;
	psPPFIRCtrl->uiNLoops			= uiPhaseLength>>1;									// Due to 2 x 32bits read for data and coefs per inner loop						
	psPPFIRCtrl->uiNCoefs			= psPPFIRDescriptor->uiNCoefs;
	psPPFIRCtrl->piCoefs			= psPPFIRDescriptor->piCoefs;
	psPPFIRCtrl->uiNPhases			= psPPFIRDescriptor->uiNPhases;
	psPPFIRCtrl->uiCoefsPhaseStep	= psPPFIRCtrl->uiPhaseStep * uiPhaseLength;			// Length (number of coefs) of a phase step

	// Sync the PPFIR
	if(PPFIR_sync(psPPFIRCtrl) != FIR_NO_ERROR)
		return FIR_ERROR;

	return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:		PPFIR_snyc											//
// Arguments:		PPFIRCtrl_t 	*psPPFIRCtrl: Ctrl strct.			//
// Return values:	FIR_NO_ERROR on success								//
//					FIR_ERROR on failure								//
// Description:		Syncs the PPFIR polyphase filter 					//
// ==================================================================== //
FIRReturnCodes_t				PPFIR_sync(PPFIRCtrl_t* psPPFIRCtrl)
{
	unsigned int		ui;

	if(psPPFIRCtrl->eEnable == FIR_ON)
	{
		// Set delay line index back to base
		psPPFIRCtrl->piDelayI	= psPPFIRCtrl->piDelayB;

		// Clear delay line
		for(ui = 0; ui < psPPFIRCtrl->uiDelayL; ui++)
			psPPFIRCtrl->piDelayB[ui]	= FIR_STATE_INIT;

		// Set coefficients phase to zero
		psPPFIRCtrl->uiCoefsPhase		= 0;

		// Clear number of output sampes
		psPPFIRCtrl->uiNOutSamples		= 0;
	}
	

	return FIR_NO_ERROR;
}


// ==================================================================== //
// Function:		PPFIR_proc											//
// Arguments:		PPFIRCtrl_t 	*psPPFIRCtrl: Ctrl strct.			//
// Return values:	FIR_NO_ERROR on success								//
//					FIR_ERROR on failure								//
// Description:		Processes the PPFIR polyphase filter 				//
// ==================================================================== //
FIRReturnCodes_t				PPFIR_proc(PPFIRCtrl_t* psPPFIRCtrl)
{
	int*			piIn				= psPPFIRCtrl->piIn;
	unsigned int	uiInStep			= psPPFIRCtrl->uiInStep;
	int*			piOut				= psPPFIRCtrl->piOut;
	unsigned int	uiOutStep			= psPPFIRCtrl->uiOutStep;
	int*			piDelayB			= psPPFIRCtrl->piDelayB;
	int*			piDelayI			= psPPFIRCtrl->piDelayI;
	int*			piDelayW			= psPPFIRCtrl->piDelayW;
	unsigned int	uiDelayO			= psPPFIRCtrl->uiDelayO;
	int*			piCoefsB			= psPPFIRCtrl->piCoefs;
	unsigned int	uiNLoops			= psPPFIRCtrl->uiNLoops;
	unsigned int	uiNCoefs			= psPPFIRCtrl->uiNCoefs;
	unsigned int	uiCoefsPhase		= psPPFIRCtrl->uiCoefsPhase;
	unsigned int	uiCoefsPhaseStep	= psPPFIRCtrl->uiCoefsPhaseStep;
	int*			piData;
	int*			piCoefs;
	int				iData0, iData1;
	int				iCoef0, iCoef1;
	__int64			i64Acc;
	unsigned int	uiNOutSamples		= 0;
	unsigned		ui, uj;


	for(ui = 0; ui < psPPFIRCtrl->uiNInSamples; ui++)
	{
		// Get new data sample to delay line (double write for circular buffer simulation) with step
		iData0					= *piIn;
		piIn					+= uiInStep;
		// Double write to simulate circular buffer
		*piDelayI				= iData0;
		*(piDelayI + uiDelayO)	= iData0;
		// Step delay (with circular simulation)
		piDelayI++;
		if(piDelayI >= piDelayW)
			piDelayI				= piDelayB;

		// Do while the current phase coefficient pointer points to phase coefficients
		// This is equivalent to know if the output sample is between the current and next input sample
		while(uiCoefsPhase < uiNCoefs)
		{
			// Clear accumulator and set access pointers
			piData					= piDelayI;
			piCoefs					= piCoefsB + uiCoefsPhase;
			i64Acc					= 0;
			for(uj = 0; uj < uiNLoops; uj++)
			{
				// DUAL data read
				iData0					= *piData++;
				iData1					= *piData++;
				// DUAL coefs read
				iCoef0					= *piCoefs++;
				iCoef1					= *piCoefs++;
				// MACCs
				MACC(&i64Acc, iData0, iCoef0);
				MACC(&i64Acc, iData1, iCoef1);
			}

			// Saturate MACCs result
			LSAT30(&i64Acc);
			// Extract 32bits result
			EXT30(&iData0, i64Acc);


			// Write output with step
			*piOut					= iData0;
			piOut					+= uiOutStep;

			// Step phase coefficient offset for next output phase
			uiCoefsPhase			+= uiCoefsPhaseStep;

			// Increase output sample counter
			uiNOutSamples++;
		}

		// Reduce phase for next input sample (one input sample corresponds to all coefficients in the PP filter)
		uiCoefsPhase				-= uiNCoefs;
	}

	// Write delay line index back for next round
	psPPFIRCtrl->piDelayI		= piDelayI;
	// Write coefs phase back for next round
	psPPFIRCtrl->uiCoefsPhase	= uiCoefsPhase;
	// Write number of samples
	psPPFIRCtrl->uiNOutSamples	= uiNOutSamples;

	return FIR_NO_ERROR;
}

