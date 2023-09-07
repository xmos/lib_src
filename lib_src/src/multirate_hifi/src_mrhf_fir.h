// Copyright 2016-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//
// FIR functions definition file for the SRC
//
// ===========================================================================
// ===========================================================================

#ifndef _SRC_MRHF_FIR_H
#define _SRC_MRHF_FIR_H
#ifndef ALIGNMENT
#  ifdef __xcore__
#    define ALIGNMENT(N)  __attribute__((aligned (N)))
#  else
#    define ALIGNMENT(N)
#  endif
#endif

/// (on xcore) Force variable to word alignment
#ifndef WORD_ALIGNED
#  define WORD_ALIGNED   ALIGNMENT(4)
#endif

/// (on xcore) Force variable to double word alignment
#ifndef DWORD_ALIGNED
#  define DWORD_ALIGNED  ALIGNMENT(8)
#endif
    // ===========================================================================
    //
    // Defines
    //
    // ===========================================================================

    // General defines
    // ---------------


    // Parameter values
    // ----------------




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

        // FIR Return Codes
        // ----------------
        typedef enum _FIRReturnCodes
        {
            FIR_NO_ERROR                            = 0,
            FIR_ERROR                                = 1
        } FIRReturnCodes_t;

        // FIR On/Off Codes
        // ----------------
        typedef enum _FIROnOffCodes
        {
            FIR_OFF                                    = 0,
            FIR_ON                                    = 1
        } FIROnOffCodes_t;


        // FIR Type Codes
        // --------------
        typedef enum _FIRTypeCodes
        {
            FIR_TYPE_OS2                            = 0,            // Over-sampler by two FIR
            FIR_TYPE_SYNC                            = 1,            // Asynchronous (low-pass) FIR
            FIR_TYPE_DS2                            = 2,            // Down-sampler by two FIR
        } FIRTypeCodes_t;

