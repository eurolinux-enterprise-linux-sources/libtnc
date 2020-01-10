// libtncimv.h
//
// Utility routines header for IMV routines
// Provides function to load IMVs, and call their public functions
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtncimv.h,v 1.5 2009/03/18 11:28:35 mikem Exp mikem $

#include <tncifimv.h>
#include <libtnc.h>


#ifndef _LIBTNCIMV_H
#define _LIBTNCIMV_H
#ifdef __cplusplus
extern "C" {
#endif


// DEFINITIONS FOR IMV (client side)
// The caller is required to define these functions which may be
// called by any IMV
//  TNC_TNCS_SendMessage
//  TNC_TNCS_RequestHandshakeRetry
//  TNC_TNCS_ProvideRecommendation

// libtnc_imv
// Records information about a loaded instance of an IMV module
typedef struct
{
    char*                                 filename;
    TNC_IMVID                             imvID;
    TNC_Version                           version;
    void*                                 dl; // Result of dlopen

    // Pointers to public functions inside the module
    TNC_IMV_InitializePointer             intialiseP;           // Mandatory
    TNC_IMV_NotifyConnectionChangePointer notifyConnectionChangeP;
    TNC_IMV_ReceiveMessagePointer         receiveMessageP;
    TNC_IMV_SolicitRecommendationPointer  solicitRecommendationP;      // Mandatory
    TNC_IMV_BatchEndingPointer            batchEndingP;
    TNC_IMV_TerminatePointer              terminateP;
    TNC_IMV_ProvideBindFunctionPointer    provideBindFunctionP; // Mandatory

    // Info about message types this IMV is interested in
    TNC_MessageTypeList                   supportedTypes;
    TNC_UInt32                            typeCount;
} libtnc_imv;

// Load the IMVs named in a config file in the format specified by IF-IMV
// Return the number of successfully loaded modules
extern int libtnc_imv_load_config(
    /*in*/ const char* filename);

// Load IMVs named in the standard place for this platform
// On Unix, we look in /etc/tnc_config
// On Windows, and we look for the well known registry keys.
// Return the number of modules successfully loaded
extern int libtnc_imv_load_std_config();

// Load one or more IMVs given the module file name
// File names may be absolute or relative
// Return TNC_RESULT_SUCCESS if all loaded and linked successfully
// else TNC_RESULT_FATAL
extern TNC_Result libtnc_imv_load_modules(
    /*in*/ const char* filenames[],
    /*in*/ TNC_UInt32 numfiles);

// Call NotifyConnectionChange in all IMVs
extern TNC_Result libtnc_imv_NotifyConnectionChange(
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_ConnectionState newState);

// Call ReceiveMessage in all IMVs that are registered to receive
// this message type
extern TNC_Result libtnc_imv_ReceiveMessage(
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference messageBuffer,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType);

// Call BatchEnding in all IMVs
extern TNC_Result libtnc_imv_BatchEnding(
    /*in*/ TNC_ConnectionID connectionID);

// Call Terminate in all IMVs and unload them
extern TNC_Result libtnc_imv_Terminate();

// Call BatchEnding in all IMVs
// recommendations is an array of receomendations we already have,
// for this conection ID indexed by imvid.
extern TNC_Result libtnc_imv_SolicitRecommendation(
    /*in*/ TNC_ConnectionID connectionID);

// Load a single IMV module given its filename
// Not for general public use.
extern libtnc_imv* libtnc_imv_new(
    /*in*/ const char* filename);

// Unload and destroy a single IMV
// Not for general public use.
extern TNC_Result libtnc_imv_destroy(
    /*in*/ libtnc_imv* self);

// Defines the maximum bumber of IMVs we can handle.
#define NUM_IMVS 100


#ifdef __cplusplus
}
#endif
#endif
