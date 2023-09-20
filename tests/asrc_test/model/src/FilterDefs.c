// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//	
// File: FilterDefs.c
//
// Filters Definitions implementation file for the ASRC
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
// FIR and FilterDefs includes
#include "FIR.h"
#include "FilterDefs.h"

// ===========================================================================
//
// Defines
//
// ===========================================================================




// ===========================================================================
//
// Variables
//
// ===========================================================================

// FIR filters descriptors (ordered by ID)
FIRDescriptor_t			sFirDescriptor[FILTER_DEFS_N_FIR_ID] = 
{
	{FIR_TYPE_SYNC,							FILTER_DEFS_FIR_BL_N_TAPS,			iFirBLCoefs},			// FILTER_DEFS_FIR_BL_ID
	{FIR_TYPE_SYNC,							FILTER_DEFS_FIR_BL9644_N_TAPS,		iFirBL9644Coefs},		// FILTER_DEFS_FIR_BL9644_ID
	{FIR_TYPE_SYNC,							FILTER_DEFS_FIR_BL8848_N_TAPS,		iFirBL8848},			// FILTER_DEFS_FIR_BL8848_ID
	{FIR_TYPE_SYNC,							FILTER_DEFS_FIR_BLF_N_TAPS,			iFirBLFCoefs},			// FILTER_DEFS_FIR_BLF_ID
	{FIR_TYPE_SYNC,							FILTER_DEFS_FIR_BL19288_N_TAPS,		iFirBL19288Coefs},		// FILTER_DEFS_FIR_BL19288_ID
	{FIR_TYPE_SYNC,							FILTER_DEFS_FIR_BL17696_N_TAPS,		iFirBL17696},			// FILTER_DEFS_FIR_BL17696_ID
	{FIR_TYPE_OS2,							FILTER_DEFS_FIR_UP_N_TAPS,			iFirUPCoefs},			// FILTER_DEFS_FIR_UP_ID
	{FIR_TYPE_OS2,							FILTER_DEFS_FIR_UP4844_N_TAPS,		iFirUP4844Coefs},		// FILTER_DEFS_FIR_UP4844_ID
	{FIR_TYPE_OS2,							FILTER_DEFS_FIR_UPF_N_TAPS,			iFirUPFCoefs},			// FILTER_DEFS_FIR_UPF_ID
	{FIR_TYPE_OS2,							FILTER_DEFS_FIR_UP192176_N_TAPS,	iFirUP192176Coefs},		// FILTER_DEFS_FIR_UP192176_ID
	{FIR_TYPE_DS2,							FILTER_DEFS_FIR_DS_N_TAPS,			iFirDSCoefs},			// FILTER_DEFS_FIR_DS_ID
	{FIR_TYPE_SYNC,							0,									0}						// FILTER_DEFS_FIR_NONE_ID
};

// FIR filters coefficients
int					iFirBLCoefs[FILTER_DEFS_FIR_BL_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_BL_FILE	
};
int					iFirBL9644Coefs[FILTER_DEFS_FIR_BL9644_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_BL9644_FILE	
};
int					iFirBL8848[FILTER_DEFS_FIR_BL8848_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_BL8848_FILE	
};
int					iFirBLFCoefs[FILTER_DEFS_FIR_BLF_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_BLF_FILE	
};
int					iFirBL19288Coefs[FILTER_DEFS_FIR_BL19288_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_BL19288_FILE	
};
int					iFirBL17696[FILTER_DEFS_FIR_BL17696_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_BL17696_FILE	
};
int					iFirUPCoefs[FILTER_DEFS_FIR_UP_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_UP_FILE	
};
int					iFirUP4844Coefs[FILTER_DEFS_FIR_UP4844_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_UP4844_FILE	
};
int					iFirUPFCoefs[FILTER_DEFS_FIR_UPF_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_UPF_FILE	
};
int					iFirUP192176Coefs[FILTER_DEFS_FIR_UP192176_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_UP192176_FILE	
};
int					iFirDSCoefs[FILTER_DEFS_FIR_DS_N_TAPS]	 = {
	#include FILTER_DEFS_FIR_DS_FILE	
};



// ADFIR filters descriptor
ADFIRDescriptor_t		sADFirDescriptor = 
{
	FILTER_DEFS_ADFIR_PHASE_N_TAPS,		FILTER_DEFS_ADFIR_N_PHASES + 2,		iADFirCoefs
};

// ADFIR filter coefficients
int					iADFirPrototypeCoefs[FILTER_DEFS_ADFIR_PROTOTYPE_N_TAPS]  = {
	#include FILTER_DEFS_ADFIR_PROTOTYPE_FILE
};
int					iADFirCoefs[FILTER_DEFS_ADFIR_N_PHASES + 2][FILTER_DEFS_ADFIR_PHASE_N_TAPS];
	





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