#ifdef __XC__
        // FIR Descriptor
        // --------------
        typedef struct _FIRDescriptor
        {
            FIRTypeCodes_t                            eType;            // Type of filter
            unsigned int                            uiNCoefs;        // Number of coefficients
            int* unsafe                                piCoefs;        // Pointer to coefficients
        } FIRDescriptor_t;


        // FIR Ctrl
        // --------
        typedef struct _FIRCtrl
        {
            FIROnOffCodes_t                            eEnable;        // FIR on/off

            int* unsafe                                piIn;            // Pointer to input data
            unsigned int                            uiNInSamples;    // Number of input samples to process
            unsigned int                            uiInStep;        // Step between input data samples
            int* unsafe                                piOut;            // Pointer to output data
            unsigned int                            uiNOutSamples;    // Number of output samples produced
            unsigned int                            uiOutStep;        // Step between output data samples

            FIRReturnCodes_t * unsafe                    pvProc;            // Processing function address

            int* unsafe                                piDelayB;        // Pointer to delay line base
            unsigned int                            uiDelayL;        // Total length of delay line
            int* unsafe                                piDelayI;        // Pointer to current position in delay line
            int* unsafe                                piDelayW;        // Delay buffer wrap around address (for circular buffer simulation)
            unsigned int                            uiDelayO;        // Delay line offset for second write (for circular buffer simulation)

            unsigned int                            uiNLoops;        // Number of inner loop iterations
            unsigned int                            uiNCoefs;        // Number of coefficients
            int* unsafe                                piCoefs;        // Pointer to coefficients
        } FIRCtrl_t;


        // ADFIR Descriptor
        // ----------------
        typedef struct _ADFIRDescriptor
        {
            unsigned int                            uiNCoefsPerPhase;    // Number of coefficients
            unsigned int                            uiNPhases;            // Number of phases
            int* unsafe                                piCoefs;            // Pointer to coefficients
        } ADFIRDescriptor_t;

        // ADFIR Ctrl
        // ----------
        typedef struct _ADFIRCtrl
        {
            int                                        iIn;                // Input sample
            int* unsafe                                piOut;                // Pointer to output sample

            int* unsafe                                piDelayB;            // Pointer to delay line base
            unsigned int                            uiDelayL;            // Total length of delay line
            int* unsafe                                piDelayI;            // Pointer to current position in delay line
            int* unsafe                                piDelayW;            // Delay buffer wrap around address (for circular buffer simulation)
            unsigned int                            uiDelayO;            // Delay line offset for second write (for circular buffer simulation)

            unsigned int                            uiNLoops;            // Number of inner loop iterations
            int* unsafe                                piADCoefs;            // Pointer to adaptive coefficients
        } ADFIRCtrl_t;


                        // PPFIR Descriptor
        // --------------
        typedef struct _PPFIRDescriptor
        {
            unsigned int                            uiNCoefs;        // Number of coefficients
            unsigned int                            uiNPhases;        // Number of phases
            int* unsafe                                piCoefs;        // Pointer to coefficients
        } PPFIRDescriptor_t;

        // PPFIR Ctrl
        // ----------
        typedef struct _PPFIRCtrl
        {
            FIROnOffCodes_t                            eEnable;        // PPFIR on/off

            int* unsafe                                piIn;                // Pointer to input data
            unsigned int                            uiNInSamples;        // Number of input samples to process
            unsigned int                            uiInStep;            // Step between input data samples
            int* unsafe                                piOut;                // Pointer to output data
            unsigned int                            uiNOutSamples;        // Number of output samples produced
            unsigned int                            uiOutStep;            // Step between output data samples

            int* unsafe                                piDelayB;            // Pointer to delay line base
            unsigned int                            uiDelayL;            // Total length of delay line
            int* unsafe                                piDelayI;            // Pointer to current position in delay line
            int* unsafe                                piDelayW;            // Delay buffer wrap around address (for circular buffer simulation)
            unsigned int                            uiDelayO;            // Delay line offset for second write (for circular buffer simulation)

            unsigned int                            uiNLoops;            // Number of inner loop iterations
            unsigned int                            uiNCoefs;            // Number of coefficients
            int* unsafe                                piCoefs;            // Pointer to coefficients
            unsigned int                            uiNPhases;            // Number of phases
            unsigned int                            uiPhaseStep;        // Phase step
            unsigned int                            uiCoefsPhaseStep;    // Number of coefficients for a phase step
            unsigned int                            uiCoefsPhase;        // Current phase coefficient offset from base

        } PPFIRCtrl_t;
