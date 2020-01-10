// libtnctncc.h
//
// Implement the TNCC layer of IF-TNCCS, part
// of TNC, inlcuding XML based batch packing and unpacking
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtnctncc.h,v 1.4 2007/05/05 02:13:11 mikem Exp mikem $
#ifndef _LIBTNCTNCC_H
#define _LIBTNCTNCC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <libxml/parser.h>
#include <libtncimc.h>

// This structure holds context and cobbection data for each current connection
typedef struct
{
    TNC_ConnectionID connectionID; // Also the index into the session dynarray
    void*            appData;      // Application data
    xmlDocPtr        cur_doc;      // XML Document currently under construction
} libtnc_tncc_connection;

// Initialize the TNCC layer using the standard config place for the platform
// Call once before any other TNCC functions
    extern TNC_Result libtnc_tncc_InitializeStd();

// Initialize the TNCC layer using a named config file
// Call once before any other TNCC functions
// Use libtnc_tncc_InitializeStd in production code
    extern TNC_Result libtnc_tncc_Initialize(
    /*in*/const char* filename);

// Set the clients preferred language
TNC_Result libtnc_tncc_PreferredLanguage(
    /*in*/const char* language);

// Terminate the TNCC layer
extern TNC_Result libtnc_tncc_Terminate();

// Create a new connection context
// data is optional application data for use by the caller
// return NULL on failure
extern libtnc_tncc_connection* libtnc_tncc_CreateConnection(
    /*in*/void* appData);

// Return connection context application data
void* libtnc_tncc_ConnectionAppData(
    /*in*/libtnc_tncc_connection* conn);

// Delete a connection context
// return its connection id
TNC_ConnectionID libtnc_tncc_DeleteConnection(
    /*in*/libtnc_tncc_connection* conn);

// Start a new handshaking session
extern TNC_Result libtnc_tncc_BeginSession(
    /*in*/ libtnc_tncc_connection* conn);

// Deliver an XML encapsulated batch of messages to TNCC from the TNCS
extern TNC_Result libtnc_tncc_ReceiveBatch(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ const char*  messageBuffer,
    /*in*/ size_t messageLength);

// Send an XML encapsulated batch of messages from the TNCC towards the TNCS
// Caller is expected to implement this
extern TNC_Result TNC_TNCC_SendBatch(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ const char* messageBuffer,
    /*in*/ size_t messageLength);

// Handle a various TNCC-TNCS messages
// Caller is expected to implement these if interested in them

// Handle a vendor-specific TNCC-TNCS message
extern TNC_Result TNC_TNCC_ReceiveMessage(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ unsigned char* message, 
    /*in*/ size_t messagelen);
// Handle a vendor-specific TNCC-TNCS XML encapsulated message
extern TNC_Result TNC_TNCC_ReceiveMessageXML(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ xmlDocPtr doc, 
    /*in*/ xmlNodePtr cur, 
    /*in*/ xmlNsPtr ns);

#ifdef __cplusplus
}
#endif
#endif
