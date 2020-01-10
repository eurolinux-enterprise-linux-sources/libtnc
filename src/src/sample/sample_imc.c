// sample_imc.c
//
// Sample Integrity Measurement Collector
// as required by
// https://www.trustedcomputinggroup.org/downloads/TNC/
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: sample_imc.c,v 1.5 2007/05/10 21:37:23 mikem Exp $

#ifdef WIN32
#define TNC_IMC_EXPORTS
#endif

#ifndef WIN32
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <libtncimc.h>

// Your Vendor ID. You _must_ use your unique IANA Private Enterprise Number
// as your vendor ID. Dont use someone elses, and certainly dont use the
// OSC number of 9048 in a production IMC. This is a bogus one
// used only for internal testing
#define VENDORID 9999

// This IMCID of this IMC module.
static TNC_IMCID myId = -1;

// Flag to indicate if this module has been initialized
static int initialized = 0;

// Pointers to callback functions in the TNCC
static TNC_TNCC_ReportMessageTypesPointer    reportMessageTypesP;
static TNC_TNCC_RequestHandshakeRetryPointer requestHandshakeRetryP;
static TNC_TNCC_SendMessagePointer           sendMessageP;

// List of message types this module is willing to receive
static TNC_MessageType messageTypes[] =
{
    // All generic TCG messages
    TNCMESSAGENUM(TNC_VENDORID_TCG, TNC_SUBTYPE_ANY),
    // Some private message types
    TNCMESSAGENUM(VENDORID, 2),
};

///////////////////////////////////////////////////////////
// Call TNC_TNCC_ReportMessageTypes in the TNCC
static TNC_Result reportMessageTypes(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_MessageTypeList supportedTypes,
    /*in*/ TNC_UInt32 typeCount)
{
    if (!reportMessageTypesP)
	return TNC_RESULT_FATAL;

    // Call the function in the TMCC
    return (*reportMessageTypesP)(imcID, supportedTypes, typeCount);
}

///////////////////////////////////////////////////////////
// Call TNC_TNCC_RequestHandshakeRetry in the TNCC
static TNC_Result requestHandshakeRetry(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_RetryReason reason)
{
    if (!requestHandshakeRetryP)
	return TNC_RESULT_FATAL;

    // Call the function in the TMCC
    return (*requestHandshakeRetryP)(imcID, connectionID, reason);
}

///////////////////////////////////////////////////////////
// Call TNC_TNCC_SendMessage in the TNCC
// This will eventually result in the message being delivered by 
// TNC_IMV_ReceiveMessage to all IMV at the server end who have 
// registered interest in the specified messageType
static TNC_Result sendMessage(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
    if (!sendMessageP)
	return TNC_RESULT_FATAL;

    // Call the function in the TMCC
    return (*sendMessageP)(imcID, connectionID, message, messageLength, messageType);
}

///////////////////////////////////////////////////////////
// Publically accessible functions as defined by TNC IF-IMC
///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
TNC_IMC_API TNC_Result TNC_IMC_Initialize(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_Version minVersion,
    /*in*/ TNC_Version maxVersion,
    /*out*/ TNC_Version *pOutActualVersion)
{
//    printf("TNC_IMC_Initialize %d %d %d\n", imcID, minVersion, maxVersion);

    if (initialized)
	return TNC_RESULT_ALREADY_INITIALIZED;

    // We only know about version 1 so far
    if (   minVersion < TNC_IFIMC_VERSION_1
	|| maxVersion > TNC_IFIMC_VERSION_1)
	return TNC_RESULT_NO_COMMON_VERSION;

    if (!pOutActualVersion)
	return TNC_RESULT_INVALID_PARAMETER;

    *pOutActualVersion = TNC_IFIMC_VERSION_1;
    myId = imcID;

    initialized++;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMC_API TNC_Result TNC_IMC_NotifyConnectionChange(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_ConnectionState newState)
{
//    printf("TNC_IMC_NotifyConnectionChange %d %d %d\n", imcID, connectionID, newState);
    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imcID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMC_API TNC_Result TNC_IMC_BeginHandshake(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID)
{
    char* msg = "this is a message";
//    printf("TNC_IMC_BeginHandshake %d %d\n", imcID, connectionID);

    if (!initialized)
    {
	printf("!initialized!\n");
	return TNC_RESULT_NOT_INITIALIZED;
    }
    
    if (imcID != myId)
    {
	printf("invalid parameter.\n");
	return TNC_RESULT_INVALID_PARAMETER;
    }

    // Just for testing, send a message:
    return sendMessage(imcID, connectionID, (TNC_BufferReference)msg, strlen(msg), TNCMESSAGENUM(VENDORID, 1));
}

///////////////////////////////////////////////////////////
TNC_IMC_API TNC_Result TNC_IMC_ReceiveMessage(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference messageBuffer,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
//    printf("TNC_IMC_ReceiveMessage %d %d %s %d %x\n", imcID, connectionID, messageBuffer, messageLength, messageType);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (messageType == TNCMESSAGENUM(VENDORID, 2))
    {
	char* msg = "this is a another message";
	return sendMessage(imcID, connectionID, (TNC_BufferReference)msg, strlen(msg), TNCMESSAGENUM(VENDORID, 3));
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMC_API TNC_Result TNC_IMC_BatchEnding(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID)
{
//    printf("TNC_IMC_BatchEnding %d %d\n", imcID, connectionID);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imcID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMC_API TNC_Result TNC_IMC_Terminate(
    /*in*/ TNC_IMCID imcID)
{
//    printf("TNC_IMC_Terminate %d %d\n", imcID);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imcID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    initialized = 0;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMC_API TNC_Result TNC_IMC_ProvideBindFunction(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_TNCC_BindFunctionPointer bindFunction)
{
//    printf("TNC_IMC_ProvideBindFunction %d %x\n", imcID, bindFunction);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imcID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    if (bindFunction)
    {
	// Look for required functions in the parent TMCC
	if ((*bindFunction)(imcID, "TNC_TNCC_ReportMessageTypes", 
			    (void**)&reportMessageTypesP) != TNC_RESULT_SUCCESS)
	    fprintf(stderr, "Could not bind function for TNC_TNCC_ReportMessageTypes\n");
	if ((*bindFunction)(imcID, "TNC_TNCC_RequestHandshakeRetry", 
			    (void**)&requestHandshakeRetryP) != TNC_RESULT_SUCCESS)
	    fprintf(stderr, "Could not bind function for TNC_TNCC_RequestHandshakeRetry\n");
	if ((*bindFunction)(imcID, "TNC_TNCC_SendMessage", 
			    (void**)&sendMessageP) != TNC_RESULT_SUCCESS)
	    fprintf(stderr, "Could not bind function for TNC_TNCC_SendMessage\n");
    }

    reportMessageTypes(imcID, messageTypes, sizeof(messageTypes) / sizeof(TNC_MessageType));
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// End of publically visible functions
///////////////////////////////////////////////////////////

