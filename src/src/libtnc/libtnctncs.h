// libtnctncs.h
//
// Implement the TNCS layer of IF-TNCSS, part
// of TNC, inlcuding XML based batch packing and unpacking
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtnctncs.h,v 1.5 2009/03/18 11:28:35 mikem Exp mikem $
#ifndef _LIBTNCTNCS_H
#define _LIBTNCTNCS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <libxml/parser.h>
#include <libtncimv.h>

// This structure holds context and cobbection data for each current connection
typedef struct
{
    TNC_ConnectionID              connectionID; // Also the index into the session dynarray
    void*                         appData;      // Application data
    xmlDocPtr                     cur_doc;      // XML Document currently under construction
    TNC_UInt32                    recommendationPolicy;
    TNC_UInt32                    have_recommendation;
    TNC_IMV_Action_Recommendation recommendation;
    TNC_IMV_Evaluation_Result     evaluation;
    TNC_UInt32                    imv_imc_count; // Hom many messages to go to the IMC
    TNC_UInt32                    have_recommendations[NUM_IMVS];
    TNC_IMV_Action_Recommendation recommendations[NUM_IMVS];
    
} libtnc_tncs_connection;

// Recommendation policies
// With LIBTNC_TNCS_RECOMMENDATION_POLICY_ALL:
//  if all IMVs recommend ALLOW the result is ALLOW
//  else if all IMVs recommend  ISOLATE the result is ISOLATE
//  else the result is NO_ACCESS
// With LIBTNC_TNCS_RECOMMENDATION_POLICY_ANY
//  if any IMVs recommend ALLOW the result is ALLOW
//  else if any IMVs recommend ISOLATE the result is ISOLATE
//  else the result is NO_ACCESS
#define LIBTNC_TNCS_RECOMMENDATION_POLICY_ALL 1
#define LIBTNC_TNCS_RECOMMENDATION_POLICY_ANY 2
extern int libtnc_tncs_SetRecommendationPolicy(
    /*in*/libtnc_tncs_connection* conn,
    /*in*/int policy);

// Initialize the TNCS layer using the standard config place for the platform
// Call once before any other TNCS functions
extern TNC_Result libtnc_tncs_InitializeStd();

// Initialize the TNCS layer using a named config file
// Call once before any other TNCS functions
// Use libtnc_tncs_Initialize in production code
extern TNC_Result libtnc_tncs_Initialize(
    /*in*/const char* filename);

// Terminate the TNCS layer
extern TNC_Result libtnc_tncs_Terminate();

// Create a new connection context
// data is optional application data for use by the caller
// return NULL on failure
extern libtnc_tncs_connection* libtnc_tncs_CreateConnection(
    /*in*/void* appData);

// Return connection context application data
void* libtnc_tncs_ConnectionAppData(
    /*in*/libtnc_tncs_connection* conn);

// Delete a connection context
// return its connection id
TNC_ConnectionID libtnc_tncs_DeleteConnection(
    /*in*/libtnc_tncs_connection* conn);

// Start a new handshaking session
extern TNC_Result libtnc_tncs_BeginSession(
    /*in*/ libtnc_tncs_connection* conn);

// Deliver an XML encapsulated batch of messages to TNCS from the TNCC
extern TNC_Result libtnc_tncs_ReceiveBatch(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ const char*  messageBuffer,
    /*in*/ size_t messageLength);

// Return TNC_RESULT_SUCCESS if the IMV has made a recommendation
// Also fill in the ponted to data with the recommendation if the pointer is not NULL
extern TNC_Result libtnc_tncs_HaveRecommendation(
	/*in*/ libtnc_tncs_connection* conn,
	/*out*/ TNC_IMV_Action_Recommendation* recommendation,
	/*out*/ TNC_IMV_Evaluation_Result* evaluation);


// Send an XML encapsulated batch of messages from the TNCS towards the TNCC
// Caller is expected to implement this
extern TNC_Result TNC_TNCS_SendBatch(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ const char* messageBuffer,
    /*in*/ size_t messageLength);

// Handle a various TNCS-TNCS messages
// Caller is expected to implement these if interested in them

extern TNC_Result TNC_TNCS_ReceiveMessage(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ unsigned char* message, 
    /*in*/ size_t messagelen);
// Handle a vendor-specific TNCC-TNCS XML encapsulated message
extern TNC_Result TNC_TNCS_ReceiveMessageXML(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ xmlDocPtr doc, 
    /*in*/ xmlNodePtr cur, 
    /*in*/ xmlNsPtr ns);

#ifdef __cplusplus
}
#endif
#endif
