// Copyright 2023-2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// ===========================================================================
// ===========================================================================
//	
// File: Main.h
//
// Main definition file for the DS3
//
// Target:	MS Windows
// Version: 1.0
//
// ===========================================================================
// ===========================================================================

#ifndef _MAIN_H_
#define _MAIN_H_

	// ===========================================================================
	//
	// Defines
	//
	// ===========================================================================
	#define		APPLICATION			"DS3"
	#define		VERSION				"Version 1.0"
	#define		COPYRIGHT			"(c) Digimath, 2015"
	#define		FATAL				0
	#define		WARNING				1

	

	// ===========================================================================
	//
	// Function prototypes
	//
	// ===========================================================================
	void	ParseCmdLine(char *input);
	void	ShowHelp();
	void	ShowUsage();
	void	HandleError(char *pzErrorMessage, short sErrorType);
	
#endif // _MAIN_H_