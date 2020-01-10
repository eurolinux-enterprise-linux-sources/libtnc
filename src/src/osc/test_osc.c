// test.c
//
// Test the OSC-IMV pair
//
// Copyright (C) 2008 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: test_osc.c,v 1.1 2008/05/07 03:19:11 mikem Exp mikem $

#include <libtncconfig.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <base64.h>
#include <libtnctncc.h>
#include <libtnctncs.h>
#include <libtncarray.h>

// Support easy test management
#define TEST_VARS static int totaltests, testnum, errors = 0;
#define TEST_PLAN(n) {totaltests = n; printf("1..%d\n", n);}
#define OK(t,m) {if(!(t)){printf("not ok %d %s in %s line %d\n", ++testnum, m, __FILE__, __LINE__); errors++;} else {printf("ok %d\n", ++testnum);}}
#define TEST_END  {return errors ? 1 : testnum == totaltests ? 0 : 1;}

static libtnc_tncc_connection* myImcConn;
static libtnc_tncs_connection* myImvConn;
static tnccSendBatch_was_called = 0;
static tncsSendBatch_was_called = 0;

#if HOST_IS_CYGWIN
// Cygwin generates funny named DLLs
static const char* config_file = "test_tnc_config.cygwin";
#elif HOST_IS_DARWIN
// Darwin generates funny named DLLs
static const char* config_file = "test_tnc_config.darwin";
#else
static const char* config_file = "test_tnc_config";
#endif

char* policy_file = "policy_test.dat";

TEST_VARS;

static int suppress_log_messages = 0;
static int log_message_was_called = 0;

// Called by TNCC when a finished batch is ready to send
TNC_Result TNC_TNCC_SendBatch(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ const char* messageBuffer,
    /*in*/ size_t messageLength)
{
//    printf("TNC_TNCC_SendBatch: %s\n", messageBuffer);
    tnccSendBatch_was_called++;
    OK((conn == myImcConn), "TNC_TNCC_SendBatch connection context");
    return libtnc_tncs_ReceiveBatch(myImvConn, messageBuffer, messageLength);
}

// Called by TNCS when a finished batch is ready to send
TNC_Result TNC_TNCS_SendBatch(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ const char* messageBuffer,
    /*in*/ size_t messageLength)
{
//    printf("TNC_TNCS_SendBatch: %s\n", messageBuffer);
    tncsSendBatch_was_called++;
    OK((conn == myImvConn), "TNC_TNCS_SendBatch connection context");
    return libtnc_tncc_ReceiveBatch(myImcConn, messageBuffer, messageLength);
}

// Override the log message fn in libtnc so we can instrument it
TNC_Result libtnc_logMessage(
    /*in*/ TNC_UInt32 severity,
    /*in*/ const char* format, ...)
{
    va_list arg;
    va_start(arg, format);

    log_message_was_called++;
    if (!suppress_log_messages)
    {
	fprintf(stderr, "test_osc.c logMessage: ");
	vfprintf(stderr, format, arg);
	fprintf(stderr, "\n");
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
int
main(int argc, char** argv)
{
    TNC_UInt32 buffLen;
    const char buffer[100];
    TEST_PLAN(20);

    // Make sure we load up the right file
    setenv("OSC_IMV_POLICY_FILE", "policy_test.dat", 1);

    // Load TNCC. Production code should use libtnc_tncc_InitializeStd
    OK((libtnc_tncc_Initialize(config_file) == TNC_RESULT_SUCCESS), "libtnc_tncc_InitializeFromFile");

    // Load TNCS. Production code should use libtnc_tncs_Initialize
    OK((libtnc_tncs_Initialize(config_file) == TNC_RESULT_SUCCESS), "libtnc_tncs_InitializeFromFile");

    // Configure a preferred language
    OK((libtnc_tncc_PreferredLanguage("en") == TNC_RESULT_SUCCESS), "libtnc_tncs_PreferredLanguage");

    suppress_log_messages = 1;
    OK(((myImcConn = libtnc_tncc_CreateConnection((void*)1)) != NULL), "libtnc_tncc_CreateConnection");
    OK(((myImvConn = libtnc_tncs_CreateConnection((void*)1)) != NULL), "libtnc_tncs_CreateConnection");

    // Make sure we can recover app data
    OK((libtnc_tncc_ConnectionAppData(myImcConn) == (void*)1), "libtnc_tncc_ConnectionAppData");

    // This should trigger the first batch to be sent
    OK((libtnc_tncs_BeginSession(myImvConn) == TNC_RESULT_SUCCESS), "libtnc_tncs_BeginSession");
    // This should trigger the first batch to be sent
    OK((libtnc_tncc_BeginSession(myImcConn) == TNC_RESULT_SUCCESS), "libtnc_tncc_BeginSession");

    OK((tnccSendBatch_was_called == 2), "TNCC SendBatch was called");
    OK((tncsSendBatch_was_called == 2), "TNCS SendBatch was called");
    OK((log_message_was_called == 1),  "Log message count");
    OK((libtnc_tncs_HaveRecommendation(myImvConn, NULL, NULL) == TNC_RESULT_SUCCESS), "libtnc_tncs_HaveRecommendation");
    // Expect we are going to delete id 0
    
    OK((libtnc_tncc_DeleteConnection(myImcConn) == 0), "libtnc_tncc_DeleteConnection");
    OK((libtnc_tncs_DeleteConnection(myImvConn) == 0), "libtnc_tncs_DeleteConnection");
    
    OK((libtnc_tncc_Terminate() == TNC_RESULT_SUCCESS), "libtnc_tncc_Terminate");
    OK((libtnc_tncs_Terminate() == TNC_RESULT_SUCCESS), "libtnc_tncs_Terminate");

    TEST_END;
	
}