#else
                        // FIR Descriptor
        // --------------
        typedef struct _FIRDescriptor
        {
            FIRTypeCodes_t                            eType;            // Type of filter
            unsigned int                            uiNCoefs;        // Number of coefficients
            int*                                    piCoefs;        // Pointer to coefficients
        } FIRDescriptor_t;


        // FIR Ctrl
        // --------
        typedef struct _FIRCtrl
        {
            FIROnOffCodes_t                            eEnable;        // FIR on/off

            int*                                    piIn;            // Pointer to input data
            unsigned int                            uiNInSamples;    // Number of input samples to process
            unsigned int                            uiInStep;        // Step between input data samples
            int*                                    piOut;            // Pointer to output data
            unsigned int                            uiNOutSamples;    // Number of output samples produced
            unsigned int                            uiOutStep;        // Step between output data samples

__attribute__((fptrgroup("G1")))
            FIRReturnCodes_t                         (*pvProc)(int *);// Processing function address

            int*                                    piDelayB;        // Pointer to delay line base
            unsigned int                            uiDelayL;        // Total length of delay line
            int*                                    piDelayI;        // Pointer to current position in delay line
            int*                                    piDelayW;        // Delay buffer wrap around address (for circular buffer simulation)
            unsigned int                            uiDelayO;        // Delay line offset for second write (for circular buffer simulation)

            unsigned int                            uiNLoops;        // Number of inner loop iterations
            unsigned int                            uiNCoefs;        // Number of coefficients
            int*                                    piCoefs;        // Pointer to coefficients
        } FIRCtrl_t;

        // ADFIR Descriptor
        // ----------------
        typedef struct _ADFIRDescriptor
        {
            unsigned int                            uiNCoefsPerPhase;    // Number of coefficients
            unsigned int                            uiNPhases;            // Number of phases
            int*                                    piCoefs;            // Pointer to coefficients
        } ADFIRDescriptor_t;

        // ADFIR Ctrl
        // ----------
        typedef struct _ADFIRCtrl
        {
            int                                        iIn;                // Input sample
            int*                                    piOut;                // Pointer to output sample

            int*                                    piDelayB;            // Pointer to delay line base
            unsigned int                            uiDelayL;            // Total length of delay line
            int*                                    piDelayI;            // Pointer to current position in delay line
            int*                                    piDelayW;            // Delay buffer wrap around address (for circular buffer simulation)
            unsigned int                            uiDelayO;            // Delay line offset for second write (for circular buffer simulation)

            unsigned int                            uiNLoops;            // Number of inner loop iterations
            int*                                    piADCoefs;            // Pointer to adaptive coefficients
        } ADFIRCtrl_t;


                        // PPFIR Descriptor
        // --------------
        typedef struct _PPFIRDescriptor
        {
            unsigned int                            uiNCoefs;        // Number of coefficients
            unsigned int                            uiNPhases;        // Number of phases
            int*                                    piCoefs;        // Pointer to coefficients
        } PPFIRDescriptor_t;

        // PPFIR Ctrl
        // ----------
        typedef struct _PPFIRCtrl
        {
            FIROnOffCodes_t                            eEnable;        // PPFIR on/off

            int*                                    piIn;                // Pointer to input data
            unsigned int                            uiNInSamples;        // Number of input samples to process
            unsigned int                            uiInStep;            // Step between input data samples
            int*                                    piOut;                // Pointer to output data
            unsigned int                            uiNOutSamples;        // Number of output samples produced
            unsigned int                            uiOutStep;            // Step between output data samples

            int*                                    piDelayB;            // Pointer to delay line base
            unsigned int                            uiDelayL;            // Total length of delay line
            int*                                    piDelayI;            // Pointer to current position in delay line
            int*                                    piDelayW;            // Delay buffer wrap around address (for circular buffer simulation)
            unsigned int                            uiDelayO;            // Delay line offset for second write (for circular buffer simulation)

            unsigned int                            uiNLoops;            // Number of inner loop iterations
            unsigned int                            uiNCoefs;            // Number of coefficients
            int*                                    piCoefs;            // Pointer to coefficients
            unsigned int                            uiNPhases;            // Number of phases
            unsigned int                            uiPhaseStep;        // Phase step
            unsigned int                            uiCoefsPhaseStep;    // Number of coefficients for a phase step
            unsigned int                            uiCoefsPhase;        // Current phase coefficient offset from base

        } PPFIRCtrl_t;
