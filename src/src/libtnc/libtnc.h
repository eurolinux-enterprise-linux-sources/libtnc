// libtnc.h
//
// Utility routines header for IMC and IMV routines
// Provides function to load IMCs, and call their public functions
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtnc.h,v 1.11 2009/03/18 11:28:35 mikem Exp mikem $

#ifndef _LIBTNC_H
#define _LIBTNC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <tncif.h>

#if WITH_DMALLOC
#include <dmalloc.h>
#endif

// Construct a message type from vendor ID and message type number
// High 24 bits are the vendor ID, log 8 bits the message number
// 0xffffffnn is reserved for messages from any vendor
// 0xnnnnnnff is reserved for any message from vendor nnnnnn
// 0xffffffff is reserved for any message from any vendor
// Vendor ID 0 is reserved for TCG
#define TNCMESSAGENUM(v,n) ((v<<8)|n)

// This is the TNC defined default location of the TNC config file
#define TNC_CONFIG_FILE_PATH "/etc/tnc_config"

// These define the names of the well known Registry keys for finding
// IMCs and IMVs on Windows
#define TNC_CONFIG_REGISTRY_KEY_IMC "SOFTWARE\\Trusted Computing Group\\TNC\\IMCs\\"
#define TNC_CONFIG_REGISTRY_KEY_IMV "SOFTWARE\\Trusted Computing Group\\TNC\\IMVs\\"
#define TNC_MAX_REGISTRY_NAME_LEN 255
#define TNC_MAX_REGISTRY_VALUE_LEN 1024

// Log message severity levels, consistent with syslog and others
#define TNC_LOG_SEVERITY_ERR         0
#define TNC_LOG_SEVERITY_WARNING     1
#define TNC_LOG_SEVERITY_NOTICE      2
#define TNC_LOG_SEVERITY_INFO        3
#define TNC_LOG_SEVERITY_DEBUG       4

// Common logging functions
// You can override this to provide an application specific logging function
extern TNC_Result libtnc_logMessage(
    /*in*/ TNC_UInt32 severity,
    /*in*/ const char* format, ...);

// For IMCs and IMVs to call
typedef TNC_Result (*TNC_9048_LogMessagePointer)(
    TNC_UInt32 severity,
    const char * message);


#ifdef __cplusplus
}
#endif
#endif
