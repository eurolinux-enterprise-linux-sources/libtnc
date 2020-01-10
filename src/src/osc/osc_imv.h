// osc_imv.h
//
// Copyright (C) 2006 Open System Consultants Pty Ltd
// Author: Mike McCauley (mikem@open.com.au)
// $Id: osc_imv.h,v 1.3 2006/04/10 22:42:21 mikem Exp $


#ifndef _LIBOSC_IMV_H
#define _LIBOSC_IMV_H
#ifdef __cplusplus
extern "C" {
#endif

#include <libtncimv.h>
#include "osc.h"
#include <libtnchash.h>

// This structure holds locally cached information about a single
// OSC IMC-IMV connection, including the data from previous OSC-IMC-IMV data requests
// For the clientdata hash, the key is sysname.subsysname.dataname
typedef struct
{
    TNC_ConnectionID              connectionID; // Callers connection id
    TNC_IMV_Action_Recommendation recommendation;
    TNC_IMV_Evaluation_Result     evaluation;
    libtnc_hash                   clientdata;   // Values previously received from the client
} osc_imv_connection;

// Private functions for internal use only:
extern TNC_Result connSetClientData(
    osc_imv_connection* conn,
    char* sysname, char* dataname, char* subsysname, char* value);

extern char* connGetClientData(
    osc_imv_connection* conn,
    char* sysname, char* dataname, char* subsysname);


// OSC Vendor specific interface functions

// If this function is defined in the parent, it will be used to 
// log diagnostic messages
TNC_IMV_API TNC_Result TNC_9048_LogMessage(
/*in*/ TNC_IMVID imvID,
/*in*/ TNC_UInt32 severity,
/*in*/ const char * message);

typedef TNC_Result (*TNC_9048_UserMessagePointer)(
    TNC_IMVID imvID,
    TNC_ConnectionID connectionID,
    const char * message);

// If this function is defined in the parent, it will be used to 
// display a message to the user
TNC_IMV_API TNC_Result TNC_9048_UserMessage(
/*in*/ TNC_IMVID imvID,
/*in*/ TNC_ConnectionID connectionID,
/*in*/ const char * message);

#ifdef __cplusplus
}
#endif
#endif
