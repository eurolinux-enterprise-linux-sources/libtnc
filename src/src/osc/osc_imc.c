// osc_imc.c
//
// Open System Consultants Integrity Measurement Collector
// Conforms to the TNC IF-IMC spec as required by
// https://www.trustedcomputinggroup.org/specs/TNC
//
// This is a simple Unix and Windows Integrity Measurement Collector
// which knows how to:
//  check the status of installed packages using rpm
//  can get details of files present on the client
//  can get Windows registry values
//  can get the results of 'hardwired' external program
//
// It works with the matching OSC-IMV verifier in osc_imv.c
//
// Copyright (C) 2006-2008 Open System Consultants Pty Ltd
// Author: Mike McCauley (mikem@open.com.au)
// $Id: osc_imc.c,v 1.8 2007/05/10 21:37:23 mikem Exp $

#ifdef WIN32
#define TNC_IMC_EXPORTS
#include <windows.h>
#define snprintf _snprintf
#else
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>  
#ifndef WIN32
#include <sys/utsname.h>
#include <errno.h>
#include <sys/wait.h>
#include <pwd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <libtncimc.h>
#include "osc_imc.h"

#define RPMPATH "rpm"


// This IMCID of this IMC module.
static TNC_IMCID myId = -1;

// Flag to indicate if this module has been initialized
static int initialized = 0;

// Pointers to callback functions in the TNCC
static TNC_TNCC_ReportMessageTypesPointer    reportMessageTypesP;
static TNC_TNCC_RequestHandshakeRetryPointer requestHandshakeRetryP;
static TNC_TNCC_SendMessagePointer           sendMessageP;
static TNC_9048_LogMessagePointer            logMessageP;
static TNC_9048_UserMessagePointer           userMessageP;

