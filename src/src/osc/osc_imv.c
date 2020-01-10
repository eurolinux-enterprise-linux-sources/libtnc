// osc_imv.c
//
// Open System Consultants Integrity Measurement Verifyer
// Conforms to the TNC IF-IMC spec as required by
// https://www.trustedcomputinggroup.org/specs/TNC
//
// This is a simple Unix and Windows Integrity Measurement Verifier
// which knows how to:
//  check the status of installed packages using rpm
//  can get details of files present on the client
//  can get details of RPM packages installed on the client
//  can get Windows registry values
//  can get the results of 'hardwired' external program
//
// It works with the matching OSC-IMC collector in osc_imc.c
//
// It reads a 'Policy' file which contains a specification of how
// to evaluate the security posture of the client by asking the OSC-IMC for
// information about the client the IMC is running on.
// The policy file is parsed according to the grammar in policy.y to generate a
// parse tree that represents the policy.
// The policy is executed for a given IMC connection by the code in policy_tree.c 
//
// Copyright (C) 2006-2008 Open System Consultants Pty Ltd
// Author: Mike McCauley (mikem@open.com.au)
// $Id: $

#ifdef WIN32
#define TNC_IMV_EXPORTS
#include <windows.h>
#define snprintf _snprintf
#else
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>  
#include <stdlib.h>
#include "osc_imv.h"
#include "policy_tree.h"

#ifndef WIN32
#include <strings.h>
#endif
#define MAX_KEY_LEN 500

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
static TNC_9048_LogMessagePointer            logMessageP;

// List of message types this module is willing to receive
static TNC_MessageType messageTypes[] =
{
    // All generic TCG messages
    TNCMESSAGENUM(TNC_VENDORID_TCG, TNC_SUBTYPE_ANY),

    // Some private message types
    OSC_MESSAGE_OS_DETAILS ,
    OSC_MESSAGE_PACKAGE_STATUS_REPLY,
    OSC_MESSAGE_FILE_STATUS_REPLY,
    OSC_MESSAGE_REGISTRY_REPLY,
    OSC_MESSAGE_EXTCOMMAND_REPLY
};

// This is the compiled policy. It is a parse tree of pt_node structures
pt_node* policy = NULL;

// This is the name of the file containing the policy
// It will be parsed by the parser in parser.y to produce a policy
// parse tree which is executed by policy_tree.c
#ifdef WIN32
char* policy_file = "C:\\osc_imv_policy.cfg";
#else
char* policy_file = "/etc/osc_imv_policy.cfg";
#endif

