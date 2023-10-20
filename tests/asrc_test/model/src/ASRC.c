// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// File: ASRC.c
//
// Top level implementation file for the ASRC
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
// ASRC include
#include "ASRC.h"

// ===========================================================================
//
// Defines
//
// ===========================================================================


// State init value
#define		ASRC_STATE_INIT							0

// Nominal Fs Ratio values
#define		ASRC_FS_44_F							(44100.0)
#define		ASRC_FS_48_F							(48000.0)
#define		ASRC_FS_88_F							(88200.0)
#define		ASRC_FS_96_F							(96000.0)
#define		ASRC_FS_176_F							(176400.0)
#define		ASRC_FS_192_F							(192000.0)

#define		ASRC_FS_RATIO_44_44 						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_44_44_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_44_48 						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_44_48_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_44_88						    (uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_44_88_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_44_96						    (uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_44_96_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_44_176						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_44_176_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_44_192						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_44_192_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_44_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))

#define		ASRC_FS_RATIO_48_44						    (uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_48_44_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_48_48						    (uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_48_48_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_48_88						    (uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_48_88_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_48_96						    (uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_48_96_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_48_176						(uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_48_176_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_48_192						(uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_48_192_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_48_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))

#define		ASRC_FS_RATIO_88_44						    (uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_88_44_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_88_48						    (uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_88_48_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_88_88						    (uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_88_88_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_88_96						    (uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_88_96_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_88_176						(uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_88_176_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_88_192						(uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_88_192_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_88_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))

#define		ASRC_FS_RATIO_96_44						    (uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_96_44_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_96_48						    (uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_96_48_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_96_88						    (uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_96_88_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_96_96						    (uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_96_96_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_96_176						(uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_96_176_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_96_192						(uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_96_192_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_96_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))

#define		ASRC_FS_RATIO_176_44						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_176_44_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_176_48						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_176_48_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_176_88						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_176_88_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_176_96						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_176_96_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_176_176						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_176_176_LO					(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_176_192						(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_176_192_LO					(uint32_t)((unsigned long long)(((double)ASRC_FS_176_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))

#define		ASRC_FS_RATIO_192_44						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_192_44_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_44_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_192_48						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_192_48_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_48_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_192_88						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_192_88_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_88_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_192_96						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_192_96_LO						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_96_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_192_176						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_192_176_LO					(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_176_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))
#define		ASRC_FS_RATIO_192_192						(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))) >> 32)
#define		ASRC_FS_RATIO_192_192_LO					(uint32_t)((unsigned long long)(((double)ASRC_FS_192_F / ASRC_FS_192_F) * ((unsigned long long)1 << (ASRC_FS_RATIO_UNIT_BIT + 32))))


#define		ASRC_FS_RATIO_MIN_FACTOR				(0.99)
#define		ASRC_FS_RATIO_MAX_FACTOR				(1.01)

#define		ASRC_ADFIR_INITIAL_PHASE				32

// Time ratio shift values
#define		ASRC_FS_RATIO_UNIT_BIT					28
#define		ASRC_FS_RATIO_PHASE_N_BITS				7
#define		ASRC_FS_RATIO_BASE_SHIFT				(ASRC_FS_RATIO_UNIT_BIT - ASRC_FS_RATIO_PHASE_N_BITS)
#define		ASRC_FS_RATIO_SHIFT_M1					(ASRC_FS_RATIO_BASE_SHIFT + 1)
#define		ASRC_FS_RATIO_SHIFT_0					(ASRC_FS_RATIO_BASE_SHIFT)
#define		ASRC_FS_RATIO_SHIFT_P1					(ASRC_FS_RATIO_BASE_SHIFT - 1)


// Random number generator / dithering
#define		ASRC_R_CONS								32767
#define		ASRC_R_BASE								1664525
#define		ASRC_RPDF_BITS_SHIFT					16						// Shift to select bits in pseudo-random number
#define		ASRC_RPDF_MASK							0x0000007F				// For dithering at 24bits (in 2.30)
#define		ASRC_DATA24_MASK						0xFFFFFF00				// Mask for 24bits data (once rescaled to 1.31)
#define		ASRC_DITHER_BIAS						0xFFFFFFC0				// TPDF dither bias for compensating masking at 24bits but expressed in 2.30


// Cycle counters
#define		ASRC_FIR_OS2_OVERHEAD_CYCLE_COUNT		(15.0)
#define		ASRC_FIR_OS2_TAP_CYCLE_COUNT			(2.125)
#define		ASRC_FIR_DS2_OVERHEAD_CYCLE_COUNT		(15.0)
#define		ASRC_FIR_DS2_TAP_CYCLE_COUNT			(2.625)
#define		ASRC_FIR_SYNC_OVERHEAD_CYCLE_COUNT		(15.0)
#define		ASRC_FIR_SYNC_TAP_CYCLE_COUNT			(2.625)

#define		ASRC_ADFIR_IN_SPL_CYCLE_COUNT			(8.0)
#define		ASRC_ADFIR_TIME_CHECK_CYCLE_COUNT		(5.0)
#define		ASRC_ADFIR_TIME_SAMPLE_CYCLE_COUNT		(15.0 + 15.0 + 90.0)
#define		ASRC_ADFIR_MACC_OVERHEAD_CYCLE_COUNT	(8.0)
#define		ASRC_ADFIR_MACC_TAP_CYCLE_COUNT			(2.5)

