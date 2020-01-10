// sample_imv.c
//
// Sample Integrity Measurement Verifier
// as required by
// https://www.trustedcomputinggroup.org/downloads/TNC/
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: sample_imv.c,v 1.1 2007/05/10 21:37:23 mikem Exp $

#ifdef WIN32
#define TNC_IMV_EXPORTS
#endif

#ifndef WIN32
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <libtncimv.h>

// Your Vendor ID. You _must_ use your unique IANA Private Enterprise Number
// as your vendor ID. Dont use someone elses, and certainly dont use the
// OSC number of 9048 in a production IMV. This is a bogus one
// used only for internal testing
#define VENDORID 9999

// This IMVID of this IMV module.
static TNC_IMVID myId = -1;

// Flag to indicate if this module has been initialized
static int initialized = 0;

// Pointers to callback functions in the TNCS
static TNC_TNCS_ReportMessageTypesPointer    reportMessageTypesP;
static TNC_TNCS_RequestHandshakeRetryPointer requestHandshakeRetryP;
static TNC_TNCS_ProvideRecommendationPointer provideRecommendationP;
static TNC_TNCS_GetAttributePointer          getAttributeP;
static TNC_TNCS_SetAttributePointer          setAttributeP;
static TNC_TNCS_SendMessagePointer           sendMessageP;

// List of message types this module is willing to receive
static TNC_MessageType messageTypes[] =
{
    // All generic TCG messages
    TNCMESSAGENUM(TNC_VENDORID_TCG, TNC_SUBTYPE_ANY),
    // Some private message types
    TNCMESSAGENUM(VENDORID, 1),
    TNCMESSAGENUM(VENDORID, 3),
};

///////////////////////////////////////////////////////////
// Call TNC_TNCS_ReportMessageTypes in the TNCS
static TNC_Result reportMessageTypes(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_MessageTypeList supportedTypes,
    /*in*/ TNC_UInt32 typeCount)
{
    if (!reportMessageTypesP)
	return TNC_RESULT_FATAL;

    // Call the function in the TMCC
    return (*reportMessageTypesP)(imvID, supportedTypes, typeCount);
}

///////////////////////////////////////////////////////////
// Call TNC_TNCS_RequestHandshakeRetry in the TNCS
static TNC_Result requestHandshakeRetry(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_RetryReason reason)
{
    if (!requestHandshakeRetryP)
	return TNC_RESULT_FATAL;

    // Call the function in the TMCC
    return (*requestHandshakeRetryP)(imvID, connectionID, reason);
}

///////////////////////////////////////////////////////////
// Call TNC_TNCS_ProvideRecommendation in the TNCS
static TNC_Result provideRecommendation(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_IMV_Action_Recommendation recommendation,
    /*in*/ TNC_IMV_Evaluation_Result evaluation)
{
    if (!provideRecommendationP)
	return TNC_RESULT_FATAL;

    // Call the function in the TMCC
    return (*provideRecommendationP)(imvID, connectionID, recommendation, evaluation);
}

///////////////////////////////////////////////////////////
// Call TNC_TNCS_GetAttribute in the TNCS
static TNC_Result getAttribute(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/  TNC_AttributeID attributeID,
    /*in*/  TNC_UInt32 bufferLength,
    /*out*/ TNC_BufferReference buffer,
    /*out*/ TNC_UInt32 *pOutValueLength)
{
    if (!getAttributeP)
	return TNC_RESULT_FATAL;

    // Call the function in the TMCC
    return (*getAttributeP)(imvID, connectionID, attributeID, bufferLength, buffer, pOutValueLength);
}

///////////////////////////////////////////////////////////
// Call TNC_TNCS_SetAttribute in the TNCS
static TNC_Result setAttribute(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/  TNC_AttributeID attributeID,
    /*in*/  TNC_UInt32 bufferLength,
    /*out*/ TNC_BufferReference buffer)
{
    if (!setAttributeP)
	return TNC_RESULT_FATAL;

    // Call the function in the TMCC
    return (*setAttributeP)(imvID, connectionID, attributeID, bufferLength, buffer);
}

///////////////////////////////////////////////////////////
// Call TNC_TNCS_SendMessage in the TNCS
// This will eventually result in the message being delivered by 
// TNC_IMV_ReceiveMessage to all IMV at the server end who have 
// registered interest in the specified messageType
static TNC_Result sendMessage(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
    if (!sendMessageP)
	return TNC_RESULT_FATAL;

    // Call the function in the TMCC
    return (*sendMessageP)(imvID, connectionID, message, messageLength, messageType);
}

