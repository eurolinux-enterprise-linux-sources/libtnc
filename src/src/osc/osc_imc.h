// osc_imc.h
//
// Copyright (C) 2006 Open System Consultants Pty Ltd
// Author: Mike McCauley (mikem@open.com.au)
// $Id: osc_imc.h,v 1.3 2006/04/10 22:42:21 mikem Exp $


#ifndef _LIBOSC_IMC_H
#define _LIBOSC_IMC_H
#ifdef __cplusplus
extern "C" {
#endif

#include "osc.h"

// OSC Vendor specific interface functions

// If this function is defined in the parent, it will be used to 
// log diagnostic messages
TNC_IMC_API TNC_Result TNC_9048_LogMessage(
/*in*/ TNC_UInt32 severity,
/*in*/ const char * message);

typedef TNC_Result (*TNC_9048_UserMessagePointer)(
    TNC_IMCID imcID,
    TNC_ConnectionID connectionID,
    const char * message);

// If this function is defined in the parent, it will be used to 
// display a message to the user
TNC_IMC_API TNC_Result TNC_9048_UserMessage(
/*in*/ TNC_IMCID imcID,
/*in*/ TNC_ConnectionID connectionID,
/*in*/ const char * message);

// This is the name of the external command that will be used to implement
// the OSC_MESSAGE_EXTCOMMAND_REQUEST requests
#define OSC_IMC_EXTERNAL_COMMAND "C:\\wzccmd.exe"

#ifdef __cplusplus
}
#endif
#endif