#define		ASRC_DITHER_SAMPLE_COUNT				(20.0)



// ===========================================================================
//
// Variables
//
// ===========================================================================

ASRCFiltersIDs_t		sFiltersIDs[ASRC_N_FS][ASRC_N_FS] =				// Filter configuration table [Fsin][Fsout]
{
	{	// Fsin = 44.1kHz
		// F1							F2
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 48kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID}		// Fsout = 192kHz
	},
	{	// Fsin = 48kHz
		// F1							F2
		{FILTER_DEFS_FIR_UP4844_ID,		FILTER_DEFS_FIR_NONE_ID},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 48kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID}		// Fsout = 192kHz
	},
	{	// Fsin = 88.2kHz
		// F1							F2
		{FILTER_DEFS_FIR_BL_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_BL8848_ID,		FILTER_DEFS_FIR_NONE_ID},		// Fsout = 48kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID}		// Fsout = 192kHz
	},
	{	// Fsin = 96kHz
		// F1							F2
		{FILTER_DEFS_FIR_BL9644_ID,		FILTER_DEFS_FIR_NONE_ID},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_BL_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 48kHz
		{FILTER_DEFS_FIR_UP4844_ID,		FILTER_DEFS_FIR_NONE_ID},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UP_ID,			FILTER_DEFS_FIR_NONE_ID}		// Fsout = 192kHz
	},
	{	// Fsin = 176.4kHz
		// F1							F2
		{FILTER_DEFS_FIR_DS_ID,			FILTER_DEFS_FIR_BL_ID},			// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_DS_ID,			FILTER_DEFS_FIR_BL8848_ID},		// Fsout = 48kHz
		{FITLER_DEFS_FIR_BLF_ID,		FILTER_DEFS_FIR_NONE_ID,},		// Fsout = 88.2kHz
		{FILTER_DEFS_FIR_BL17696_ID,	FILTER_DEFS_FIR_NONE_ID},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UPF_ID,		FILTER_DEFS_FIR_NONE_ID},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UPF_ID,		FILTER_DEFS_FIR_NONE_ID}		// Fsout = 192kHz
	},
	{	// Fsin = 192kHz
		// F1							F2
		{FILTER_DEFS_FIR_DS_ID,			FILTER_DEFS_FIR_BL9644_ID},		// Fsout = 44.1kHz
		{FILTER_DEFS_FIR_DS_ID,			FILTER_DEFS_FIR_BL_ID},			// Fsout = 48kHz
		{FITLER_DEFS_FIR_BL19288_ID,	FILTER_DEFS_FIR_NONE_ID},		// Fsout = 88.2kHz
		{FITLER_DEFS_FIR_BLF_ID,		FILTER_DEFS_FIR_NONE_ID},		// Fsout = 96kHz
		{FILTER_DEFS_FIR_UP192176_ID,	FILTER_DEFS_FIR_NONE_ID},		// Fsout = 176.4kHz
		{FILTER_DEFS_FIR_UPF_ID,		FILTER_DEFS_FIR_NONE_ID}		// Fsout = 192kHz
	}
};

