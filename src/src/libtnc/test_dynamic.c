// test_dynamic.c
//
// Test dynamic loading of IMC and IMV modules
// Test correct operation of sample_imc and sample_imv
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: test_dynamic.c,v 1.8 2009/03/18 11:28:35 mikem Exp mikem $

#include <libtncconfig.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <libtncimc.h>
#include <libtncimv.h>


// Support easy test management
#define TEST_VARS static int totaltests, testnum, errors = 0;
#define TEST_PLAN(n) {totaltests = n; printf("1..%d\n", n);}
#define OK(t,m) {if(!(t)){printf("not ok %d %s in %s line %d\n", ++testnum, m, __FILE__, __LINE__); errors++;} else {printf("ok %d\n", ++testnum);}}
#define TEST_END  {return errors ? 1 : testnum == totaltests ? 0 : 1;}

static TNC_ConnectionID myImcConnID  = 10;
static TNC_ConnectionID myImvConnID  = 11;

// Counters to indicate if our callbacks have been called
static tnccRequestHandshakeRetry_was_called = 0;
static tnccSendMessage_was_called           = 0;
static tncsRequestHandshakeRetry_was_called = 0;
static tncsProvideRecommendation_was_called = 0;
static tncsGetAttribute_was_called          = 0;
static tncsSetAttribute_was_called          = 0;
static tncsSendMessage_was_called           = 0;

TEST_VARS;

static int suppress_log_messages = 0;
static int log_message_was_called = 0;

// Names of the test libraries
#if HOST_IS_CYGWIN
// Cygwin generates funny named DLLs
static const char* imcmodule = "../sample/.libs/cygsample_imc-0.dll";
static const char* imvmodule = "../sample/.libs/cygsample_imv-0.dll";
#elif HOST_IS_DARWIN
// Darwin generates funny named DLLs
static const char* imcmodule = "../sample/.libs/libsample_imc.dylib";
static const char* imvmodule = "../sample/.libs/libsample_imv.dylib";
#elif HOST_IS_WINDOWS_VS2005
static const char* imcmodule = ".\\sample_imc.dll";
static const char* imvmodule = ".\\sample_imv.dll";
#else
static const char* imcmodule = "../sample/.libs/libsample_imc.so";
static const char* imvmodule = "../sample/.libs/libsample_imv.so";
#endif

