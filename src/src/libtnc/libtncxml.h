// libtncxml.h
//
// Implement the TNCC layer of IF-TNCCS, part
// of TNC
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtncxml.h,v 1.2 2007/02/13 08:09:20 mikem Exp mikem $
// 
// Changes 2010/10/27, Jan Stalhut, Decoit GmbH:
// Added TNCCS-TNCSContactInfo to inform the TNCC of the TNCS's IP and port
// as defined in IF-TNCCS v1.2, Revision 6.00, section 2.8.5.5

#ifndef _LIBTNCXML_H
#define _LIBTNCXML_H
#ifdef __cplusplus
extern "C" {
#endif

#include <tncifimv.h>
#include <libxml/parser.h>

// debug dump doc details to stdout
extern void libtncxml_docDump(xmlDocPtr doc);

// debug dump node details to stdout
extern void libtncxml_nodeDump(xmlDocPtr doc, xmlNodePtr cur);

// Create a new XML doc ready to add nodes
extern xmlDocPtr libtncxml_new(
    /*in*/ int batchid,
    /*in*/ const char* recipient);

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-Error to an
// XML TNCCS-Batch document
extern void libtncxml_add_tnccs_error(
    /*in*/ xmlDocPtr  doc,
    /*in*/ const char* type,
    /*in*/ const char* message);

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-PreferredLanguage to an
// XML TNCCS-Batch document
extern void libtncxml_add_tnccs_preferred_language(
    /*in*/ xmlDocPtr  doc,
    /*in*/ const char* language);

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-ReasonString to an
// XML TNCCS-Batch document
void libtncxml_add_tnccs_reason(
    /*in*/ xmlDocPtr  doc,
    /*in*/ const char* language,
    /*in*/ const char* reason);

////////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-TNCSContactInfo to an
// XML TNCCS-Batch document
void libtncxml_add_tnccs_tncscontactinfo(
    /*in*/ xmlDocPtr  doc,
    /*in*/ const char* address,
    /*in*/ const char* port);

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-Recommendation to an
// XML TNCCS-Batch document
extern void libtncxml_add_tnccs_recommendation(
    /*in*/ xmlDocPtr  doc,
    /*in*/ TNC_IMV_Action_Recommendation recommendation);

// Add an IMC-IMV-Message to the XML TNCCS-Batch document.
extern void libtncxml_add_imc_imv_message(
    /*in*/ xmlDocPtr  doc,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType);




#ifdef __cplusplus
}
#endif
#endif