#endif

        // ===========================================================================
        //
        // Function prototypes
        //
        // ===========================================================================

        // ==================================================================== //
        // Function:        FIR_init_from_desc                                    //
        // Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
        //                    FIRDescriptor_t *psFIRDescriptor: Desc. strct.        //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Inits the FIR from the Descriptor                    //
        // ==================================================================== //
        FIRReturnCodes_t                FIR_init_from_desc(FIRCtrl_t* psFIRCtrl, FIRDescriptor_t* psFIRDescriptor);

        // ==================================================================== //
        // Function:        FIR_sync                                            //
        // Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Syncs the FIR                                         //
        // ==================================================================== //
        FIRReturnCodes_t                FIR_sync(FIRCtrl_t* psFIRCtrl);

        // ==================================================================== //
        // Function:        FIR_proc_os2                                        //
        // Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Processes the FIR in over-sample by 2 mode            //
        // ==================================================================== //
        FIRReturnCodes_t                FIR_proc_os2(FIRCtrl_t* psFIRCtrl);

        // ==================================================================== //
        // Function:        FIR_proc_sync                                        //
        // Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Processes the FIR in asynchronous mode                //
        // ==================================================================== //
        FIRReturnCodes_t                FIR_proc_sync(FIRCtrl_t* psFIRCtrl);

        // ==================================================================== //
        // Function:        FIR_proc_ds2                                        //
        // Arguments:        FIRCtrl_t     *psFIRCtrl: Ctrl strct.                    //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Processes the FIR in down-sample by 2 mode            //
        // ==================================================================== //
        FIRReturnCodes_t                FIR_proc_ds2(FIRCtrl_t* psFIRCtrl);

        // ==================================================================== //
        // Function:        ADFIR_init_from_desc                                //
        // Arguments:        ADFIRCtrl_t     *psADFIRCtrl: Ctrl strct.            //
        //                    ADFIRDescriptor_t *psADFIRDescriptor: Desc. strct.    //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Inits the ADFIR from the Descriptor                    //
        // ==================================================================== //
        FIRReturnCodes_t                ADFIR_init_from_desc(ADFIRCtrl_t* psADFIRCtrl, ADFIRDescriptor_t* psADFIRDescriptor);

        // ==================================================================== //
        // Function:        ADFIR_snyc                                            //
        // Arguments:        ADFIRCtrl_t     *psADFIRCtrl: Ctrl strct.            //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Syncs the ADFIR filter                                 //
        // ==================================================================== //
        FIRReturnCodes_t                ADFIR_sync(ADFIRCtrl_t* psADFIRCtrl);

        // ==================================================================== //
        // Function:        ADFIR_proc_in_sample                                //
        // Arguments:        ADFIRCtrl_t     *psADFIRCtrl: Ctrl strct.            //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Gets new input sample to delay line    with step         //
        // ==================================================================== //
        FIRReturnCodes_t                ADFIR_proc_in_spl(ADFIRCtrl_t* psADFIRCtrl);

        // ==================================================================== //
        // Function:        ADFIR_proc_macc                                        //
        // Arguments:        ADFIRCtrl_t     *psADFIRCtrl: Ctrl strct.            //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Processes the macc loop for the ADFIR filter        //
        // ==================================================================== //
        FIRReturnCodes_t                ADFIR_proc_macc(ADFIRCtrl_t* psADFIRCtrl);

        // ==================================================================== //
        // Function:        PPFIR_init_from_desc                                //
        // Arguments:        PPFIRCtrl_t     *psPPFIRCtrl: Ctrl strct.            //
        //                    PPFIRDescriptor_t *psPPFIRDescriptor: Desc. strct.    //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Inits the PPFIR from the Descriptor                    //
        // ==================================================================== //
        FIRReturnCodes_t                PPFIR_init_from_desc(PPFIRCtrl_t* psPPFIRCtrl, PPFIRDescriptor_t* psPPFIRDescriptor);

        // ==================================================================== //
        // Function:        PPFIR_snyc                                            //
        // Arguments:        PPFIRCtrl_t     *psPPFIRCtrl: Ctrl strct.            //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Syncs the PPFIR polyphase filter                     //
        // ==================================================================== //
        FIRReturnCodes_t                PPFIR_sync(PPFIRCtrl_t* psPPFIRCtrl);

        // ==================================================================== //
        // Function:        PPFIR_proc                                            //
        // Arguments:        PPFIRCtrl_t     *psPPFIRCtrl: Ctrl strct.            //
        // Return values:    FIR_NO_ERROR on success                                //
        //                    FIR_ERROR on failure                                //
        // Description:        Processes the PPFIR polyphase filter                 //
        // ==================================================================== //
        FIRReturnCodes_t                PPFIR_proc(PPFIRCtrl_t* psPPFIRCtrl);
    #endif // nINCLUDE_FROM_ASM

#endif // _SRC_MRHF_FIR_H
