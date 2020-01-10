// libtncimc.h
//
// Utility routines header for IMC routines
// Provides function to load IMCs, and call their public functions
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtncimc.h,v 1.5 2009/03/18 11:28:35 mikem Exp mikem $

#include <tncifimc.h>
#include <libtnc.h>


#ifndef _LIBTNCIMC_H
#define _LIBTNCIMC_H
#ifdef __cplusplus
extern "C" {
#endif


// DEFINITIONS FOR IMC (client side)
// The caller is required to define these functions which may be
// called by any IMC
//  TNC_TNCC_SendMessage
//  TNC_TNCC_RequestHandshakeRetry

// libtnc_imc
// Records information about a loaded instance of an IMC module
typedef struct
{
    char*                                 filename;
    TNC_IMCID                             imcID;
    TNC_Version                           version;
    void*                                 dl; // Result of dlopen, on Windows this is an HMODULE.

    // Pointers to public functions inside the module
    TNC_IMC_InitializePointer             intialiseP;           // Mandatory
    TNC_IMC_NotifyConnectionChangePointer notifyConnectionChangeP;
    TNC_IMC_BeginHandshakePointer         beginHandshakeP;      // Mandatory
    TNC_IMC_ReceiveMessagePointer         receiveMessageP;
    TNC_IMC_BatchEndingPointer            batchEndingP;
    TNC_IMC_TerminatePointer              terminateP;
    TNC_IMC_ProvideBindFunctionPointer    provideBindFunctionP; // Mandatory

    // Info about message types this IMC is interested in
    TNC_MessageTypeList                   supportedTypes;
    TNC_UInt32                            typeCount;
} libtnc_imc;

// Load the IMCs named in a config file in the format specified by IF-IMC
// Return the number of successfully loaded modules
extern int libtnc_imc_load_config(
    /*in*/ const char* filename);

// Load IMCs named in the standard place for this platform
// On Unix, we look in /etc/tnc_config
// On Windows, and we look for the well known registry keys.
// Return the number of modules successfully loaded
extern int libtnc_imc_load_std_config();

// Load one or more IMCs given the module file name
// File names may be absolute or relative
// Return TNC_RESULT_SUCCESS if all loaded and linked successfully
// else TNC_RESULT_FATAL
extern TNC_Result libtnc_imc_load_modules(
    /*in*/ const char* filenames[],
    /*in*/ TNC_UInt32 numfiles);

// Call NotifyConnectionChange in all IMCs
extern TNC_Result libtnc_imc_NotifyConnectionChange(
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_ConnectionState newState);

// Call BeginHandshake in all IMCs
extern TNC_Result libtnc_imc_BeginHandshake(
    /*in*/ TNC_ConnectionID connectionID);

// Call ReceiveMessage in all IMCs that are registered to receive
// this message type
extern TNC_Result libtnc_imc_ReceiveMessage(
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference messageBuffer,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType);

// Call BatchEnding in all IMCs
extern TNC_Result libtnc_imc_BatchEnding(
    /*in*/ TNC_ConnectionID connectionID);

// Call Terminate in all IMCs and unload them
extern TNC_Result libtnc_imc_Terminate();

// Load a single IMC module given its filename
// Not for general public use.
extern libtnc_imc* libtnc_imc_new(
    /*in*/ const char* filename);

// Unload and destroy a single IMC
// Not for general public use.
extern TNC_Result libtnc_imc_destroy(
    /*in*/ libtnc_imc* self);


// Defines the maximum bumber of IMCs we can handle.
#define NUM_IMCS 100


#ifdef __cplusplus
}
#endif
#endif
