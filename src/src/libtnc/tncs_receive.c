// tncs_receive.c
//
// Default implementations of TNC_TNCS_ReceiveRecommendation, TNCS_ReceiveMessage*
// which will be linked if the caller does not wish to handle
// TNCC-TNCS messages.
// If you need to handle TNCC-TNCS messages, 
// provide your own implementation 
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: tncs_receive.c,v 1.2 2007/05/06 01:06:02 mikem Exp $

#include <libtncconfig.h>
#include <libxml/parser.h>
#include <libtnc.h>

// Handle a vendor-specific TNCC-TNCS message
TNC_Result TNC_TNCS_ReceiveMessage(
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ unsigned char* message, 
    /*in*/ int messagelen)
{
    libtnc_logMessage(TNC_LOG_SEVERITY_INFO, "Vendor-specific TNC_TNCS_ReceiveMessage not implemented to handle TNCC-TNCS-Message type %x\n", messageType);
    return TNC_RESULT_SUCCESS;
}

// Handle a vendor-specific TNCC-TNCS message XML encapsulated message
TNC_Result TNC_TNCS_ReceiveMessageXML(
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ xmlDocPtr doc, 
    /*in*/ xmlNodePtr cur, 
    /*in*/ xmlNsPtr ns)
{
    libtnc_logMessage(TNC_LOG_SEVERITY_INFO, "Vendor-specific TNC_TNCS_ReceiveMessageXML not implemented to handle TNCC-TNCS-Message type %x\n", messageType);
    return TNC_RESULT_SUCCESS;
}
