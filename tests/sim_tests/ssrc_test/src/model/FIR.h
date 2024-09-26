// ===========================================================================
// ===========================================================================
//	
// File: FIR.h
//
// FIR functions definition file for the SSRC
//
// Target:	MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================

#ifndef _FIR_H_
#define _FIR_H_

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
			FIR_NO_ERROR							= 0,
			FIR_ERROR								= 1
		} FIRReturnCodes_t;

		// FIR On/Off Codes
		// ----------------
		typedef enum _FIROnOffCodes									
		{
			FIR_OFF									= 0,
			FIR_ON									= 1
		} FIROnOffCodes_t;
		

		// FIR Type Codes
		// --------------
		typedef enum _FIRTypeCodes									
		{
			FIR_TYPE_OS2							= 0,			// Over-sampler by two FIR
			FIR_TYPE_SYNC							= 1,			// Synchronous (low-pass) FIR
			FIR_TYPE_DS2							= 2,			// Down-sampler by two FIR
		} FIRTypeCodes_t;
		
		// FIR Descriptor
		// --------------
		typedef struct _FIRDescriptor										
		{
			FIRTypeCodes_t							eType;			// Type of filter
			unsigned int							uiNCoefs;		// Number of coefficients
			int*									piCoefs;		// Pointer to coefficients
		} FIRDescriptor_t;


		// FIR Ctrl
		// --------
		typedef struct _FIRCtrl										
		{
			FIROnOffCodes_t							eEnable;		// FIR on/off

			int*									piIn;			// Pointer to input data
			unsigned int							uiNInSamples;	// Number of input samples to process
			unsigned int							uiInStep;		// Step between input data samples
			int*									piOut;			// Pointer to output data
			unsigned int							uiNOutSamples;	// Number of output samples produced
			unsigned int							uiOutStep;		// Step between output data samples

			FIRReturnCodes_t 						(*pvProc)(void*);			// Processing function address

			int*									piDelayB;		// Pointer to delay line base
			unsigned int							uiDelayL;		// Total length of delay line
			int*									piDelayI;		// Pointer to current position in delay line
			int*									piDelayW;		// Delay buffer wrap around address (for circular buffer simulation)
			unsigned int							uiDelayO;		// Delay line offset for second write (for circular buffer simulation)

			unsigned int							uiNLoops;		// Number of inner loop iterations
			unsigned int							uiNCoefs;		// Number of coefficients
			int*									piCoefs;		// Pointer to coefficients
		} FIRCtrl_t;


		// PPFIR Descriptor
		// --------------
		typedef struct _PPFIRDescriptor										
		{
			unsigned int							uiNCoefs;		// Number of coefficients
			unsigned int							uiNPhases;		// Number of phases
			int*									piCoefs;		// Pointer to coefficients
		} PPFIRDescriptor_t;

		// PPFIR Ctrl
		// ----------
		typedef struct _PPFIRCtrl										
		{
			FIROnOffCodes_t							eEnable;		// PPFIR on/off

			int*									piIn;				// Pointer to input data
			unsigned int							uiNInSamples;		// Number of input samples to process
			unsigned int							uiInStep;			// Step between input data samples
			int*									piOut;				// Pointer to output data
			unsigned int							uiNOutSamples;		// Number of output samples produced
			unsigned int							uiOutStep;			// Step between output data samples

			int*									piDelayB;			// Pointer to delay line base
			unsigned int							uiDelayL;			// Total length of delay line
			int*									piDelayI;			// Pointer to current position in delay line
			int*									piDelayW;			// Delay buffer wrap around address (for circular buffer simulation)
			unsigned int							uiDelayO;			// Delay line offset for second write (for circular buffer simulation)

			unsigned int							uiNLoops;			// Number of inner loop iterations
			unsigned int							uiNCoefs;			// Number of coefficients
			int*									piCoefs;			// Pointer to coefficients
			unsigned int							uiNPhases;			// Number of phases
			unsigned int							uiPhaseStep;		// Phase step
			unsigned int							uiCoefsPhaseStep;	// Number of coefficients for a phase step
			unsigned int							uiCoefsPhase;		// Current phase coefficient offset from base
			
		} PPFIRCtrl_t;

		

		// ===========================================================================
		//
		// Function prototypes
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
		FIRReturnCodes_t				FIR_init_from_desc(FIRCtrl_t* psFIRCtrl, FIRDescriptor_t* psFIRDescriptor);	

		// ==================================================================== //
		// Function:		FIR_sync											//
		// Arguments:		FIRCtrl_t 	*psFIRCtrl: Ctrl strct.					//
		// Return values:	FIR_NO_ERROR on success								//
		//					FIR_ERROR on failure								//
		// Description:		Syncs the FIR 										//
		// ==================================================================== //
		FIRReturnCodes_t				FIR_sync(FIRCtrl_t* psFIRCtrl);

		// ==================================================================== //
		// Function:		FIR_proc_os2										//
		// Arguments:		FIRCtrl_t 	*psFIRCtrl: Ctrl strct.					//
		// Return values:	FIR_NO_ERROR on success								//
		//					FIR_ERROR on failure								//
		// Description:		Processes the FIR in over-sample by 2 mode			//
		// ==================================================================== //
		FIRReturnCodes_t				FIR_proc_os2(FIRCtrl_t* psFIRCtrl);	

		// ==================================================================== //
		// Function:		FIR_proc_sync										//
		// Arguments:		FIRCtrl_t 	*psFIRCtrl: Ctrl strct.					//
		// Return values:	FIR_NO_ERROR on success								//
		//					FIR_ERROR on failure								//
		// Description:		Processes the FIR in synchronous mode				//
		// ==================================================================== //
		FIRReturnCodes_t				FIR_proc_sync(FIRCtrl_t* psFIRCtrl);

		// ==================================================================== //
		// Function:		FIR_proc_ds2										//
		// Arguments:		FIRCtrl_t 	*psFIRCtrl: Ctrl strct.					//
		// Return values:	FIR_NO_ERROR on success								//
		//					FIR_ERROR on failure								//
		// Description:		Processes the FIR in down-sample by 2 mode			//
		// ==================================================================== //
		FIRReturnCodes_t				FIR_proc_ds2(FIRCtrl_t* psFIRCtrl);	

		// ==================================================================== //
		// Function:		PPFIR_init_from_desc								//
		// Arguments:		PPFIRCtrl_t 	*psPPFIRCtrl: Ctrl strct.			//
		//					PPFIRDescriptor_t *psPPFIRDescriptor: Desc. strct.	//
		// Return values:	FIR_NO_ERROR on success								//
		//					FIR_ERROR on failure								//
		// Description:		Inits the PPFIR from the Descriptor					//
		// ==================================================================== //
		FIRReturnCodes_t				PPFIR_init_from_desc(PPFIRCtrl_t* psPPFIRCtrl, PPFIRDescriptor_t* psPPFIRDescriptor);

		// ==================================================================== //
		// Function:		PPFIR_snyc											//
		// Arguments:		PPFIRCtrl_t 	*psPPFIRCtrl: Ctrl strct.			//
		// Return values:	FIR_NO_ERROR on success								//
		//					FIR_ERROR on failure								//
		// Description:		Syncs the PPFIR polyphase filter 					//
		// ==================================================================== //
		FIRReturnCodes_t				PPFIR_sync(PPFIRCtrl_t* psPPFIRCtrl);

		// ==================================================================== //
		// Function:		PPFIR_proc											//
		// Arguments:		PPFIRCtrl_t 	*psPPFIRCtrl: Ctrl strct.			//
		// Return values:	FIR_NO_ERROR on success								//
		//					FIR_ERROR on failure								//
		// Description:		Processes the PPFIR polyphase filter 				//
		// ==================================================================== //
		FIRReturnCodes_t				PPFIR_proc(PPFIRCtrl_t* psPPFIRCtrl);
		
	#endif // nINCLUDE_FROM_ASM

#endif // _FIR_H_