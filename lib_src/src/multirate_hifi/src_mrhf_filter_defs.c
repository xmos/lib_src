// Copyright 2016-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// Filters Definitions implementation file for the SRC
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

// FIR and FilterDefs includes
#include "src_mrhf_fir.h"
#include "src_mrhf_filter_defs.h"

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

// SSRC FIR filters descriptors (ordered by ID)
FIRDescriptor_t            sSSRCFirDescriptor[FILTER_DEFS_SSRC_N_FIR_ID] =
{
    {FIR_TYPE_DS2,                            FILTER_DEFS_FIR_BL_N_TAPS,            iFirBLCoefs},            // FILTER_DEFS_FIR_BL_ID
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BL9644_N_TAPS,        iFirBL9644Coefs},        // FILTER_DEFS_FIR_BL9644_ID
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BL8848_N_TAPS,        iFirBL8848},            // FILTER_DEFS_FIR_BL8848_ID
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BL19288_N_TAPS,        iFirBL19288Coefs},        // FILTER_DEFS_FIR_BL19288_ID
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BL17696_N_TAPS,        iFirBL17696},            // FILTER_DEFS_FIR_BL17696_ID
    {FIR_TYPE_OS2,                            FILTER_DEFS_FIR_UP_N_TAPS,            iFirUPCoefs},            // FILTER_DEFS_FIR_UP_ID
    {FIR_TYPE_OS2,                            FILTER_DEFS_FIR_UP4844_N_TAPS,        iFirUP4844Coefs},        // FILTER_DEFS_FIR_UP4844_ID
    {FIR_TYPE_OS2,                            FILTER_DEFS_FIR_UPF_N_TAPS,            iFirUPFCoefs},            // FILTER_DEFS_FIR_UPF_ID
    {FIR_TYPE_OS2,                            FILTER_DEFS_FIR_UP192176_N_TAPS,    iFirUP192176Coefs},        // FILTER_DEFS_FIR_UP192176_ID
    {FIR_TYPE_DS2,                            FILTER_DEFS_FIR_DS_N_TAPS,            iFirDSCoefs},            // FILTER_DEFS_FIR_DS_ID
    {FIR_TYPE_OS2,                            FILTER_DEFS_FIR_OS_N_TAPS,            iFirOSCoefs},            // FILTER_DEFS_FIR_OS_ID
    {FIR_TYPE_SYNC,                            0,                                    0}                        // FILTER_DEFS_FIR_NONE_ID
};
// ASRC  FIR filters descriptors (ordered by ID)
FIRDescriptor_t            sASRCFirDescriptor[FILTER_DEFS_ASRC_N_FIR_ID] =
{
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BL_N_TAPS,            iFirBLCoefs},            // FILTER_DEFS_FIR_BL_ID
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BL9644_N_TAPS,        iFirBL9644Coefs},        // FILTER_DEFS_FIR_BL9644_ID
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BL8848_N_TAPS,        iFirBL8848},            // FILTER_DEFS_FIR_BL8848_ID
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BLF_N_TAPS,            iFirBLFCoefs},            // FILTER_DEFS_FIR_BLF_ID
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BL19288_N_TAPS,        iFirBL19288Coefs},        // FILTER_DEFS_FIR_BL19288_ID
    {FIR_TYPE_SYNC,                            FILTER_DEFS_FIR_BL17696_N_TAPS,        iFirBL17696},            // FILTER_DEFS_FIR_BL17696_ID
    {FIR_TYPE_OS2,                            FILTER_DEFS_FIR_UP_N_TAPS,            iFirUPCoefs},            // FILTER_DEFS_FIR_UP_ID
    {FIR_TYPE_OS2,                            FILTER_DEFS_FIR_UP4844_N_TAPS,        iFirUP4844Coefs},        // FILTER_DEFS_FIR_UP4844_ID
    {FIR_TYPE_OS2,                            FILTER_DEFS_FIR_UPF_N_TAPS,            iFirUPFCoefs},            // FILTER_DEFS_FIR_UPF_ID
    {FIR_TYPE_OS2,                            FILTER_DEFS_FIR_UP192176_N_TAPS,    iFirUP192176Coefs},        // FILTER_DEFS_FIR_UP192176_ID
    {FIR_TYPE_DS2,                            FILTER_DEFS_FIR_DS_N_TAPS,            iFirDSCoefs},            // FILTER_DEFS_FIR_DS_ID
    {FIR_TYPE_SYNC,                            0,                                    0}                        // FILTER_DEFS_FIR_NONE_ID
};