// List of message types this module is willing to receive
static TNC_MessageType messageTypes[] =
{
    OSC_MESSAGE_PACKAGE_STATUS_REQUEST,
    OSC_MESSAGE_FILE_STATUS_REQUEST,
    OSC_MESSAGE_REGISTRY_REQUEST,
    OSC_MESSAGE_EXTCOMMAND_REQUEST,
    OSC_MESSAGE_USER_MESSAGE
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
// Call TNC_9048_LogMessage in the TNCC
// If it exists, it should log a diagnostic message.
// If it does not exist, use fprintf(stderr)
static TNC_Result logMessage(
    /*in*/ TNC_IMCID imcID,
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
    fprintf(stderr, "OSC_IMC message: ");
    vfprintf(stderr, format, arg);
    fprintf(stderr, "\n");

    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Run a command, return its exit status
// maybe capture its stdout
// return -1 on error
// FIME: runProg is not very good at telling whether
// a program succeeded if waitfor is not set
static int runProg(
    /*in*/ TNC_IMCID    imcID,
    /*in*/  const char* progfile,
    /*in*/  const char* argv[],
    /*in*/  int         waitfor,
    /*out*/ char*       output,
    /*in*/  int         output_size)
{
#ifdef WIN32
#else
    int filedes1[2], filedes2[2];
    int pid;

    // Make pipes to communicate with the child
    if (   pipe(filedes1) == -1
	|| pipe(filedes2) == -1)
    {
	logMessage(imcID, TNC_LOG_SEVERITY_ERR, 
		   "runProg could not create pipes: %s\n", strerror(errno));
	return -1;
    }

    // Fork a child
    if ((pid = fork()) == 0)
    {
	dup2(filedes1[0], fileno(stdin)); 
	dup2(filedes2[1], fileno(stdout));
      
	if (execvp(progfile, argv)) // Should never return
        {
	    logMessage(imcID, TNC_LOG_SEVERITY_ERR, 
		       "runProg could not execvp %s: %s\n", progfile, strerror(errno));
	    exit(-1);
        }
    }
    else if (pid == -1)
    {
	logMessage(imcID, TNC_LOG_SEVERITY_ERR, 
		   "runProg could not fork: %s\n", strerror(errno));
	return -1;
    }
    else if (waitfor)
    {
	// Fork succeeded, need to wait for completion
	int  status;
	char ch;
	int  i = 0;

	// Close the unused ends of the pipes, so we dont hang on read
	close(filedes1[0]);
	close(filedes2[1]);

	// Read everything the program produces and save the first 
	// output_size bytes
	while (read(filedes2[0], &ch, 1) > 0)
	{
	    if (output && i < output_size - 1) // Dont overflow the output buffer
		output[i++] = ch;
	}
	if (output) output[i] = '\0';

	// Find out what happened to the child
	if (waitpid(pid, &status, WNOHANG) == -1)
	{
	    logMessage(imcID, TNC_LOG_SEVERITY_ERR, 
		       "runProg could not waitpid: %s\n", strerror(errno));
	    return -1;
	}
	return status;
    }
    else
    {
	return 0;
    }
#endif
}

///////////////////////////////////////////////////////////
static TNC_Result checkPackage(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ char* packagename)
{
#ifdef WIN32
    // Dont know how to do this on Windows
    return TNC_RESULT_FATAL;

#else
    int         result;
    char        version[MAX_MESSAGE_LEN];
    const char* query_argv[] = {RPMPATH, "-q", packagename, NULL};
    const char* verify_argv[] = {RPMPATH, "-V", packagename, NULL};
    char        reply[MAX_MESSAGE_LEN];
    int         reply_size;

    // Run rpm to find out about the package and capture its output version number
    result = runProg(imcID, query_argv[0], query_argv, 1, version, MAX_MESSAGE_LEN);
    if (result == -1)
	return TNC_RESULT_FATAL;
    // Remove any trailing newline from the version string
    if (strlen(version) && version[strlen(version) - 1] == '\n')
	version[strlen(version) - 1] = '\0';

    // Verify the package is installed OK
    result = runProg(imcID, query_argv[0], query_argv, 1, NULL, 0);
    if (result == -1)
	return TNC_RESULT_FATAL;

    // Format the reply message
    reply_size = snprintf(reply, MAX_MESSAGE_LEN, "%s|%d|%s", packagename, result, version); 

    // Send the reply back
    return sendMessage(imcID, connectionID, (TNC_BufferReference)reply, reply_size, 
		       OSC_MESSAGE_PACKAGE_STATUS_REPLY);
#endif
	return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
static TNC_Result checkFile(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ char* filename)
{
// Silly windows has a different naming convention for stat()
#ifdef WIN32
#define STATBUF struct _stat
#define STAT _stat
#else
#define STATBUF struct stat
#define STAT stat
#endif

    char        reply[MAX_MESSAGE_LEN];
    int         reply_size;
    STATBUF     statbuf;

    // Format the reply message
    if (STAT(filename, &statbuf) == 0)
    {
	// Stat succeeded, get file info
	reply_size = snprintf(reply, MAX_MESSAGE_LEN, "%s|%d|%d|%d", 
			      filename, 0, statbuf.st_size, statbuf.st_mode); 
    }
    else
    {
	// stat failed
	reply_size = snprintf(reply, MAX_MESSAGE_LEN, "%s|%d", filename, errno); 
    }
    // Send the reply back
    return sendMessage(imcID, connectionID, (TNC_BufferReference)reply, reply_size, 
		       OSC_MESSAGE_FILE_STATUS_REPLY);
}

///////////////////////////////////////////////////////////
static TNC_Result checkRegistry(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ char* keyname)
{
#ifdef WIN32
    char        reply[MAX_MESSAGE_LEN];
    int         reply_size = 0;
    HKEY        key;
    char*       valuename;
    char*       keynamecopy = strdup(keyname);

    if (!keynamecopy)
	return TNC_RESULT_FATAL;
	
    if (valuename = strrchr(keynamecopy, '\\'))
        *valuename++ = 0;
    else
	return TNC_RESULT_FATAL;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keynamecopy, 0, KEY_READ, &key) == ERROR_SUCCESS)
    {
	DWORD       varType;
	char        buf[100];
	DWORD       bufSize = 100;

	if (RegQueryValueEx(key, valuename, NULL, &varType, buf, &bufSize) == ERROR_SUCCESS)
	{
	    switch (varType)
	    {
		case REG_SZ:
		case REG_EXPAND_SZ:
		    reply_size = snprintf(reply, MAX_MESSAGE_LEN, "%s|%d|%s", keyname, varType, buf);
		    break;

		case REG_DWORD_LITTLE_ENDIAN:
		case REG_DWORD_BIG_ENDIAN:
		    reply_size = snprintf(reply, MAX_MESSAGE_LEN, "%s|%d|%d", keyname, varType, *(DWORD*)buf);
		    break;

		default:
		    reply_size = snprintf(reply, MAX_MESSAGE_LEN, "%s|%d|UNABLETOCONVERT", keyname, varType);
		    break;
	    }
	}
	RegCloseKey(key);
    }
    if (!reply_size)
    {
	reply_size = snprintf(reply, MAX_MESSAGE_LEN, "%s:-1:UNABLETOFIND", keyname);
    }
    free(keynamecopy);
    // Send the reply back
    return sendMessage(imcID, connectionID, 
		       (TNC_BufferReference)reply, reply_size, 
		       OSC_MESSAGE_REGISTRY_REPLY);
#else
    // Cant do this on Unix
    return TNC_RESULT_FATAL;
#endif
}


///////////////////////////////////////////////////////////
// Most of this code generously contributed by Kevin Koster of Cloudpath Networks
static TNC_Result externalCommand(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ char* argstring)
{
    char        reply[MAX_MESSAGE_LEN];
    int         reply_size = 0;

#ifdef WIN32
#define BUFSIZE 4096
    HANDLE hChildStdinRd, hChildStdinWr, hChildStdoutRd, hChildStdoutWr;
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bFuncRetn = FALSE;
    TCHAR szCmd[] = TEXT(OSC_IMC_EXTERNAL_COMMAND);
    TCHAR *szCmdline;
    SECURITY_ATTRIBUTES saAttr; 
    BOOL fSuccess; 
    
    // Set the bInheritHandle flag so pipe handles are inherited. 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
    
    // Create a pipe for the child process's STDOUT. 
    if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
    {
	fprintf(stderr, "Stdout pipe creation failed\n"); 
	return TNC_RESULT_FATAL;
    }
    // Ensure that the read handle to the child process's pipe for STDOUT is not inherited.
    SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);
    
    // Create a pipe for the child process's STDIN. 
    if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) 
    {
	fprintf(stderr, "Stdin pipe creation failed\n"); 
	return TNC_RESULT_FATAL;
    }
    // Ensure that the write handle to the child process's pipe for STDIN is not inherited. 
    SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0);
 
    // Build a command line
    szCmdline = malloc(strlen(szCmd) + strlen(argstring) + 2);
    sprintf(szCmdline, "%s %s", szCmd, argstring);
    
    // Set up members of the PROCESS_INFORMATION structure.
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    
    // Set up members of the STARTUPINFO structure. 
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStdoutWr;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.hStdInput = hChildStdinRd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    
    // Create the child process.  
    bFuncRetn = CreateProcess(NULL, 
			      szCmdline,     // command line 
			      NULL,          // process security attributes 
			      NULL,          // primary thread security attributes 
			      TRUE,          // handles are inherited 
			      0,             // creation flags 
			      NULL,          // use parent's environment
			      NULL,          // use parent's current directory 
			      &siStartInfo,  // STARTUPINFO pointer 
			      &piProcInfo);  // receives PROCESS_INFORMATION 
    free(szCmdline);
    
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    if (bFuncRetn == 0) 
    {
	fprintf(stderr, "CreateProcess failed\n");
	return TNC_RESULT_FATAL;
    }
    else
    {
	DWORD dwRead, dwWritten; 
	CHAR chBuf[BUFSIZE]; 

	// Get stdout from the child
	// Close the write end of the pipe before reading from the 
	// read end of the pipe. 
	if (!CloseHandle(hChildStdoutWr)) 
	{
	    fprintf(stderr, "Closing handle failed"); 
	    return TNC_RESULT_FATAL;
	}
	// Read output from the child process, and write to parent's STDOUT. 
	ReadFile(hChildStdoutRd, chBuf, BUFSIZE, &dwRead, NULL);
	
	// REVISIT:
	reply_size = snprintf(reply, MAX_MESSAGE_LEN, "%s|%d|%s", argstring, bFuncRetn, chBuf); 
	CloseHandle(hChildStdoutRd);

	// Send the reply back
	return sendMessage(imcID, connectionID, 
			   (TNC_BufferReference)reply, reply_size, 
			   OSC_MESSAGE_EXTCOMMAND_REPLY);
    }
