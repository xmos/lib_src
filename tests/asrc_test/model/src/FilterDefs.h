// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//	
// File: FilterDefs.h
//
// Filters Definitions definition file for the ASRC
//
// Target:	MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================

#ifndef _FILTER_DEFS_H_
#define _FILTER_DEFS_H_

	// ===========================================================================
	//
	// Defines
	//
	// ===========================================================================
	
	// General defines
	// ---------------
	// FIR filters IDs
	#define		FILTER_DEFS_FIR_BL_ID				0			// ID of BL FIR filter
	#define		FILTER_DEFS_FIR_BL9644_ID			1			// ID of BL9644 FIR filter
	#define		FILTER_DEFS_FIR_BL8848_ID			2			// ID of BL8848 FIR filter
	#define		FITLER_DEFS_FIR_BLF_ID				3			// ID of BLF FIR filter
	#define		FITLER_DEFS_FIR_BL19288_ID			4			// ID of BL19288 FIR filter
	#define		FILTER_DEFS_FIR_BL17696_ID			5			// ID of BL17696 FIR filter
	#define		FILTER_DEFS_FIR_UP_ID				6			// ID of UP FIR filter
	#define		FILTER_DEFS_FIR_UP4844_ID			7			// ID of UP4844 FIR filter
	#define		FILTER_DEFS_FIR_UPF_ID				8			// ID of UPF FIR filter
	#define		FILTER_DEFS_FIR_UP192176_ID			9			// ID of UP192176 FIR filter
	#define		FILTER_DEFS_FIR_DS_ID				10			// ID of DS FIR filter
	#define		FILTER_DEFS_FIR_NONE_ID				11			// ID of disabled FIR filter

	#define		FILTER_DEFS_N_FIR_ID				(FILTER_DEFS_FIR_NONE_ID + 1)		// Number of FIR IDs corresponding to real FIRs

	// FIR filter number of taps
	#define		FILTER_DEFS_FIR_BL_N_TAPS			144			// Number of taps of BL FIR filter
	#define		FILTER_DEFS_FIR_BL9644_N_TAPS		160			// Number of taps of BL9644 FIR filter
	#define		FILTER_DEFS_FIR_BL8848_N_TAPS		144			// Number of taps of BL8848 filter
	#define		FILTER_DEFS_FIR_BLF_N_TAPS			96			// Number of taps of BLF FIR filter
	#define		FILTER_DEFS_FIR_BL19288_N_TAPS		96			// Number of taps of BL19288 filter
	#define		FILTER_DEFS_FIR_BL17696_N_TAPS		96			// Number of taps of BL17696 filter
	#define		FILTER_DEFS_FIR_UP_N_TAPS			144			// Number of taps of UP filter
	#define		FILTER_DEFS_FIR_UP4844_N_TAPS		160			// Number of taps of UP4844 filter
	#define		FILTER_DEFS_FIR_UPF_N_TAPS			96			// Number of taps of UPF filter
	#define		FILTER_DEFS_FIR_UP192176_N_TAPS		96			// Number of taps of UP192176 filter
	#define		FILTER_DEFS_FIR_DS_N_TAPS			32			// Number of taps of DS filter
	
	#define		FILTER_DEFS_FIR_MAX_TAPS_LONG		FILTER_DEFS_FIR_BL9644_N_TAPS		// Maximum number of taps for long FIR filter
	#define		FILTER_DEFS_FIR_MAX_TAPS_SHORT		FILTER_DEFS_FIR_DS_N_TAPS			// Maximum number of taps for short FIR filter			
	
	// FIR filter coefficients files
	#define		FILTER_DEFS_FIR_BL_FILE				"FilterData/BL.dat"			// Coefficients file for BL FIR filter
	#define		FILTER_DEFS_FIR_BL9644_FILE			"FilterData/BL9644.dat"		// Coefficients file for BL9644 FIR filter
	#define		FILTER_DEFS_FIR_BL8848_FILE			"FilterData/BL8848.dat"		// Coefficients file for BL8848 filter
	#define		FILTER_DEFS_FIR_BLF_FILE			"FilterData/BLF.dat"		// Coefficients file for BLF FIR filter
	#define		FILTER_DEFS_FIR_BL19288_FILE		"FilterData/BL19288.dat"	// Coefficients file for BL19288 FIR filter
	#define		FILTER_DEFS_FIR_BL17696_FILE		"FilterData/BL17696.dat"	// Coefficients file for BL17696 filter
	#define		FILTER_DEFS_FIR_UP_FILE				"FilterData/UP.dat"			// Coefficients file for UP filter
	#define		FILTER_DEFS_FIR_UP4844_FILE			"FilterData/UP4844.dat"		// Coefficients file for UP4844 filter
	#define		FILTER_DEFS_FIR_UPF_FILE			"FilterData/UPF.dat"		// Coefficients file for UPF filter
	#define		FILTER_DEFS_FIR_UP192176_FILE		"FilterData/UP192176.dat"	// Coefficients file for UP192176 filter
	#define		FILTER_DEFS_FIR_DS_FILE				"FilterData/DS.dat"			// Coefficients file for DS filter
	
	// ADFIR filter number of taps and phases
	#define		FILTER_DEFS_ADFIR_PROTOTYPE_N_TAPS	1920																		// Number of taps of ADFIR filter prototype

	#define		FILTER_DEFS_ADFIR_N_PHASES			128																			// Number of phases of ADFIR filter
	#define		FILTER_DEFS_ADFIR_PHASE_N_TAPS		((FILTER_DEFS_ADFIR_PROTOTYPE_N_TAPS / FILTER_DEFS_ADFIR_N_PHASES) + 1)		// Number of taps per phase
	#define		FILTER_DEFS_ADFIR_N_TAPS			(FILTER_DEFS_ADFIR_N_PHASES + 2) * FILTER_DEFS_ADFIR_PHASE_N_TAPS			// Here 130 phases of 16 taps = 2080 coefs

	// ADFIR filter coefficients files
	#define		FILTER_DEFS_ADFIR_PROTOTYPE_FILE	"FilterData/ADFir.dat"		// Coefficients file for the ADFIR filter (prototype)
	

	
	// ===========================================================================
	//
	// Variables
	//
	// ===========================================================================
	
	// FIR filters descriptors (ordered by ID)
	extern		FIRDescriptor_t						sFirDescriptor[FILTER_DEFS_N_FIR_ID];
	// FIR filters coefficients
	extern		int									iFirBLCoefs[FILTER_DEFS_FIR_BL_N_TAPS];
	extern		int									iFirBL9644Coefs[FILTER_DEFS_FIR_BL9644_N_TAPS];
	extern		int									iFirBL8848[FILTER_DEFS_FIR_BL8848_N_TAPS];
	extern		int									iFirBLFCoefs[FILTER_DEFS_FIR_BLF_N_TAPS];
	extern		int									iFirBL19288Coefs[FILTER_DEFS_FIR_BL19288_N_TAPS];
	extern		int									iFirBL17696[FILTER_DEFS_FIR_BL17696_N_TAPS];
	extern		int									iFirUPCoefs[FILTER_DEFS_FIR_UP_N_TAPS];
	extern		int									iFirUP4844Coefs[FILTER_DEFS_FIR_UP4844_N_TAPS];
	extern		int									iFirUPFCoefs[FILTER_DEFS_FIR_UPF_N_TAPS];
	extern		int									iFirUP192176Coefs[FILTER_DEFS_FIR_UP192176_N_TAPS];
	extern		int									iFirDSCoefs[FILTER_DEFS_FIR_DS_N_TAPS];
	
	// ADFIR filter descriptor
	extern		ADFIRDescriptor_t					sADFirDescriptor;
	// ADFIR filters coefficients
	extern		int									iADFirPrototypeCoefs[FILTER_DEFS_ADFIR_PROTOTYPE_N_TAPS];
	extern		int									iADFirCoefs[FILTER_DEFS_ADFIR_N_PHASES + 2][FILTER_DEFS_ADFIR_PHASE_N_TAPS];
	

	// ===========================================================================
	//
	// TypeDefs
	//
	// ===========================================================================

	
#endif // _FILTER_DEFS_H_