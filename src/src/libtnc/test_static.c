// test_static.c
//
// Test statically bound IMC and IMV modules
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: test_static.c,v 1.6 2009/03/18 11:28:35 mikem Exp mikem $

#include <libtncconfig.h>
#include <stdio.h>
#include <string.h>
#include <libtncimc.h>
#include <libtncimv.h>
#include "libtncarray.h"
#include "libtnchash.h"

// Support easy test management
#define TEST_VARS static int totaltests, testnum, errors = 0;
#define TEST_PLAN(n) {totaltests = n; printf("1..%d\n", n);}
#define OK(t,m) {if(!(t)){printf("not ok %d %s in %s line %d\n", ++testnum, m, __FILE__, __LINE__); errors++;} else {printf("ok %d\n", ++testnum);}}
#define TEST_END  {return errors ? 1 : testnum == totaltests ? 0 : 1;}

static TNC_IMCID        myImcID   = 1;
static TNC_IMVID        myImvID   = 2;
static TNC_ConnectionID myImcConnID  = 99;
static TNC_ConnectionID myImvConnID  = 199;

// Counters to indicate if our callbacks have been called
static tnccReportMessageTypes_was_called    = 0;
static tnccRequestHandshakeRetry_was_called = 0;
static tnccSendMessage_was_called           = 0;
static tncsReportMessageTypes_was_called    = 0;
static tncsRequestHandshakeRetry_was_called = 0;
static tncsSendMessage_was_called           = 0;
static tncsProvideRecommendation_was_called = 0;
static tncsGetAttribute_was_called          = 0;
static tncsSetAttribute_was_called          = 0;

TEST_VARS;

char* test_strings[] = 
{
    "",
    "1",
    "12",
    "123",
    "1234",
    "12345",
    "this is a string",
    "This is a very long string which should be broken into multiple lines"
    };