// Double-word alignment enables the VX4 scalar kernels' paired loads.
// FIR filters coefficients
int DWORD_ALIGNED      iFirBLCoefs[FILTER_DEFS_FIR_BL_N_TAPS]     = {
    #include FILTER_DEFS_FIR_BL_FILE
};
int DWORD_ALIGNED      iFirBL9644Coefs[FILTER_DEFS_FIR_BL9644_N_TAPS]     = {
    #include FILTER_DEFS_FIR_BL9644_FILE
};
int DWORD_ALIGNED      iFirBL8848[FILTER_DEFS_FIR_BL8848_N_TAPS]     = {
    #include FILTER_DEFS_FIR_BL8848_FILE
};
int DWORD_ALIGNED      iFirBLFCoefs[FILTER_DEFS_FIR_BLF_N_TAPS]     = {
    #include FILTER_DEFS_FIR_BLF_FILE
};
int DWORD_ALIGNED      iFirBL19288Coefs[FILTER_DEFS_FIR_BL19288_N_TAPS]     = {
    #include FILTER_DEFS_FIR_BL19288_FILE
};
int DWORD_ALIGNED      iFirBL17696[FILTER_DEFS_FIR_BL17696_N_TAPS]     = {
    #include FILTER_DEFS_FIR_BL17696_FILE
};
int DWORD_ALIGNED      iFirUPCoefs[FILTER_DEFS_FIR_UP_N_TAPS]     = {
    #include FILTER_DEFS_FIR_UP_FILE
};
int DWORD_ALIGNED      iFirUP4844Coefs[FILTER_DEFS_FIR_UP4844_N_TAPS]     = {
    #include FILTER_DEFS_FIR_UP4844_FILE
};
int DWORD_ALIGNED      iFirUPFCoefs[FILTER_DEFS_FIR_UPF_N_TAPS]     = {
    #include FILTER_DEFS_FIR_UPF_FILE
};
int DWORD_ALIGNED      iFirUP192176Coefs[FILTER_DEFS_FIR_UP192176_N_TAPS]     = {
    #include FILTER_DEFS_FIR_UP192176_FILE
};
int DWORD_ALIGNED      iFirDSCoefs[FILTER_DEFS_FIR_DS_N_TAPS]     = {
    #include FILTER_DEFS_FIR_DS_FILE
};
int DWORD_ALIGNED      iFirOSCoefs[FILTER_DEFS_FIR_OS_N_TAPS]     = {
    #include FILTER_DEFS_FIR_OS_FILE
};



// ADFIR filters descriptor
ADFIRDescriptor_t        sADFirDescriptor =
{
    FILTER_DEFS_ADFIR_PHASE_N_TAPS,        FILTER_DEFS_ADFIR_N_PHASES + 2,        (int *)iADFirCoefs
};

// ADFIR filter coefficients
int                    iADFirPrototypeCoefs[FILTER_DEFS_ADFIR_PROTOTYPE_N_TAPS]  = {
    #include FILTER_DEFS_ADFIR_PROTOTYPE_FILE
};
#if SRC_USE_VPU
int                    iADFirCoefs[FILTER_DEFS_ADFIR_PHASE_N_TAPS][FILTER_DEFS_ADFIR_N_PHASES + 2];
#else
int                    iADFirCoefs[FILTER_DEFS_ADFIR_N_PHASES + 2][FILTER_DEFS_ADFIR_PHASE_N_TAPS];
#endif

// PPFIR filters descriptors (ordered by ID)
PPFIRDescriptor_t        sPPFirDescriptor[FILTER_DEFS_N_PPFIR_ID] =
{
    {FILTER_DEFS_PPFIR_HS294_N_TAPS,        FILTER_DEFS_PPFIR_HS294_N_PHASES,    iPPFirHS294Coefs},        // FILTER_DEFS_PPFIR_HS294_ID
    {FILTER_DEFS_PPFIR_HS320_N_TAPS,        FILTER_DEFS_PPFIR_HS320_N_PHASES,    iPPFirHS320Coefs},        // FILTER_DEFS_PPFIR_HS320_ID
    {0,                                        0,                                    0}                        // FILTER_DEFS_PPFIR_NONE_ID
};

// PPFIR filters coefficients
int DWORD_ALIGNED      iPPFirHS294Coefs[FILTER_DEFS_PPFIR_HS294_N_TAPS]  = {
    #include FILTER_DEFS_PPFIR_HS294_FILE
};
int DWORD_ALIGNED      iPPFirHS320Coefs[FILTER_DEFS_PPFIR_HS320_N_TAPS]  = {
    #include FILTER_DEFS_PPFIR_HS320_FILE
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