///////////////////////////////////////////////////////////
// Publically accessible functions as defined by TNC IF-IMV
///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_Initialize(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_Version minVersion,
    /*in*/ TNC_Version maxVersion,
    /*out*/ TNC_Version *pOutActualVersion)
{
//    printf("TNC_IMV_Initialize %d %d %d\n", imvID, minVersion, maxVersion);

    if (initialized)
	return TNC_RESULT_ALREADY_INITIALIZED;

    // We only know about version 1 so far
    if (   minVersion < TNC_IFIMV_VERSION_1
	|| maxVersion > TNC_IFIMV_VERSION_1)
	return TNC_RESULT_NO_COMMON_VERSION;

    if (!pOutActualVersion)
	return TNC_RESULT_INVALID_PARAMETER;

    *pOutActualVersion = TNC_IFIMV_VERSION_1;
    myId = imvID;

    initialized++;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_NotifyConnectionChange(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_ConnectionState newState)
{
//    printf("TNC_IMV_NotifyConnectionChange %d %d %d\n", imvID, connectionID, newState);
    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imvID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_SolicitRecommendation(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID)
{
//    printf("TNC_IMV_SolicitRecommendation %d %d\n", imvID, connectionID);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imvID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    // Just for testing, provide a recommendation:
    // IMVs may tell the TNCS about languages and resons
    setAttribute(imvID, connectionID, TNC_ATTRIBUTEID_REASON_LANGUAGE, 3, (TNC_BufferReference)"en");
    setAttribute(imvID, connectionID, TNC_ATTRIBUTEID_REASON_STRING, 8, (TNC_BufferReference)"testing");
    return provideRecommendation(imvID, connectionID, 
				 TNC_IMV_ACTION_RECOMMENDATION_ALLOW, 
				 TNC_IMV_EVALUATION_RESULT_COMPLIANT);
}

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_ReceiveMessage(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference messageBuffer,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
//    printf("TNC_IMV_ReceiveMessage %d %d %s %d %x\n", imvID, connectionID, messageBuffer, messageLength, messageType);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (messageType == TNCMESSAGENUM(VENDORID, 1))
    {
	char* msg = "this is a another message";
	return sendMessage(imvID, connectionID, (TNC_BufferReference)msg, strlen(msg), TNCMESSAGENUM(VENDORID, 2));
    }
    else if (messageType == TNCMESSAGENUM(VENDORID, 3))
    {
	// IMVs may tell the TNCS about languages and resons
	setAttribute(imvID, connectionID, TNC_ATTRIBUTEID_REASON_LANGUAGE, 2, (TNC_BufferReference)"en");
	setAttribute(imvID, connectionID, TNC_ATTRIBUTEID_REASON_STRING, 7, (TNC_BufferReference)"testing");
	return provideRecommendation(imvID, connectionID, TNC_IMV_ACTION_RECOMMENDATION_ISOLATE, TNC_IMV_EVALUATION_RESULT_NONCOMPLIANT_MAJOR);
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_BatchEnding(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID)
{
//    printf("TNC_IMV_BatchEnding %d %d\n", imvID, connectionID);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imvID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_Terminate(
    /*in*/ TNC_IMVID imvID)
{
//    printf("TNC_IMV_Terminate %d %d\n", imvID);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imvID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    initialized = 0;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_ProvideBindFunction(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_TNCS_BindFunctionPointer bindFunction)
{
//    printf("TNC_IMV_ProvideBindFunction %d %x\n", imvID, bindFunction);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imvID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    if (bindFunction)
    {
	// Look for required functions in the parent TMCC
	if ((*bindFunction)(imvID, "TNC_TNCS_ReportMessageTypes", 
			    (void**)&reportMessageTypesP) != TNC_RESULT_SUCCESS)
	    fprintf(stderr, "Could not bind function for TNC_TNCS_ReportMessageTypes\n");
	if ((*bindFunction)(imvID, "TNC_TNCS_RequestHandshakeRetry", 
			    (void**)&requestHandshakeRetryP) != TNC_RESULT_SUCCESS)
	    fprintf(stderr, "Could not bind function for TNC_TNCS_RequestHandshakeRetry\n");
	if ((*bindFunction)(imvID, "TNC_TNCS_ProvideRecommendation", 
			    (void**)&provideRecommendationP) != TNC_RESULT_SUCCESS)
	    fprintf(stderr, "Could not bind function for TNC_TNCS_ProvideRecommendation\n");
	if ((*bindFunction)(imvID, "TNC_TNCS_SendMessage", 
			    (void**)&sendMessageP) != TNC_RESULT_SUCCESS)
	    fprintf(stderr, "Could not bind function for TNC_TNCS_SendMessage\n");
	if ((*bindFunction)(imvID, "TNC_TNCS_GetAttribute", 
			    (void**)&getAttributeP) != TNC_RESULT_SUCCESS)
	    fprintf(stderr, "Could not bind optional function for TNC_TNCS_GetAttribute\n");
	if ((*bindFunction)(imvID, "TNC_TNCS_SetAttribute", 
			    (void**)&setAttributeP) != TNC_RESULT_SUCCESS)
	    fprintf(stderr, "Could not bind optional function for TNC_TNCS_SetAttribute\n");
    }

    reportMessageTypes(imvID, messageTypes, sizeof(messageTypes) / sizeof(TNC_MessageType));
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// End of publically visible functions
///////////////////////////////////////////////////////////