#else
    // Cant do this on Unix
    return TNC_RESULT_FATAL;
#endif
}

///////////////////////////////////////////////////////////
// Diplay a message to the user by whatever means we can find
static TNC_Result userMessage(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ const char* message)
{
    const char* xmessage_argv[] = {"xmessage", "-center", "-timeout", "20", message, NULL};

    // First try to send it to our parent
    if (userMessageP)
	return (*userMessageP)(imcID, connectionID, message);
#if WIN32
    // What else can we do here
    fprintf(stderr, "OSC IMV user message: %s\n", message);
#else
    // Otherwise, try xmessage
    if (runProg(imcID, xmessage_argv[0], xmessage_argv, 0, NULL, 0) == 0)
	return TNC_RESULT_SUCCESS;
#endif
    // REVISIT: could try wall(1)?

    // Could not do anything with it!
    return TNC_RESULT_FATAL;
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
//    logMessage(imcID, TNC_LOG_SEVERITY_DEBUG, "initialize");
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
    char            msg[MAX_MESSAGE_LEN];

#ifdef WIN32
    OSVERSIONINFOEX osvi;

//    printf("TNC_IMC_BeginHandshake %d %d\n", imcID, connectionID);
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO*)&osvi);
    snprintf(msg, MAX_MESSAGE_LEN, "Windows|%d|%d|%d|%d|%s|%d|%d|%04x|%d", 
	      osvi.dwMajorVersion, 
	      osvi.dwMinorVersion,
	      osvi.dwBuildNumber,
	      osvi.dwPlatformId,
	      osvi.szCSDVersion,
	      osvi.wServicePackMajor,
	      osvi.wServicePackMinor,
	      osvi.wSuiteMask,
	      osvi.wProductType);
    // Report the OS type in message type 1
    return sendMessage(imcID, connectionID, (TNC_BufferReference)msg, strlen(msg), 
		       OSC_MESSAGE_OS_DETAILS);