///////////////////////////////////////////////////////////
// Callbacks, called from within an IMC
TNC_Result TNC_TNCC_ReportMessageTypes(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_MessageTypeList supportedTypes,
    /*in*/ TNC_UInt32 typeCount)
{
//    printf("TNC_TNCC_ReportMessageTypes %d %x %d\n", imcID, supportedTypes, typeCount);
    OK((imcID == myImcID), "TNC_TNCC_ReportMessageTypes correct ID");

    tnccReportMessageTypes_was_called++;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCC_RequestHandshakeRetry(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_RetryReason reason)
{
//    printf("TNC_TNCC_RequestHandshakeRetry %d %d %d\n", imcID, connectionID, reason);
    OK((imcID == myImcID), "TNC_TNCC_RequestHandshakeRetry correct imc ID");
    OK((connectionID == myImcConnID), "TNC_TNCC_RequestHandshakeRetry correct connection ID");

    tnccRequestHandshakeRetry_was_called++;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCC_SendMessage(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
//    printf("TNC_TNCC_SendMessage %d %d %s %d %x\n", imcID, connectionID, message, messageLength, messageType);
    OK((imcID == myImcID), "TNC_TNCC_SendMessage correct imc ID");
    OK((connectionID == myImcConnID), "TNC_TNCC_SendMessage correct connection ID");

    tnccSendMessage_was_called++;
    // Deliver it to the IMV
    return TNC_IMV_ReceiveMessage(myImvID, myImvConnID, message, messageLength, messageType);
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCC_BindFunction(
    TNC_IMCID imcID,
    char *functionName,
    void **pOutfunctionPointer)
{
//    printf("TNC_TNCC_BindFunction %d %s\n", imcID, functionName);

    if (!strcmp(functionName, "TNC_TNCC_ReportMessageTypes"))
	*pOutfunctionPointer = (void*)TNC_TNCC_ReportMessageTypes;
    else if (!strcmp(functionName, "TNC_TNCC_RequestHandshakeRetry"))
	*pOutfunctionPointer = (void*)TNC_TNCC_RequestHandshakeRetry;
    else if (!strcmp(functionName, "TNC_TNCC_SendMessage"))
	*pOutfunctionPointer = (void*)TNC_TNCC_SendMessage;
    else
	return TNC_RESULT_INVALID_PARAMETER;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Callbacks, called from within an IMV
TNC_Result TNC_TNCS_ReportMessageTypes(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_MessageTypeList supportedTypes,
    /*in*/ TNC_UInt32 typeCount)
{
//    printf("TNC_TNCS_ReportMessageTypes %d %x %d\n", imvID, supportedTypes, typeCount);
    OK((imvID == myImvID), "TNC_TNCS_ReportMessageTypes correct ID");

    tncsReportMessageTypes_was_called++;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCS_RequestHandshakeRetry(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_RetryReason reason)
{
//    printf("TNC_TNCS_RequestHandshakeRetry %d %d %d\n", imvID, connectionID, reason);
    OK((imvID == myImvID), "TNC_TNCS_RequestHandshakeRetry correct imv ID");
    OK((connectionID == myImvConnID), "TNC_TNCS_RequestHandshakeRetry correct connection ID");

    tncsRequestHandshakeRetry_was_called++;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCS_ProvideRecommendation(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_IMV_Action_Recommendation recommendation,
    /*in*/ TNC_IMV_Evaluation_Result evaluation)
{
//    printf("TNC_TNCS_ProvideRecommendation %d %d %d %d\n", imvID, connectionID, recommendation, evaluation);
    OK((imvID == myImvID), "TNC_TNCS_ProvideRecommendation correct imv ID");
    OK((connectionID == myImvConnID), "TNC_TNCS_ProvideRecommendation correct connection ID");

    tncsProvideRecommendation_was_called++;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCS_SetAttribute(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_AttributeID attributeID,
    /*in*/ TNC_UInt32 bufferLength,
    /*in*/ TNC_BufferReference buffer)
{
//    printf("TNC_TNCS_SetAttribute %d %d %d\n", imvID, connectionID, attributeID);
    OK((imvID == myImvID), "TNC_TNCS_SetAttribute correct imv ID");
    OK((connectionID == myImvConnID), "TNC_TNCS_SetAttribute correct connection ID");

    tncsSetAttribute_was_called++;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCS_GetAttribute(
    /*in*/  TNC_IMVID imvID,
    /*in*/  TNC_ConnectionID connectionID,
    /*in*/  TNC_AttributeID attributeID,
    /*in*/  TNC_UInt32 bufferLength,
    /*out*/ TNC_BufferReference buffer,
    /*out*/ TNC_UInt32 *pOutValueLength)
{
//    printf("TNC_TNCS_GetAttribute %d %d %d\n", imvID, connectionID, attributeID);
    OK((imvID == myImvID), "TNC_TNCS_GetAttribute correct imv ID");
    OK((connectionID == myImvConnID), "TNC_TNCS_GetAttribute correct connection ID");

    *pOutValueLength = 0;
    tncsGetAttribute_was_called++;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCS_SendMessage(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
//    printf("TNC_TNCS_SendMessage %d %d %s %d %x\n", imvID, connectionID, message, messageLength, messageType );
    OK((imvID == myImvID), "TNC_TNCS_SendMessage correct imv ID");
    OK((connectionID == myImvConnID), "TNC_TNCS_SendMessage correct connection ID");

    tncsSendMessage_was_called++;
    // Deliver it to the IMC
    return TNC_IMC_ReceiveMessage(myImcID, myImcConnID, message, messageLength, messageType);
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCS_BindFunction(
    TNC_IMVID imvID,
    char *functionName,
    void **pOutfunctionPointer)
{
//    printf("TNC_TNCS_BindFunction %d %s\n", imvID, functionName);

    if (!strcmp(functionName, "TNC_TNCS_ReportMessageTypes"))
	*pOutfunctionPointer = (void*)TNC_TNCS_ReportMessageTypes;
    else if (!strcmp(functionName, "TNC_TNCS_RequestHandshakeRetry"))
	*pOutfunctionPointer = (void*)TNC_TNCS_RequestHandshakeRetry;
    else if (!strcmp(functionName, "TNC_TNCS_ProvideRecommendation"))
	*pOutfunctionPointer = (void*)TNC_TNCS_ProvideRecommendation;
    else if (!strcmp(functionName, "TNC_TNCS_SendMessage"))
	*pOutfunctionPointer = (void*)TNC_TNCS_SendMessage;
    else if (!strcmp(functionName, "TNC_TNCS_GetAttribute"))
	*pOutfunctionPointer = (void*)TNC_TNCS_GetAttribute;
    else if (!strcmp(functionName, "TNC_TNCS_SetAttribute"))
	*pOutfunctionPointer = (void*)TNC_TNCS_SetAttribute;
    else
	return TNC_RESULT_INVALID_PARAMETER;
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
int
main(int argc, char** argv)
{
    TNC_Version imcActualVersion;
    TNC_Version imvActualVersion;
    int i;

    TEST_PLAN(112); // How many tests we are going to run

    // Test base64 conversion
    for (i = 0; i < sizeof(test_strings) / sizeof(char*); i++)
    {
	char* encoded;
	size_t elen;
	char* decoded;
	size_t dlen;

	elen = encode_base64((unsigned char*)test_strings[i], strlen(test_strings[i]), &encoded);
	dlen = decode_base64(encoded, (unsigned char**)&decoded);
	OK((dlen == strlen(test_strings[i])), "decoded length");
	OK((strcmp(test_strings[i], decoded) == 0), "decoding");
	free(encoded);
	free(decoded);
    }

    // Test dynamic arrays
    {
	libtnc_array array1 = LIBTNC_ARRAY_INIT;
	libtnc_array array2 = LIBTNC_ARRAY_INIT;

	OK((libtnc_array_resize(&array2, 200) != 0), "libtnc_array_resize");

	OK((libtnc_array_pop(&array1) == (void*)0), "libtnc_array_pop");
	OK((libtnc_array_push(&array1, (void*)1) == 0), "libtnc_array_push");
	OK((libtnc_array_index(&array1, 0) == (void*)1), "libtnc_array_index");
	OK((libtnc_array_push(&array1, (void*)2) == 1), "libtnc_array_push");
	OK((libtnc_array_index(&array1, 1) == (void*)2), "libtnc_array_index");
	OK((libtnc_array_resize(&array1, 200) != 0), "libtnc_array_resize");
	OK((libtnc_array_index(&array1, 0) == (void*)1), "libtnc_array_index after resize");
	OK((libtnc_array_index(&array1, 1) == (void*)2), "libtnc_array_index after resize");

	OK((libtnc_array_pop(&array1) == (void*)2), "libtnc_array_pop");
	OK((libtnc_array_pop(&array1) == (void*)1), "libtnc_array_pop");
	OK((libtnc_array_pop(&array1) == (void*)0), "libtnc_array_pop");

	OK((libtnc_array_delete(&array1, 100) == NULL), "libtnc_array_delete");
	OK((libtnc_array_push(&array1, (void*)1) == 0), "libtnc_array_push");
	OK((libtnc_array_push(&array1, (void*)2) == 1), "libtnc_array_push");
	OK((libtnc_array_delete(&array1, 0) == (void*)1), "libtnc_array_delete");
	OK((libtnc_array_delete(&array1, 1) == (void*)2), "libtnc_array_delete");
	OK((libtnc_array_index(&array1, 0) == (void*)0), "libtnc_array_index");
	OK((libtnc_array_index(&array1, 1) == (void*)0), "libtnc_array_index");

	OK((libtnc_array_new(&array1, (void*)1) == 0), "libtnc_array_new");
	OK((libtnc_array_new(&array1, (void*)2) == 1), "libtnc_array_new");
	OK((libtnc_array_index(&array1, 0) == (void*)1), "libtnc_array_index after new");
	OK((libtnc_array_index(&array1, 1) == (void*)2), "libtnc_array_index after new");
	OK((libtnc_array_delete(&array1, 0) == (void*)1), "libtnc_array_delete after new");
	OK((libtnc_array_new(&array1, (void*)1) == 0), "libtnc_array_new after delete");

	OK((libtnc_array_free(&array1) == 1), "libtnc_array_free");
	OK((libtnc_array_free(&array2) == 1), "libtnc_array_free");

	
    }

    // Test hashes
    {
	libtnc_hash hash1 = LIBTNC_HASH_INIT;
	libtnc_hash *hash2 = libtnc_hash_create();
	
	OK((libtnc_hash_init(&hash1) == 1), "libtnc_hash_init");
	OK((libtnc_hash_free(&hash1, NULL) == 1), "libtnc_hash_free");
	OK((libtnc_hash_init(&hash1) == 1), "libtnc_hash_init again");
	OK((libtnc_hash_addS(&hash1, "key1", (void*)1) == 0), "libtnc_hash_addS");
	OK((libtnc_hash_addS(&hash1, "key1", (void*)1) == 1), "libtnc_hash_addS");
	OK((libtnc_hash_addS(&hash1, "key2", (void*)2) == 0), "libtnc_hash_addS");
	OK((libtnc_hash_addS(&hash1, "key3", (void*)3) == 0), "libtnc_hash_addS");
	OK((libtnc_hash_getS(&hash1, "key1") == (void*)1), "libtnc_hash_getS");
	OK((libtnc_hash_getS(&hash1, "key2") == (void*)2), "libtnc_hash_getS");
	OK((libtnc_hash_getS(&hash1, "key3") == (void*)3), "libtnc_hash_getS");
	OK((libtnc_hash_getS(&hash1, "key999999") == (void*)NULL), "libtnc_hash_getS");
	OK((libtnc_hash_deleteS(&hash1, "key2") == (void*)2), "libtnc_hash_deleteS");
	OK((libtnc_hash_deleteS(&hash1, "key2") == (void*)NULL), "libtnc_hash_deleteS");
	OK((libtnc_hash_addS(&hash1, "key2", (void*)2) == 0), "libtnc_hash_addS");
	OK((libtnc_hash_deleteS(&hash1, "key2") == (void*)2), "libtnc_hash_deleteS");
	OK((libtnc_hash_free(&hash1, NULL) == 1), "libtnc_hash_free");

	OK((libtnc_hash_destroy(hash2, NULL) == 1), "libtnc_hash_destroy");
    }

    ///////////////////////////////////////////////////////////
    // Initialise the sample IMC
    // Should fail: too soon
    OK((TNC_IMC_ProvideBindFunction(myImcID, TNC_TNCC_BindFunction) == TNC_RESULT_NOT_INITIALIZED), "TNC_IMC_ProvideBindFunction too early");
    // Badly:
    OK((TNC_IMC_Initialize(myImcID, 0, 0, &imcActualVersion) != TNC_RESULT_SUCCESS), "TNC_IMC_Initialize bad");
    // Goodly:
    OK((TNC_IMC_Initialize(myImcID, TNC_IFIMC_VERSION_1, TNC_IFIMC_VERSION_1, &imcActualVersion) == TNC_RESULT_SUCCESS), "TNC_IMC_Initialize");
    // Again, should fail
    OK((TNC_IMC_Initialize(myImcID, TNC_IFIMC_VERSION_1, TNC_IFIMC_VERSION_1, &imcActualVersion) == TNC_RESULT_ALREADY_INITIALIZED), "TNC_IMC_Initialize reinitialize");
    // Tell the IMC about us and our BindFunction
    OK((TNC_IMC_ProvideBindFunction(myImcID, TNC_TNCC_BindFunction) == TNC_RESULT_SUCCESS), "TNC_IMC_ProvideBindFunction");
    // Make sure TNC_TNCC_ReportMessageTypes was called somewhere above
    OK((tnccReportMessageTypes_was_called == 1), "TNC_TNCC_ReportMessageTypes called");


    ///////////////////////////////////////////////////////////
    // Initialise the sample IMV
    // Should fail: too soon
    OK((TNC_IMV_ProvideBindFunction(myImvID, TNC_TNCS_BindFunction) == TNC_RESULT_NOT_INITIALIZED), "TNC_IMV_ProvideBindFunction too early");
    // Badly:
    OK((TNC_IMV_Initialize(myImvID, 0, 0, &imvActualVersion) != TNC_RESULT_SUCCESS), "TNC_IMV_Initialize bad");
    // Goodly:
    OK((TNC_IMV_Initialize(myImvID, TNC_IFIMV_VERSION_1, TNC_IFIMV_VERSION_1, &imvActualVersion) == TNC_RESULT_SUCCESS), "TNC_IMV_Initialize");
    // Again, should fail
    OK((TNC_IMV_Initialize(myImvID, TNC_IFIMV_VERSION_1, TNC_IFIMV_VERSION_1, &imvActualVersion) == TNC_RESULT_ALREADY_INITIALIZED), "TNC_IMV_Initialize reinitialize");
    // Tell the IMV about us and our BindFunction
    OK((TNC_IMV_ProvideBindFunction(myImvID, TNC_TNCS_BindFunction) == TNC_RESULT_SUCCESS), "TNC_IMV_ProvideBindFunction");
    // Make sure TNC_TNCC_ReportMessageTypes was called somewhere above
    OK((tnccReportMessageTypes_was_called == 1), "TNC_TNCC_ReportMessageTypes called");

    ///////////////////////////////////////////////////////////
    // Create a new connection in the IMC
    OK((TNC_IMC_NotifyConnectionChange(myImcID, myImcConnID, TNC_CONNECTION_STATE_CREATE) == TNC_RESULT_SUCCESS), "TNC_IMC_NotifyConnectionChange CREATE");
    OK((TNC_IMC_NotifyConnectionChange(myImcID, myImcConnID, TNC_CONNECTION_STATE_HANDSHAKE) == TNC_RESULT_SUCCESS), "TNC_IMC_NotifyConnectionChange HANDSHAKE");

    ///////////////////////////////////////////////////////////
    // Create a new connection in the IMC
    OK((TNC_IMV_NotifyConnectionChange(myImvID, myImvConnID, TNC_CONNECTION_STATE_CREATE) == TNC_RESULT_SUCCESS), "TNC_IMV_NotifyConnectionChange CREATE");
    OK((TNC_IMV_NotifyConnectionChange(myImvID, myImvConnID, TNC_CONNECTION_STATE_HANDSHAKE) == TNC_RESULT_SUCCESS), "TNC_IMV_NotifyConnectionChange HANDSHAKE");

    // This will provoke an exchange of messages between the IMC and IMV
    OK((TNC_IMC_BeginHandshake(myImcID, myImcConnID) == TNC_RESULT_SUCCESS), "TNC_IMC_BeginHandshake");

    // Make sure TNC_TNCC_SendMessage was called from inside TNC_IMC_BeginHandshake
    OK((tnccSendMessage_was_called == 2), "TNC_TNCC_SendMessage called");
    OK((tncsSendMessage_was_called == 1), "TNC_TNCS_SendMessage called");
    OK((tncsProvideRecommendation_was_called == 1), "TNC_TNCS_ProvideRecommentation called");
    OK((tncsSetAttribute_was_called == 2), "TNC_TNCS_SetAttribute called");

    // Ask for a recommendation from the IMV (again)
    OK((TNC_IMV_SolicitRecommendation(myImvID, myImvConnID) == TNC_RESULT_SUCCESS), "TNC_IMC_SolicitRecommendation");
    OK((tncsProvideRecommendation_was_called == 2), "TNC_TNCS_ProvideRecommentation called again");
    OK((tncsSetAttribute_was_called == 4), "TNC_TNCS_SetAttribute called again");

    // End the messages in the IMC
    OK((TNC_IMC_BatchEnding(myImcID, myImcConnID) == TNC_RESULT_SUCCESS), "TNC_IMC_BatchEnding");
    OK((TNC_IMC_NotifyConnectionChange(myImcID, myImcConnID, TNC_CONNECTION_STATE_ACCESS_ALLOWED) == TNC_RESULT_SUCCESS), "TNC_IMC_NotifyConnectionChange ACCESS ALLOWED");
    OK((TNC_IMC_NotifyConnectionChange(myImcID, myImcConnID, TNC_CONNECTION_STATE_DELETE) == TNC_RESULT_SUCCESS), "TNC_IMC_NotifyConnectionChange DELETE");

    // End the messages in the IMV
    OK((TNC_IMV_BatchEnding(myImvID, myImvConnID) == TNC_RESULT_SUCCESS), "TNC_IMV_BatchEnding");
    OK((TNC_IMV_NotifyConnectionChange(myImvID, myImvConnID, TNC_CONNECTION_STATE_ACCESS_ALLOWED) == TNC_RESULT_SUCCESS), "TNC_IMV_NotifyConnectionChange ACCESS ALLOWED");
    OK((TNC_IMV_NotifyConnectionChange(myImvID, myImvConnID, TNC_CONNECTION_STATE_DELETE) == TNC_RESULT_SUCCESS), "TNC_IMV_NotifyConnectionChange DELETE");

    // Terminate the sample IMC
    OK((TNC_IMC_Terminate(myImcID) == TNC_RESULT_SUCCESS), "TNC_IMC_Terminate");

    // Terminate the sample IMV
    OK((TNC_IMV_Terminate(myImvID) == TNC_RESULT_SUCCESS), "TNC_IMV_Terminate");

    TEST_END;
}