// Override the log message fn in libtnc so we can instrument it
TNC_Result libtnc_logMessage(
    /*in*/ TNC_UInt32 severity,
    /*in*/ const char* format, ...)
{
    va_list arg;
    va_start(arg, format);

    log_message_was_called++;
    if (!suppress_log_messages)
	vfprintf(stderr, format, arg);
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCC_RequestHandshakeRetry(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_RetryReason reason)
{
//    printf("TNC_TNCC_RequestHandshakeRetry %d %d %d\n", imcID, connectionID, reason);

    tnccRequestHandshakeRetry_was_called++;
    // TODO: restart handshake
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
//    printf("TNC_TNCC_SendMessage %d %d %s %d %x\n", imcID, connectionID, message, messageLength, messageType );

    OK((connectionID == myImcConnID), "TNC_TNCC_SendMessage correct connection ID");

    tnccSendMessage_was_called++;

    // Forward to IMV
    return libtnc_imv_ReceiveMessage(myImvConnID, message, messageLength, messageType);
}

///////////////////////////////////////////////////////////
TNC_Result TNC_TNCS_RequestHandshakeRetry(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_RetryReason reason)
{
//    printf("TNC_TNCS_RequestHandshakeRetry %d %d %d\n", imvID, connectionID, reason);

    tncsRequestHandshakeRetry_was_called++;
    // TODO: restart handshake
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

    OK((connectionID == myImvConnID), "TNC_TNCS_SendMessage correct connection ID");

    tncsSendMessage_was_called++;

    // Forward to IMC
    return libtnc_imc_ReceiveMessage(myImcConnID, message, messageLength, messageType);
}

///////////////////////////////////////////////////////////
int
main(int argc, char** argv)
{
    libtnc_imc* imc;
    libtnc_imv* imv;
    const char* imcmodules[] = {imcmodule};
    int         num_imcmodules = sizeof(imcmodules) / sizeof(const char*);
    const char* imvmodules[] = {imvmodule};
    int         num_imvmodules = sizeof(imvmodules) / sizeof(const char*);

    TEST_PLAN(49);
    
    ///////////////////////////////////////////////////////////
    // Test loading of IMC
    // will fail. We dont usually use this function.
    // Prevent messages appearing on stderr, but count them
    suppress_log_messages = 1;
    OK(((imc = libtnc_imc_new("ignore.this.dlopen.error.message")) == NULL), "libtnc_imc_new unknown filename");
    OK((log_message_was_called == 1),  "IMC error count");
    suppress_log_messages = 0;
    // should succeed
    OK((libtnc_imc_load_modules(imcmodules, num_imcmodules) == TNC_RESULT_SUCCESS), "libtnc_load_modules");
    // Should fail
    OK((libtnc_imc_load_config("ignore.this.TNC.config.error.message") == -1), "libtnc_imc_load_config bad filename");
    // should succeed
    OK((log_message_was_called == 1),  "IMC load error count");

    ///////////////////////////////////////////////////////////
    // Test loading of IMV
    // will fail. We dont usually use this function.
    // Prevent messages appearing on stderr, but count them
    suppress_log_messages = 1;
    OK(((imv = libtnc_imv_new("ignore.this.dlopen.error.message")) == NULL), "libtnc_imv_new unknown filename");
    OK((log_message_was_called == 2),  "IMV error count");
    suppress_log_messages = 0;
    // should succeed
    OK((libtnc_imv_load_modules(imvmodules, num_imvmodules) == TNC_RESULT_SUCCESS), "libtnc_load_modules");
    // Should fail
    OK((libtnc_imv_load_config("ignore.this.TNC.config.error.message") == -1), "libtnc_imv_load_config bad filename");
    // should succeed
    OK((log_message_was_called == 2),  "IMV load error count");


    ///////////////////////////////////////////////////////////
    // Create a new connection in all IMCs
    OK((libtnc_imc_NotifyConnectionChange(myImcConnID, TNC_CONNECTION_STATE_CREATE) == TNC_RESULT_SUCCESS), "libtnc_imc_NotifyConnectionChange CREATE");
    OK((libtnc_imc_NotifyConnectionChange(myImcConnID, TNC_CONNECTION_STATE_HANDSHAKE) == TNC_RESULT_SUCCESS), "libtnc_imc_NotifyConnectionChange CREATE");

    ///////////////////////////////////////////////////////////
    // Create a new connection in all IMVs
    OK((libtnc_imv_NotifyConnectionChange(myImvConnID, TNC_CONNECTION_STATE_CREATE) == TNC_RESULT_SUCCESS), "libtnc_imv_NotifyConnectionChange CREATE");
    OK((libtnc_imv_NotifyConnectionChange(myImvConnID, TNC_CONNECTION_STATE_HANDSHAKE) == TNC_RESULT_SUCCESS), "libtnc_imv_NotifyConnectionChange CREATE");

    // This will provoke an exchange of messages between the IMC and IMV
    OK((libtnc_imc_BeginHandshake(myImcConnID) == TNC_RESULT_SUCCESS), "libtnc_imc_BeginHandshake");

    // Make sure TNC_TNCC_SendMessage was called from inside TNC_IMC_BeginHandshake
    OK((tnccSendMessage_was_called == 2), "TNC_TNCC_SendMessage called");
    OK((tncsSendMessage_was_called == 1), "TNC_TNCS_SendMessage called");
    OK((tncsProvideRecommendation_was_called == 1), "TNC_TNCS_ProvideRecommendation called");
    OK((tncsSetAttribute_was_called == 2), "TNC_TNCS_SetAttribute called");

    OK((libtnc_imv_SolicitRecommendation(myImvConnID) == TNC_RESULT_SUCCESS), "libtnc_imc_SolicitRecommendation");
    OK((tncsProvideRecommendation_was_called == 2), "TNC_TNCS_ProvideRecommentation called again");
    OK((libtnc_imv_SolicitRecommendation(myImvConnID) == TNC_RESULT_SUCCESS), "libtnc_imc_SolicitRecommendation");
    OK((tncsProvideRecommendation_was_called == 3), "TNC_TNCS_ProvideRecommentation not called again");

    // Send a message to all IMCs that have registered interest in this
    // message type
    {
	char* msg = "this is a reply";

	// This should fail due illegal message type
	OK((libtnc_imc_ReceiveMessage(myImcConnID, (TNC_BufferReference)msg, strlen(msg), TNCMESSAGENUM(9999, 0xff))  == TNC_RESULT_INVALID_PARAMETER), "libtnc_imc_ReceiveMessage invalid message type");

	// This should fail due illegal vendor id
	OK((libtnc_imc_ReceiveMessage(myImcConnID, (TNC_BufferReference)msg, strlen(msg), TNCMESSAGENUM(0xffffff, 0))  == TNC_RESULT_INVALID_PARAMETER), "libtnc_imc_ReceiveMessage invalid vendor id");

	// This should succeed but not be delivered to anyone
	OK((libtnc_imc_ReceiveMessage(myImcConnID, (TNC_BufferReference)msg, strlen(msg), TNCMESSAGENUM(9999, 999))  == TNC_RESULT_SUCCESS), "libtnc_imc_ReceiveMessage unregistered message type");
	
	// This should be delivered to sample_imc
	OK((libtnc_imc_ReceiveMessage(myImcConnID, (TNC_BufferReference)msg, strlen(msg), TNCMESSAGENUM(9999, 2))  == TNC_RESULT_SUCCESS), "libtnc_imc_ReceiveMessage");    
    }

    OK((libtnc_imc_BatchEnding(myImcConnID) == TNC_RESULT_SUCCESS), "libtnc_imc_BatchEnding");

    // TODO: check IMV result

    OK((libtnc_imc_NotifyConnectionChange(myImcConnID, TNC_CONNECTION_STATE_ACCESS_ALLOWED) == TNC_RESULT_SUCCESS), "TNC_IMC_NotifyConnectionChange ACCESS ALLOWED");
    OK((libtnc_imc_NotifyConnectionChange(myImcConnID, TNC_CONNECTION_STATE_DELETE) == TNC_RESULT_SUCCESS), "TNC_IMC_NotifyConnectionChange DELETE");

    OK((libtnc_imc_Terminate() == TNC_RESULT_SUCCESS), "libtnc_imc_Terminate");
    OK((libtnc_imv_Terminate() == TNC_RESULT_SUCCESS), "libtnc_imv_Terminate");

    // Unload all extant libraries
    OK((log_message_was_called == 2),  "error count");

    TEST_END;
}