ASRCFsRatioConfigs_t		sFsRatioConfigs[ASRC_N_FS][ASRC_N_FS] =				// Fs ratio configuration table [Fsin][Fsout]
{
	{	// Fsin = 44.1kHz
		// Nominal Fs Ratio				// Minimal Fs Ratio														// Maximal Fs Ratio														Shift for time step
		{ASRC_FS_RATIO_44_44,		ASRC_FS_RATIO_44_44_LO, 	(unsigned int)(ASRC_FS_RATIO_44_44 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_44_44 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 44.1kHz
		{ASRC_FS_RATIO_44_48, ASRC_FS_RATIO_44_48_LO,			(unsigned int)(ASRC_FS_RATIO_44_48 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_44_48 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 48kHz
		{ASRC_FS_RATIO_44_88, ASRC_FS_RATIO_44_88_LO,			(unsigned int)(ASRC_FS_RATIO_44_88 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_44_88 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 88.2kHz
		{ASRC_FS_RATIO_44_96, ASRC_FS_RATIO_44_96_LO,			(unsigned int)(ASRC_FS_RATIO_44_96 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_44_96 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 96kHz
		{ASRC_FS_RATIO_44_176, ASRC_FS_RATIO_44_176_LO,			(unsigned int)(ASRC_FS_RATIO_44_176 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_44_176 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 176.4kHz
		{ASRC_FS_RATIO_44_192, ASRC_FS_RATIO_44_192_LO,			(unsigned int)(ASRC_FS_RATIO_44_192 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_44_192 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1}			// Fsout = 192kHz
	},
	{	// Fsin = 48kHz
		// Nominal Fs Ratio				// Minimal Fs Ratio														// Maximal Fs Ratio														Shift for time step
		{ASRC_FS_RATIO_48_44,	ASRC_FS_RATIO_48_44_LO,		(unsigned int)(ASRC_FS_RATIO_48_44 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_48_44 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 44.1kHz
		{ASRC_FS_RATIO_48_48, ASRC_FS_RATIO_48_48_LO,			(unsigned int)(ASRC_FS_RATIO_48_48 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_48_48 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 48kHz
		{ASRC_FS_RATIO_48_88, ASRC_FS_RATIO_48_88_LO,			(unsigned int)(ASRC_FS_RATIO_48_88 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_48_88 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 88.2kHz
		{ASRC_FS_RATIO_48_96, ASRC_FS_RATIO_48_96_LO,			(unsigned int)(ASRC_FS_RATIO_48_96 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_48_96 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 96kHz
		{ASRC_FS_RATIO_48_176, ASRC_FS_RATIO_48_176_LO,			(unsigned int)(ASRC_FS_RATIO_48_176 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_48_176 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 176.4kHz
		{ASRC_FS_RATIO_48_192, ASRC_FS_RATIO_48_192_LO,			(unsigned int)(ASRC_FS_RATIO_48_192 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_48_192 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1}			// Fsout = 192kHz
	},
	{	// Fsin = 88.2kHz
		// Nominal Fs Ratio				// Minimal Fs Ratio														// Maximal Fs Ratio														Shift for time step
		{ASRC_FS_RATIO_88_44,	ASRC_FS_RATIO_88_44_LO,		(unsigned int)(ASRC_FS_RATIO_88_44 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_88_44 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_0},			// Fsout = 44.1kHz
		{ASRC_FS_RATIO_88_48,	ASRC_FS_RATIO_88_48_LO,		(unsigned int)(ASRC_FS_RATIO_88_48 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_88_48 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_0},			// Fsout = 48kHz
		{ASRC_FS_RATIO_88_88, ASRC_FS_RATIO_88_88_LO,			(unsigned int)(ASRC_FS_RATIO_88_88 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_88_88 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 88.2kHz
		{ASRC_FS_RATIO_88_96, ASRC_FS_RATIO_88_96_LO,			(unsigned int)(ASRC_FS_RATIO_88_96 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_88_96 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 96kHz
		{ASRC_FS_RATIO_88_176, ASRC_FS_RATIO_88_176_LO,			(unsigned int)(ASRC_FS_RATIO_88_176 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_88_176 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 176.4kHz
		{ASRC_FS_RATIO_88_192, ASRC_FS_RATIO_88_192_LO,			(unsigned int)(ASRC_FS_RATIO_88_192 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_88_192 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1}			// Fsout = 192kHz
	},
	{	// Fsin = 96kHz
		// Nominal Fs Ratio				// Minimal Fs Ratio														// Maximal Fs Ratio														Shift for time step
		{ASRC_FS_RATIO_96_44,	ASRC_FS_RATIO_96_44_LO,		(unsigned int)(ASRC_FS_RATIO_96_44 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_96_44 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_0},			// Fsout = 44.1kHz
		{ASRC_FS_RATIO_96_48, ASRC_FS_RATIO_96_48_LO,			(unsigned int)(ASRC_FS_RATIO_96_48 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_96_48 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_0},			// Fsout = 48kHz
		{ASRC_FS_RATIO_96_88, ASRC_FS_RATIO_96_88_LO,			(unsigned int)(ASRC_FS_RATIO_96_88 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_96_88 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 88.2kHz
		{ASRC_FS_RATIO_96_96,	ASRC_FS_RATIO_96_96_LO,		(unsigned int)(ASRC_FS_RATIO_96_96 * ASRC_FS_RATIO_MIN_FACTOR),			(unsigned int)(ASRC_FS_RATIO_96_96 * ASRC_FS_RATIO_MAX_FACTOR),			ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 96kHz
		{ASRC_FS_RATIO_96_176,	ASRC_FS_RATIO_96_176_LO,		(unsigned int)(ASRC_FS_RATIO_96_176 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_96_176 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 176.4kHz
		{ASRC_FS_RATIO_96_192,	ASRC_FS_RATIO_96_192_LO,		(unsigned int)(ASRC_FS_RATIO_96_192 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_96_192 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1}			// Fsout = 192kHz
	},
	{	// Fsin = 176.4kHz
		// Nominal Fs Ratio				// Minimal Fs Ratio														// Maximal Fs Ratio														Shift for time step
		{ASRC_FS_RATIO_176_44,	ASRC_FS_RATIO_176_44_LO,		(unsigned int)(ASRC_FS_RATIO_176_44 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_176_44 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_M1},		// Fsout = 44.1kHz
		{ASRC_FS_RATIO_176_48,	ASRC_FS_RATIO_176_48_LO,		(unsigned int)(ASRC_FS_RATIO_176_48 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_176_48 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_M1},		// Fsout = 48kHz
		{ASRC_FS_RATIO_176_88,	ASRC_FS_RATIO_176_88_LO,		(unsigned int)(ASRC_FS_RATIO_176_88 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_176_88 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_0},			// Fsout = 88.2kHz
		{ASRC_FS_RATIO_176_96,	ASRC_FS_RATIO_176_96_LO,		(unsigned int)(ASRC_FS_RATIO_176_96 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_176_96 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_0},			// Fsout = 96kHz
		{ASRC_FS_RATIO_176_176,	ASRC_FS_RATIO_176_176_LO,		(unsigned int)(ASRC_FS_RATIO_176_176 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_176_176 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 176.4kHz
		{ASRC_FS_RATIO_176_192,	ASRC_FS_RATIO_176_192_LO,		(unsigned int)(ASRC_FS_RATIO_176_192 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_176_192 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1}			// Fsout = 192kHz
	},
	{	// Fsin = 192kHz
		// Nominal Fs Ratio				// Minimal Fs Ratio														// Maximal Fs Ratio														Shift for time step
		{ASRC_FS_RATIO_192_44,	ASRC_FS_RATIO_192_44_LO,		(unsigned int)(ASRC_FS_RATIO_192_44 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_192_44 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_M1},		// Fsout = 44.1kHz
		{ASRC_FS_RATIO_192_48,	ASRC_FS_RATIO_192_48_LO,		(unsigned int)(ASRC_FS_RATIO_192_48 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_192_48 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_M1},		// Fsout = 48kHz
		{ASRC_FS_RATIO_192_88,	ASRC_FS_RATIO_192_88_LO,		(unsigned int)(ASRC_FS_RATIO_192_88 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_192_88 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_0},			// Fsout = 88.2kHz
		{ASRC_FS_RATIO_192_96,	ASRC_FS_RATIO_192_96_LO,		(unsigned int)(ASRC_FS_RATIO_192_96 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_192_96 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_0},			// Fsout = 96kHz
		{ASRC_FS_RATIO_192_176,	ASRC_FS_RATIO_192_176_LO,		(unsigned int)(ASRC_FS_RATIO_192_176 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_192_176 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1},		// Fsout = 176.4kHz
		{ASRC_FS_RATIO_192_192,	ASRC_FS_RATIO_192_192_LO,		(unsigned int)(ASRC_FS_RATIO_192_192 * ASRC_FS_RATIO_MIN_FACTOR),		(unsigned int)(ASRC_FS_RATIO_192_192 * ASRC_FS_RATIO_MAX_FACTOR),		ASRC_FS_RATIO_SHIFT_P1}			// Fsout = 192kHz
	}
};


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
// Function:		ASRC_prepare_coefs									//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Prepares the ASRC coefficients from the prototype	//
//					Needs to be called only once						//
// ==================================================================== //
ASRCReturnCodes_t				ASRC_prepare_coefs(void)
{
	unsigned int		ui, uj;
	int*				piPrototypeCoefs = iADFirPrototypeCoefs + FILTER_DEFS_ADFIR_PROTOTYPE_N_TAPS - FILTER_DEFS_ADFIR_N_PHASES;

	// First fill in the phases which start normally
	for(ui = 0; ui < FILTER_DEFS_ADFIR_N_PHASES; ui++)
	{
		// Copy phase information
		for(uj = 0; uj < FILTER_DEFS_ADFIR_PHASE_N_TAPS - 1; uj++)
			iADFirCoefs[ui][uj] = *(piPrototypeCoefs + ui - uj * FILTER_DEFS_ADFIR_N_PHASES);
		// Zero fill last coefficient
		iADFirCoefs[ui][FILTER_DEFS_ADFIR_PHASE_N_TAPS - 1]	= 0;
	}

	// Then fill in the two phases which start delayed
	for(ui = 0; ui < 2; ui++)
	{
		// Zero fill first coefficient
		iADFirCoefs[FILTER_DEFS_ADFIR_N_PHASES + ui][0]	= 0;
		// Copy phase informaiton
		for(uj = 0; uj < FILTER_DEFS_ADFIR_PHASE_N_TAPS - 1; uj++)
			iADFirCoefs[FILTER_DEFS_ADFIR_N_PHASES + ui][uj + 1] = *(piPrototypeCoefs + ui - uj * FILTER_DEFS_ADFIR_N_PHASES);
	}

	return ASRC_NO_ERROR;
}

// ==================================================================== //
// Function:		ASRC_init											//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Inits the ASRC passed as argument					//
// ==================================================================== //
ASRCReturnCodes_t				ASRC_init(ASRCCtrl_t* psASRCCtrl)
{
	ASRCFiltersIDs_t*			psFiltersID;
	FIRDescriptor_t*			psFIRDescriptor;
	ADFIRDescriptor_t*			psADFIRDescriptor;


	// Check if state is allocated
	if(psASRCCtrl->psState == 0)
		return ASRC_ERROR;

	// Check if stack is allocated
	if(psASRCCtrl->piStack == 0)
		return ASRC_ERROR;

	// Check if valid Fsin and Fsout have been provided
	if( (psASRCCtrl->eInFs < ASRC_FS_MIN) || (psASRCCtrl->eInFs > ASRC_FS_MAX))
		return ASRC_ERROR;
	if( (psASRCCtrl->eOutFs < ASRC_FS_MIN) || (psASRCCtrl->eOutFs > ASRC_FS_MAX))
		return ASRC_ERROR;

	// Set nominal fs ratio
	psASRCCtrl->uiFsRatio	= sFsRatioConfigs[psASRCCtrl->eInFs][psASRCCtrl->eOutFs].uiNominalFsRatio;
	psASRCCtrl->uiFsRatio_lo = sFsRatioConfigs[psASRCCtrl->eInFs][psASRCCtrl->eOutFs].uiNominalFsRatio_lo;

	// Check that number of input samples is allocated and is a multiple of 4
	if(psASRCCtrl->uiNInSamples == 0)
		return ASRC_ERROR;
	if((psASRCCtrl->uiNInSamples & 0x3) != 0x0)
		return ASRC_ERROR;

	// Load filters ID and number of samples
	psFiltersID		= &sFiltersIDs[psASRCCtrl->eInFs][psASRCCtrl->eOutFs];


	// Configure filters F1 and F2 from filters ID and number of samples

	// Filter F1
	// ---------
	psFIRDescriptor							= &sFirDescriptor[psFiltersID->uiFID[ASRC_F1_INDEX]];
	// Set number of input samples and input samples step
	psASRCCtrl->sFIRF1Ctrl.uiNInSamples		= psASRCCtrl->uiNInSamples;
	psASRCCtrl->sFIRF1Ctrl.uiInStep			= psASRCCtrl->uiInStep;
	// Set delay line base pointer
	if( psFiltersID->uiFID[ASRC_F1_INDEX] == FILTER_DEFS_FIR_DS_ID )
		psASRCCtrl->sFIRF1Ctrl.piDelayB			= psASRCCtrl->psState->iDelayFIRShort;
	else
		psASRCCtrl->sFIRF1Ctrl.piDelayB			= psASRCCtrl->psState->iDelayFIRLong;
	// Set output buffer step
	psASRCCtrl->sFIRF1Ctrl.uiOutStep		= psASRCCtrl->uiOutStep;

	// Call init for FIR F1
	if(FIR_init_from_desc(&psASRCCtrl->sFIRF1Ctrl, psFIRDescriptor) != FIR_NO_ERROR)
		return FIR_ERROR;

	// Update synchronous number of samples
	if( psFiltersID->uiFID[ASRC_F1_INDEX] != FILTER_DEFS_FIR_NONE_ID )
		psASRCCtrl->uiNSyncSamples				= psASRCCtrl->sFIRF1Ctrl.uiNOutSamples;


	// Filter F2
	// ---------
	psFIRDescriptor							= &sFirDescriptor[psFiltersID->uiFID[ASRC_F2_INDEX]];
	// Set number of input samples and input samples step
	psASRCCtrl->sFIRF2Ctrl.uiNInSamples		= psASRCCtrl->sFIRF1Ctrl.uiNOutSamples;
	psASRCCtrl->sFIRF2Ctrl.uiInStep			= psASRCCtrl->sFIRF1Ctrl.uiOutStep;
	// Set delay line base pointer (second filter is always long with ASRC)
	psASRCCtrl->sFIRF2Ctrl.piDelayB			= psASRCCtrl->psState->iDelayFIRLong;
	// Set output buffer step
	psASRCCtrl->sFIRF2Ctrl.uiOutStep		= psASRCCtrl->uiOutStep;

	// Call init for FIR F2
	if(FIR_init_from_desc(&psASRCCtrl->sFIRF2Ctrl, psFIRDescriptor) != FIR_NO_ERROR)
		return FIR_ERROR;

	// Update synchronous number of samples
	if( psFiltersID->uiFID[ASRC_F2_INDEX] != FILTER_DEFS_FIR_NONE_ID )
		psASRCCtrl->uiNSyncSamples				= psASRCCtrl->sFIRF2Ctrl.uiNOutSamples;


	// Setup fixed input/output buffers for F1 and F2
	// ----------------------------------------------
	// We set all fixed items (to stack base)
	// F1 input is never from stack, so don't set it
	psASRCCtrl->sFIRF2Ctrl.piIn				= psASRCCtrl->piStack;
	psASRCCtrl->sFIRF1Ctrl.piOut			= psASRCCtrl->piStack;
	psASRCCtrl->sFIRF2Ctrl.piOut			= psASRCCtrl->piStack;



	// Filter F3
	// ---------
	psADFIRDescriptor						= &sADFirDescriptor;

	// Set delay line base pointer
	psASRCCtrl->sADFIRF3Ctrl.piDelayB		= psASRCCtrl->psState->iDelayADFIR;
	// Set AD coefficients pointer
	psASRCCtrl->sADFIRF3Ctrl.piADCoefs		= psASRCCtrl->piADCoefs;

	// Call init for ADFIR F3
	if(ADFIR_init_from_desc(&psASRCCtrl->sADFIRF3Ctrl, psADFIRDescriptor) != FIR_NO_ERROR)
		return FIR_ERROR;

	// Call sync function
	if(ASRC_sync(psASRCCtrl) != ASRC_NO_ERROR)
		return ASRC_ERROR;

	return ASRC_NO_ERROR;
}


// ==================================================================== //
// Function:		ASRC_sync											//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Syncs the ASRC passed as argument					//
// ==================================================================== //
ASRCReturnCodes_t				ASRC_sync(ASRCCtrl_t* psASRCCtrl)
{
	// Sync the FIR and ADFIR
	if(FIR_sync(&psASRCCtrl->sFIRF1Ctrl) != FIR_NO_ERROR)
		return ASRC_ERROR;
	if(FIR_sync(&psASRCCtrl->sFIRF2Ctrl) != FIR_NO_ERROR)
		return ASRC_ERROR;

	if(ADFIR_sync(&psASRCCtrl->sADFIRF3Ctrl) != FIR_NO_ERROR)
		return ASRC_ERROR;

	// Reset time
	psASRCCtrl->iTimeInt		= FILTER_DEFS_ADFIR_N_PHASES + ASRC_ADFIR_INITIAL_PHASE;
	psASRCCtrl->uiTimeFract		= 0;

	// Reset random seeds to initial values
	psASRCCtrl->psState->uiRndSeed	= psASRCCtrl->uiRndSeedInit;

	// Update time step based on Fs ratio
	if(ASRC_update_fs_ratio(psASRCCtrl) != ASRC_NO_ERROR)
		return ASRC_ERROR;

	// Reset cycle counters
	psASRCCtrl->sProfilingInfo.fCycleCountF1F2				= 0;
	psASRCCtrl->sProfilingInfo.fCycleCountF3AdaptiveCoefs	= 0;
	psASRCCtrl->sProfilingInfo.fCycleCountF3				= 0;
	psASRCCtrl->sProfilingInfo.fCycleCountDither			= 0;

	return ASRC_NO_ERROR;
}


// ==================================================================== //
// Function:		ASRC_proc_F1_F2										//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Processes F1 and F2 for a channel					//
// ==================================================================== //
ASRCReturnCodes_t				ASRC_proc_F1_F2(ASRCCtrl_t* psASRCCtrl)
{
	int*			piIn		= psASRCCtrl->piIn;
	int*			piOut		= psASRCCtrl->piOut;
	unsigned int	ui;


	// Setup variable input / output buffers
	psASRCCtrl->sFIRF1Ctrl.piIn			= psASRCCtrl->piIn;

	// F1 is always enabled, so call F1
	if(psASRCCtrl->sFIRF1Ctrl.pvProc(&psASRCCtrl->sFIRF1Ctrl) != FIR_NO_ERROR)
		return ASRC_ERROR;

	// Cycle counter estimation
	if(psASRCCtrl->sFIRF1Ctrl.pvProc == FIR_proc_os2)
		psASRCCtrl->sProfilingInfo.fCycleCountF1F2	+= psASRCCtrl->uiNInSamples * ( ASRC_FIR_OS2_OVERHEAD_CYCLE_COUNT +
										(psASRCCtrl->sFIRF1Ctrl.uiNCoefs * ASRC_FIR_OS2_TAP_CYCLE_COUNT) );

	if(psASRCCtrl->sFIRF1Ctrl.pvProc == FIR_proc_ds2)
		psASRCCtrl->sProfilingInfo.fCycleCountF1F2	+= (psASRCCtrl->uiNInSamples>>1) * ( ASRC_FIR_DS2_OVERHEAD_CYCLE_COUNT +
										 (psASRCCtrl->sFIRF1Ctrl.uiNCoefs * ASRC_FIR_DS2_TAP_CYCLE_COUNT) );

	if(psASRCCtrl->sFIRF1Ctrl.pvProc == FIR_proc_sync)
		psASRCCtrl->sProfilingInfo.fCycleCountF1F2	+= psASRCCtrl->uiNInSamples * ( ASRC_FIR_SYNC_OVERHEAD_CYCLE_COUNT +
										 (psASRCCtrl->sFIRF1Ctrl.uiNCoefs * ASRC_FIR_SYNC_TAP_CYCLE_COUNT) );


	// Check if F2 is enabled
	if(psASRCCtrl->sFIRF2Ctrl.eEnable == FIR_ON)
	{
		// F2 is enabled, so call F2
		if(psASRCCtrl->sFIRF2Ctrl.pvProc(&psASRCCtrl->sFIRF2Ctrl) != FIR_NO_ERROR)
			return ASRC_ERROR;

		// Cycle counter estimation
		if(psASRCCtrl->sFIRF2Ctrl.pvProc == FIR_proc_os2)
			psASRCCtrl->sProfilingInfo.fCycleCountF1F2	+= psASRCCtrl->sFIRF2Ctrl.uiNInSamples * ( ASRC_FIR_OS2_OVERHEAD_CYCLE_COUNT +
											 (psASRCCtrl->sFIRF2Ctrl.uiNCoefs * ASRC_FIR_OS2_TAP_CYCLE_COUNT) );

		if(psASRCCtrl->sFIRF2Ctrl.pvProc == FIR_proc_ds2)
			psASRCCtrl->sProfilingInfo.fCycleCountF1F2	+= (psASRCCtrl->sFIRF2Ctrl.uiNInSamples>>1) * ( ASRC_FIR_DS2_OVERHEAD_CYCLE_COUNT +
											 (psASRCCtrl->sFIRF2Ctrl.uiNCoefs * ASRC_FIR_DS2_TAP_CYCLE_COUNT) );

		if(psASRCCtrl->sFIRF2Ctrl.pvProc == FIR_proc_sync)
			psASRCCtrl->sProfilingInfo.fCycleCountF1F2	+= psASRCCtrl->sFIRF2Ctrl.uiNInSamples * ( ASRC_FIR_SYNC_OVERHEAD_CYCLE_COUNT +
											 (psASRCCtrl->sFIRF2Ctrl.uiNCoefs * ASRC_FIR_SYNC_TAP_CYCLE_COUNT) );
	}

	return ASRC_NO_ERROR;
}


// ==================================================================== //
// Function:		ASRC_update_fs_ratio								//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Updates the ASRC with the new Fs ratio				//
// ==================================================================== //
ASRCReturnCodes_t				ASRC_update_fs_ratio(ASRCCtrl_t* psASRCCtrl)
{
	unsigned int	uiFsRatio		= psASRCCtrl->uiFsRatio;
	unsigned int    uiFsRatio_low = psASRCCtrl->uiFsRatio_lo;

	// Check for bounds of new Fs ratio
	if( (uiFsRatio < sFsRatioConfigs[psASRCCtrl->eInFs][psASRCCtrl->eOutFs].uiMinFsRatio) ||
		(uiFsRatio > sFsRatioConfigs[psASRCCtrl->eInFs][psASRCCtrl->eOutFs].uiMaxFsRatio) )
		return ASRC_ERROR;

	// Apply shift to time ratio to build integer and fractional parts of time step
	psASRCCtrl->iTimeStepInt	 = uiFsRatio >> (sFsRatioConfigs[psASRCCtrl->eInFs][psASRCCtrl->eOutFs].iFsRatioShift);
	psASRCCtrl->uiTimeStepFract  = uiFsRatio << (32 - sFsRatioConfigs[psASRCCtrl->eInFs][psASRCCtrl->eOutFs].iFsRatioShift);
	psASRCCtrl->uiTimeStepFract |= (uint32_t)(uiFsRatio_low >> sFsRatioConfigs[psASRCCtrl->eInFs][psASRCCtrl->eOutFs].iFsRatioShift);

	return ASRC_NO_ERROR;
}


// ==================================================================== //
// Function:		ASRC_proc_F3_in_spl									//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
//					int iInSample: new input sample						//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Writes new input sample to F3 delay line			//
// ==================================================================== //
ASRCReturnCodes_t				ASRC_proc_F3_in_spl(ASRCCtrl_t* psASRCCtrl, int iInSample)
{
	psASRCCtrl->sADFIRF3Ctrl.iIn		= iInSample;
	if(ADFIR_proc_in_spl(&psASRCCtrl->sADFIRF3Ctrl) != FIR_NO_ERROR)
		return ASRC_ERROR;

	// Decrease next output time (this is an integer value, so no influence on fractional part)
	psASRCCtrl->iTimeInt	-= FILTER_DEFS_ADFIR_N_PHASES;

	// Update cycle count
	psASRCCtrl->sProfilingInfo.fCycleCountF3		+= ASRC_ADFIR_IN_SPL_CYCLE_COUNT;

	return ASRC_NO_ERROR;
}


// ==================================================================== //
// Function:		ASRC_proc_F3_time									//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR if an output sample must be produced	//
//					ASRC_ERROR if no output sample needs to be produced	//
// Description:		Processes F3 time									//
// ==================================================================== //
ASRCReturnCodes_t				ASRC_proc_F3_time(ASRCCtrl_t* psASRCCtrl)
{
	unsigned int	uiTemp;
	int				iAlpha;
	unsigned int	ui;

	int				iH0, iH1, iH2;
	int				iZero;
	int				iData0;
	int				iData1;
	__int64			i64Acc0;
	__int64			i64Acc1;
	int*			piPhase0;
	int*			piPhase1;
	int*			piPhase2;
	int*			piADCoefs;

	// Check if the next output time instant is in the current time slot
	// -----------------------------------------------------------------
	// if not return value showing that no output sample needs to be produced
	// Update cycle count
	psASRCCtrl->sProfilingInfo.fCycleCountF3AdaptiveCoefs	+= ASRC_ADFIR_TIME_CHECK_CYCLE_COUNT;
	if(psASRCCtrl->iTimeInt >= FILTER_DEFS_ADFIR_N_PHASES)
		return ASRC_ERROR;


	// Update adaptive filter coefficients
	// -----------------------------------
	// Coefficients computation load approximation: 15 instructions
	// Loop load approximation: 5.625 instructions per sample + 15 instructions overhead => 5.625 * 16 + 15 = 90 + 15 = 105 Instructions
	// Total: 125 instructions

	// Set register to zero
	iZero		= 0;

	// Compute adative coefficients spline factors
	// The fractional part of time gives alpha
	iAlpha		= psASRCCtrl->uiTimeFract>>1;		// Now alpha can be seen as a signed number
	LMUL(&i64Acc0, iAlpha, iAlpha, iZero, iZero);	// Compte H0 = 0.5 * alpha * alpha
	iH0			= (int)(i64Acc0>>32);
	iH2			= 0x40000000;						// Load H2 with 0.5;
	iH1			= iH2 - iH0;						// H1 = 0.5 - 0.5 * alpha * alpha;
	iH1			= iH1 - iH0;						// H1 = 0.5 - alpha * alpha
	iH1			= iH1 + iAlpha;						// H1 = 0.5 + alpha - alpha * alpha;
	iH2			= iH2 - iAlpha;						// H2 = 0.5 - alpha
	iH2			= iH2 + iH0;						// H2 = 0.5 - alpha + 0.5 * alpha * alpha

	// The integer part of time gives the phase
	piPhase0		= iADFirCoefs[psASRCCtrl->iTimeInt];
	piPhase1		= piPhase0 + FILTER_DEFS_ADFIR_PHASE_N_TAPS;
	piPhase2		= piPhase1 + FILTER_DEFS_ADFIR_PHASE_N_TAPS;
	piADCoefs		= psASRCCtrl->piADCoefs;		// Given limited number of registers, this should be DP

	// Apply spline coefficients to filter coefficients
	for(ui = 0; ui < FILTER_DEFS_ADFIR_PHASE_N_TAPS; ui += 2)
	{
		// Double read (long word)
		iData0		= *(piPhase0 + ui);
		iData1		= *(piPhase0 + ui + 1);
		// MACC for each word
		LMUL(&i64Acc0, iH2, iData0, iZero, iZero);
		LMUL(&i64Acc1, iH2, iData1, iZero, iZero);

		// Double read (long word)
		iData0		= *(piPhase1 + ui);
		iData1		= *(piPhase1 + ui + 1);
		// Double read (long word)
		MACC(&i64Acc0, iH1, iData0);
		MACC(&i64Acc1, iH1, iData1);

		// Double read (long word)
		iData0		= *(piPhase2 + ui);
		iData1		= *(piPhase2 + ui + 1);
		// Double read (long word)
		MACC(&i64Acc0, iH0, iData0);
		MACC(&i64Acc1, iH0, iData1);

		// Adaptive coefficient is given by upper part of accumulator
		// Use two writes of single word (as we don't have enough registers, piADCoefs should be DP
		*(piADCoefs + ui)		= (int)(i64Acc0>>32);
		*(piADCoefs + ui + 1)	= (int)(i64Acc1>>32);
	}

	// Step time for next output sample
	// --------------------------------
	// Step to next output time (add integer and fractional parts)
	psASRCCtrl->iTimeInt		+= psASRCCtrl->iTimeStepInt;
	// For fractional part, this can be optimized using the add with carry instruction of XS2
	uiTemp		= psASRCCtrl->uiTimeFract;
	psASRCCtrl->uiTimeFract		+= psASRCCtrl->uiTimeStepFract;
	if(psASRCCtrl->uiTimeFract < uiTemp)
		psASRCCtrl->iTimeInt++;

	//printf("TimeInt = %x  TimeFract = %x", psASRCCtrl->iTimeInt, psASRCCtrl->uiTimeFract);

	// Increase cycle counter
	psASRCCtrl->sProfilingInfo.fCycleCountF3AdaptiveCoefs		+= ASRC_ADFIR_TIME_SAMPLE_CYCLE_COUNT;

	// Return value showing that an output sample must be produced
	return ASRC_NO_ERROR;
}


// ==================================================================== //
// Function:		ASRC_proc_F3_macc									//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
//					int* piOutSample: Address of output sample			//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Processes F3 for a channel							//
// ==================================================================== //
ASRCReturnCodes_t				ASRC_proc_F3_macc(ASRCCtrl_t* psASRCCtrl, int* piOutSample)
{
	psASRCCtrl->sADFIRF3Ctrl.piOut		= piOutSample;
	// Call processing function
	if(ADFIR_proc_macc(&psASRCCtrl->sADFIRF3Ctrl) != FIR_NO_ERROR)
		return ASRC_ERROR;

	psASRCCtrl->uiNASRCOutSamples++;

	// Cycle counter estimation
	psASRCCtrl->sProfilingInfo.fCycleCountF3		+= ASRC_ADFIR_MACC_OVERHEAD_CYCLE_COUNT + (psASRCCtrl->sADFIRF3Ctrl.uiNLoops * 2.0 * ASRC_ADFIR_MACC_TAP_CYCLE_COUNT);

	return ASRC_NO_ERROR;
}


// ==================================================================== //
// Function:		ASRC_proc_dither									//
// Arguments:		ASRCCtrl_t 	*psASRCCtrl: Ctrl strct.				//
// Return values:	ASRC_NO_ERROR on success							//
//					ASRC_ERROR on failure								//
// Description:		Processes dither for a channel						//
// ==================================================================== //
ASRCReturnCodes_t				ASRC_proc_dither(ASRCCtrl_t* psASRCCtrl)
{
	int*			piData;
	unsigned int	uiR;
	int				iDither;
	__int64			i64Acc;
	unsigned int	ui;


	// Apply dither if required
	if(psASRCCtrl->uiDitherOnOff == ASRC_DITHER_ON)
	{
		// Get data buffer
		piData	= psASRCCtrl->piOut;
		// Get random seed
		uiR		= psASRCCtrl->psState->uiRndSeed;

		// Loop through samples
		for(ui = 0; ui < psASRCCtrl->uiNASRCOutSamples * psASRCCtrl->uiInStep; ui += psASRCCtrl->uiInStep)
		{
			// Compute dither sample (TPDF)
			iDither		= ASRC_DITHER_BIAS;

			uiR			= (unsigned int)(ASRC_R_BASE * uiR);
			uiR			= (unsigned int)(ASRC_R_CONS + uiR);
			iDither		+= ((uiR>>ASRC_RPDF_BITS_SHIFT) & ASRC_RPDF_MASK);

			uiR			= (unsigned int)(ASRC_R_BASE * uiR);
			uiR			= (unsigned int)(ASRC_R_CONS + uiR);
			iDither		+= ((uiR>>ASRC_RPDF_BITS_SHIFT) & ASRC_RPDF_MASK);

			// Use MACC instruction to saturate and dither + signal
			i64Acc		= ((__int64)iDither <<32);	// On XMOS this is not necessary, just load dither in the top word of the ACC register
			MACC(&i64Acc, piData[ui], 0x7FFFFFFF);
			LSAT30(&i64Acc);
			// Extract 32bits result
			EXT30(&piData[ui], i64Acc);

			// Mask to 24bits
			piData[ui]	&= ASRC_DATA24_MASK;

			// Increase cycle counter
			psASRCCtrl->sProfilingInfo.fCycleCountDither += ASRC_DITHER_SAMPLE_COUNT;
		}

		// Write random seed back
		psASRCCtrl->psState->uiRndSeed	= uiR;
	}

	return ASRC_NO_ERROR;
}