// This is a hash of connections. The key is the connection ID (as a binary integer)
// and the value is a pointer to an osc_imv_connection structure
// Must use a hash because the calling libtnc may not use simple small integers as 
// the connection ID
static libtnc_hash connections = LIBTNC_HASH_INIT;


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
// Call TNC_9048_LogMessage in the TNCC
// If it exists, it should log a diagnostic message.
// If it does not exist, use fprintf(stderr)
TNC_Result logMessage(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_UInt32 severity,
    /*in*/ const char* format, ...)
{
    va_list arg;
    va_start(arg, format);

    // Maybe send it to the logging function in our parent
    if (logMessageP)
    {
	char message[MAX_MESSAGE_LEN];
	vsnprintf(message, MAX_MESSAGE_LEN, format, arg);
	return (*logMessageP)(severity, message);
   }
	
    // No parent function, log it to stderr
    fprintf(stderr, "OSC_IMV message: ");
    vfprintf(stderr, format, arg);
    fprintf(stderr, "\n");

    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Split string s on character c, put the result in out, 
// up to at most max strings in out
// Return the number put into out.
// Munges the input string by inserting NULs
// s is expected to be NUL terminated
static int split(char* s, char c, char** out, int max)
{
    int i = 0; // Number set in out

    while (s && *s && i < max)
    {
	out[i++] = s;
	s = strchr(s, c);
	if (s)
	{
	    *s = '\0';
	    s++;
	}
    }
    return i;
}

// Functions for managing the connections hash follow:
static osc_imv_connection* connFind(
    /*in*/ TNC_ConnectionID connectionID)
{
    return libtnc_hash_get(&connections, 
			   (TNC_BufferReference)&connectionID, sizeof(connectionID));
}

// Free a connection structure
static void connFree(
    /*in*/ osc_imv_connection* conn)
{
    libtnc_hash_free(&conn->clientdata, free);
    free(conn);
}

static osc_imv_connection* connCreate(
    /*in*/ TNC_ConnectionID connectionID)
{
    osc_imv_connection* conn = NULL;

    if (conn = calloc(1, sizeof(osc_imv_connection)))
    {
	conn->connectionID   = connectionID;
	conn->recommendation = TNC_IMV_ACTION_RECOMMENDATION_NO_RECOMMENDATION;
	conn->evaluation     = TNC_IMV_EVALUATION_RESULT_DONT_KNOW;

	libtnc_hash_init(&conn->clientdata);
	void* oldConn = libtnc_hash_add(&connections, 
			(TNC_BufferReference)&connectionID, sizeof(connectionID), conn);
	if (oldConn)
	    connFree(oldConn);
    }
    return conn;
}

// Delete the connection from the connection list and return the conn pointer
static osc_imv_connection* connDelete(
    /*in*/ TNC_ConnectionID connectionID)
{
    return (osc_imv_connection*)libtnc_hash_delete
	(&connections, (TNC_BufferReference)&connectionID, sizeof(connectionID));
}

// Delete the connection from the connection list and destroy the connection 
// and all its storage
static void connDestroy(
    /*in*/ TNC_ConnectionID connectionID)
{
    osc_imv_connection* conn = connDelete(connectionID);

    if (conn)
	connFree(conn);
}

// Returns true if successful
TNC_Result connSetClientData(
    osc_imv_connection* conn,
    char* sysname, char* dataname, char* subsysname, char* value)
{
    char key[MAX_KEY_LEN];
    char* savevalue = strdup(value);
    char* oldvalue;

    snprintf(key, MAX_KEY_LEN, "%s.%s.%s", sysname, subsysname, dataname);
    oldvalue = (char*)libtnc_hash_addS(&conn->clientdata, key, (void*)savevalue);
    if (oldvalue)
	free(oldvalue);
    return 1;
}

// Dont auto-instantiate
char* connGetClientData(
    osc_imv_connection* conn,
    char* sysname, char* dataname, char* subsysname)
{
    char key[MAX_KEY_LEN];

    snprintf(key, MAX_KEY_LEN, "%s.%s.%s", sysname, subsysname, dataname);
    return (char*)libtnc_hash_getS(&conn->clientdata, key);
}

TNC_Result
connSendMessage(
    /*in*/ osc_imv_connection* conn,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
    return sendMessage(myId, conn->connectionID, message, messageLength, messageType);
}

// Request that the client send the requested data item
void
connRequestClientData(osc_imv_connection* conn, char* sysname, char* dataname)
{
    if (strcmp(sysname, "Registry") == 0)
    {
	connSendMessage(conn, 
			(TNC_BufferReference)dataname, 
			(TNC_UInt32)strlen(dataname), 
			OSC_MESSAGE_REGISTRY_REQUEST);
    }
    else if (strcmp(sysname, "Package") == 0)
    {
	connSendMessage(conn, 
			(TNC_BufferReference)dataname, 
			(TNC_UInt32)strlen(dataname), 
			OSC_MESSAGE_PACKAGE_STATUS_REQUEST);
    }
    else if (strcmp(sysname, "File") == 0)
    {
	connSendMessage(conn, 
			(TNC_BufferReference)dataname, 
			(TNC_UInt32)strlen(dataname), 
			OSC_MESSAGE_FILE_STATUS_REQUEST);
    }
    else if (strcmp(sysname, "Extcommand") == 0)
    {
	connSendMessage(conn, 
			(TNC_BufferReference)dataname, 
			(TNC_UInt32)strlen(dataname), 
			OSC_MESSAGE_EXTCOMMAND_REQUEST);
    }
    else
    {
    }
}

TNC_Result connSetRecommendation(
    osc_imv_connection* conn,
    TNC_IMV_Action_Recommendation recommendation)
{
    conn->recommendation = recommendation;
    return provideRecommendation(myId, conn->connectionID, 
				 conn->recommendation,
				 conn->evaluation);
}

TNC_Result connLogMessage(
    osc_imv_connection* conn,
    /*in*/ TNC_UInt32 severity,
    /*in*/ const char* format, ...)
{
    return logMessage(myId, severity, format);
}

TNC_Result connUserMessage(
    osc_imv_connection* conn,
    char* message)
{
    return connSendMessage(conn, 
			   (TNC_BufferReference)message, 
			   (TNC_UInt32)strlen(message), 
			   OSC_MESSAGE_USER_MESSAGE);
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
    osc_imv_connection* conn;

//    printf("TNC_IMV_NotifyConnectionChange %d %d %d\n", imvID, connectionID, newState);
    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imvID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    if (newState == TNC_CONNECTION_STATE_CREATE)
    {
	if (policy == NULL)
	{
	    // Need to load the policy file
	    // See if there is an attribute configured for the policy file name
	    char* env = getenv(ENV_IMV_POLICY_FILE);
	    if (env)
		policy_file = env;
	    
	    if ((policy = pt_parse_policy(policy_file)) == NULL)
	    {
		fprintf(stderr, "OSC_IMV: there was an error parsing the policy file %s\n", policy_file);
		return TNC_RESULT_FATAL;
	    }
	}

	conn = connCreate(connectionID);
    }
    else if (newState == TNC_CONNECTION_STATE_DELETE)
    {
	connDestroy(connectionID);
    }
    else
    {
	conn = connFind(connectionID);
    }

    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_SolicitRecommendation(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID)
{
    osc_imv_connection* conn;

//    printf("TNC_IMV_SolicitRecommendation %d %d\n", imvID, connectionID);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imvID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    // The connection needs to have been established beforehand
    if ((conn = connFind(connectionID)) == NULL)
	return TNC_RESULT_FATAL;
	
    // IMVs may tell the TNCS about languages and reasons
//    setAttribute(imvID, connectionID, TNC_ATTRIBUTEID_REASON_LANGUAGE, 3, (TNC_BufferReference)"en");
//    setAttribute(imvID, connectionID, TNC_ATTRIBUTEID_REASON_STRING, 8, (TNC_BufferReference)"testing");
    return provideRecommendation(imvID, connectionID, 
				 conn->recommendation,
				 conn->evaluation);
}

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_ReceiveMessage(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference messageBuffer,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
    osc_imv_connection* conn;
    char* strings[10];
    int   numstrings;

//    printf("TNC_IMV_ReceiveMessage %d %d %s %d %x\n", imvID, connectionID, messageBuffer, messageLength, messageType);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    // The connection needs to have been established beforehand
    if ((conn = connFind(connectionID)) == NULL)
	return TNC_RESULT_FATAL;
	

    // Figure out what sort of message came from the IMC and do something with it
    switch(messageType)
    {
	case OSC_MESSAGE_OS_DETAILS:
	    // Linux|nodename|release|version|machine|lang|username
	    // or
	    // Windows|majvers|minvers|build|platformid|csdversion|spmaj|spmin|suite|prodtype
	    // Save values to this client connection saved values table
	    numstrings = split((char*)messageBuffer, '|', strings, 10);
	    if (numstrings)
	    {
		if (strcmp(strings[0], "Linux") == 0)
		{
		    connSetClientData(conn, "System", "", "name",     strings[0]);
		    connSetClientData(conn, "System", "", "nodename", strings[1]);
		    connSetClientData(conn, "System", "", "release",  strings[2]);
		    connSetClientData(conn, "System", "", "version",  strings[3]);
		    connSetClientData(conn, "System", "", "machine",  strings[4]);
		    connSetClientData(conn, "System", "", "lang",     strings[5]);
		    connSetClientData(conn, "System", "", "user",     strings[6]);
		}
		else if (strcmp(strings[0], "Windows") == 0)
		{
		    connSetClientData(conn, "System", "", "name",             strings[0]);
		    connSetClientData(conn, "System", "", "majorversion",     strings[1]);
		    connSetClientData(conn, "System", "", "minorversion",     strings[2]);
		    connSetClientData(conn, "System", "", "buildnumber",      strings[3]);
		    connSetClientData(conn, "System", "", "platformid",       strings[4]);
		    connSetClientData(conn, "System", "", "csdversion",       strings[5]);
		    connSetClientData(conn, "System", "", "servicepackmajor", strings[6]);
		    connSetClientData(conn, "System", "", "servicepackminor", strings[7]);
		    connSetClientData(conn, "System", "", "suitemask",        strings[8]);
		    connSetClientData(conn, "System", "", "producttype",      strings[9]);
		}
		else
		{
		    // REVISIT: error ?
		}
	    }
	    break;

	case OSC_MESSAGE_PACKAGE_STATUS_REPLY:
	    numstrings = split((char*)messageBuffer, '|', strings, 10);
	    if (numstrings)
	    {
		connSetClientData(conn, "Package", strings[0], "status", strings[1]);
		connSetClientData(conn, "Package", strings[0], "version", strings[2]);
	    }
	    break;

	case OSC_MESSAGE_FILE_STATUS_REPLY:
	    numstrings = split((char*)messageBuffer, '|', strings, 10);
	    if (numstrings)
	    {
		connSetClientData(conn, "File", strings[0], "status", strings[1]);
		connSetClientData(conn, "File", strings[0], "size", strings[2]);
		connSetClientData(conn, "File", strings[0], "mode", strings[3]);
	    }
	    break;

	case OSC_MESSAGE_REGISTRY_REPLY:
	    numstrings = split((char*)messageBuffer, '|', strings, 10);
	    if (numstrings)
	    {
		connSetClientData(conn, "Registry", strings[0], "type", strings[1]);
		connSetClientData(conn, "Registry", strings[0], "value", strings[2]);
	    }
	    break;
	    
	case OSC_MESSAGE_EXTCOMMAND_REPLY:
	    numstrings = split((char*)messageBuffer, '|', strings, 10);
	    if (numstrings)
	    {
		connSetClientData(conn, "Extcommand", strings[0], "status", strings[1]);
		connSetClientData(conn, "Extcommand", strings[0], "result", strings[2]);
	    }
	    break;

    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_IMV_API TNC_Result TNC_IMV_BatchEnding(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID)
{
    osc_imv_connection* conn;
//    printf("TNC_IMV_BatchEnding %d %d\n", imvID, connectionID);

    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imvID != myId)
	return TNC_RESULT_INVALID_PARAMETER;
    // The connection needs to have been established beforehand
    if ((conn = connFind(connectionID)) == NULL)
	return TNC_RESULT_FATAL;
	
    // Call the policy evaluator to assess the posture of this client
    // This may result in requests for more data from the client, and may
    // result in the conn->recommendation being set
    if (!policy)
	return TNC_RESULT_FATAL;
    pt_evaluate(policy, conn);

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

    // Destroy the policy which we parsed at Initialize time
    if (policy)
    {
	pt_destroy(policy);
	policy = NULL;
    }
	    
    libtnc_hash_free(&connections, (void(*)(void*))connDestroy);
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
	// Look for a vendor extensions for logging and user messaging
	// Not an error if it does not exist
	(*bindFunction)(imvID, "TNC_9048_LogMessage", (void**)&logMessageP);

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