#else
    struct utsname  buf;
    char*           lang = getenv("LANG");
    struct passwd  *pwd;

//    printf("TNC_IMC_BeginHandshake %d %d\n", imcID, connectionID);
    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (imcID != myId)
	return TNC_RESULT_INVALID_PARAMETER;

    // Get OS details using uname(2)
    if (uname(&buf) == -1)
    {
	logMessage(imcID, TNC_LOG_SEVERITY_ERR, 
		   "osc_imc TNC_IMC_BeginHandshake could not determine system type using uname(2): %s\n",
		   strerror(errno));
	return TNC_RESULT_FATAL;
    }

    // Get username of this process
    pwd = getpwuid(getuid());

    if (!lang) lang = "";
    snprintf(msg, MAX_MESSAGE_LEN, "%s|%s|%s|%s|%s|%s|%s", buf.sysname, buf.nodename, 
	     buf.release, buf.version, buf.machine, lang, 
	     pwd ? pwd->pw_name : "--unknown--");

    // Report the OS type in message type 1
    return sendMessage(imcID, connectionID, (TNC_BufferReference)msg, strlen(msg), 
		       OSC_MESSAGE_OS_DETAILS);
#endif
}

///////////////////////////////////////////////////////////
TNC_IMC_API TNC_Result TNC_IMC_ReceiveMessage(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference messageBuffer,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
    char buf[MAX_MESSAGE_LEN+1];

//    printf("TNC_IMC_ReceiveMessage %d %d %x %d %d\n", imcID, connectionID, messageBuffer, messageLength, messageType);
    if (!initialized)
	return TNC_RESULT_NOT_INITIALIZED;

    if (messageLength >= MAX_MESSAGE_LEN)
	return TNC_RESULT_INVALID_PARAMETER;

    // Make sure the msg is NUL terminated
    memcpy(buf, messageBuffer, messageLength);
    buf[messageLength] = '\0';

    if (messageType == OSC_MESSAGE_PACKAGE_STATUS_REQUEST)
    {
	// Package status request
	return checkPackage(imcID, connectionID, buf);
    }
    else if (messageType == OSC_MESSAGE_USER_MESSAGE)
    {
	// Show message to the user
	return userMessage(imcID, connectionID, buf);
    }
    else if (messageType == OSC_MESSAGE_FILE_STATUS_REQUEST)
    {
	// File status request
	return checkFile(imcID, connectionID, buf);
    }
    else if (messageType == OSC_MESSAGE_REGISTRY_REQUEST)
    {
	// File status request
	return checkRegistry(imcID, connectionID, buf);
    }
    else if (messageType == OSC_MESSAGE_EXTCOMMAND_REQUEST)
    {
	// File status request
	return externalCommand(imcID, connectionID, buf);
    }
    else
    {
	// Unknown message type. old/broken/rogue IMV ?
	return TNC_RESULT_INVALID_PARAMETER;
    }
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
//    printf("TNC_IMC_Terminate %d\n", imcID);
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
	// Look for a vendor extensions for logging and user messaging
	// Not an error if it does not exist
	(*bindFunction)(imcID, "TNC_9048_LogMessage", (void**)&logMessageP);
	(*bindFunction)(imcID, "TNC_9048_UserMessage", (void**)&userMessageP);

	// Look for required functions in the parent TMCC
	if ((*bindFunction)(imcID, "TNC_TNCC_ReportMessageTypes", 
			    (void**)&reportMessageTypesP) != TNC_RESULT_SUCCESS)
	    logMessage(imcID, TNC_LOG_SEVERITY_ERR, 
		       "Could not bind function for TNC_TNCC_ReportMessageTypes\n");
	if ((*bindFunction)(imcID, "TNC_TNCC_RequestHandshakeRetry", 
			    (void**)&requestHandshakeRetryP) != TNC_RESULT_SUCCESS)
	    logMessage(imcID, TNC_LOG_SEVERITY_ERR, 
		       "Could not bind function for TNC_TNCC_RequestHandshakeRetry\n");
	if ((*bindFunction)(imcID, "TNC_TNCC_SendMessage", 
			    (void**)&sendMessageP) != TNC_RESULT_SUCCESS)
	    logMessage(imcID, TNC_LOG_SEVERITY_ERR, 
		       "Could not bind function for TNC_TNCC_SendMessage\n");
    }

    reportMessageTypes(imcID, messageTypes, 
		       sizeof(messageTypes) / sizeof(TNC_MessageType));
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// End of publically visible functions
///////////////////////////////////////////////////////////

