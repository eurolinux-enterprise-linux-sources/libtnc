// libtncxml.h
//
// Implment XML utility routines for TNCC and TNCS modules of
// of TNC
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtncxml.c,v 1.3 2009/03/18 11:28:35 mikem Exp mikem $
//
// Changes 2010/10/27, Jan Stalhut, Decoit GmbH:
// Added TNCCS-TNCSContactInfo to inform the TNCC of the TNCS's IP and port
// as defined in IF-TNCCS v1.2, Revision 6.00, section 2.8.5.5

#include <libtncconfig.h>
#include <tncifimv.h>
#include <libtncxml.h>
#include <libtncdebug.h>
#include <base64.h>

///////////////////////////////////////////////////////////
// debug dump doc details to stdout
void libtncxml_docDump(xmlDocPtr doc)
{
    xmlChar*   xmlbuff;
    int        buffersize;

    xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
    printf("Doc Dump: %s\n", (char *)xmlbuff);
    xmlFree(xmlbuff);
}

///////////////////////////////////////////////////////////
// debug dump node details to stdout
void libtncxml_nodeDump(xmlDocPtr doc, xmlNodePtr cur)
{
    xmlBuffer* buf=xmlBufferCreate();
    int        n = xmlNodeDump(buf,doc,cur,0,1);

    printf("Node Dump: %s\n", xmlBufferContent(buf));
    xmlBufferFree(buf);
}

#ifdef WIN32
#define snprintf _snprintf      // Needed to fix MSVC brain deadness.
#endif

///////////////////////////////////////////////////////////
// Create a new XML doc ready to add nodes
xmlDocPtr libtncxml_new(
    /*in*/ int batchid,
    /*in*/ const char* recipient)
{
    xmlNodePtr n;
    // Create a new document with all the required infrastructure
    xmlDocPtr  doc = xmlNewDoc(BAD_CAST "1.0");
    char       buf[12];

    n = xmlNewNode(NULL, BAD_CAST "TNCCS-Batch");
    snprintf(buf, 12, "%d", batchid);
    xmlNewProp(n, BAD_CAST "BatchId", BAD_CAST buf);
    xmlNewProp(n, BAD_CAST "Recipient", BAD_CAST recipient);
    xmlNewProp(n, BAD_CAST "xmlns", BAD_CAST "http://www.trustedcomputinggroup.org/IWG/TNC/1_0/IF_TNCCS#");
    xmlNewProp(n, BAD_CAST "xmlns:xsi", BAD_CAST "http://www.w3.org/2001/XMLSchema-instance");
    xmlNewProp(n, BAD_CAST "xsi:schemaLocation", BAD_CAST "http://www.trustedcomputinggroup.org/IWG/TNC/1_0/IF_TNCCS# https://www.trustedcomputinggroup.org/XML/SCHEMA/TNCCS_1.0.xsd");
    xmlDocSetRootElement(doc, n);

    return doc;
}

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-Error to an
// XML TNCCS-Batch document
void libtncxml_add_tnccs_error(
    /*in*/ xmlDocPtr  doc,
    /*in*/ const char* type,
    /*in*/ const char* message)
{
    xmlNodePtr root= xmlDocGetRootElement(doc);
    xmlNodePtr messagenode;
    xmlNodePtr n, n2;

    DEBUG3("libtncxml_add_tnccs_error %s %s", type, message);
    // Create a new TNCC-TNCS-Message node
    messagenode = xmlNewNode(NULL, BAD_CAST "TNCC-TNCS-Message");
    xmlAddChild(root, messagenode);

    // Add the message type number
    n = xmlNewNode(NULL, BAD_CAST "Type");
    xmlNodeSetContent(n, BAD_CAST "00000002");
    xmlAddChild(messagenode, n);

    n = xmlNewNode(NULL, BAD_CAST "XML");
    xmlAddChild(messagenode, n);

    n2 = xmlNewNode(NULL, BAD_CAST "TNCCS-Error");
    xmlNewProp(n2, BAD_CAST "type", BAD_CAST type);
    xmlNodeSetContent(n2, BAD_CAST message);
    xmlAddChild(n, n2);
}

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-PreferredLanguage to an
// XML TNCCS-Batch document
void libtncxml_add_tnccs_preferred_language(
    /*in*/ xmlDocPtr  doc,
    /*in*/ const char* language)
{
    xmlNodePtr root= xmlDocGetRootElement(doc);
    xmlNodePtr messagenode;
    xmlNodePtr n, n2;

    DEBUG2("libtncxml_add_tnccs_preferred_language %s", language);
    // Create a new TNCC-TNCS-Message node
    messagenode = xmlNewNode(NULL, BAD_CAST "TNCC-TNCS-Message");
    xmlAddChild(root, messagenode);

    // Add the message type number
    n = xmlNewNode(NULL, BAD_CAST "Type");
    xmlNodeSetContent(n, BAD_CAST "00000003");
    xmlAddChild(messagenode, n);

    n = xmlNewNode(NULL, BAD_CAST "XML");
    xmlAddChild(messagenode, n);

    n2 = xmlNewNode(NULL, BAD_CAST "TNCCS-PreferredLanguage");
    xmlNodeSetContent(n2, BAD_CAST language);
    xmlAddChild(n, n2);
}

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-ReasonString to an
// XML TNCCS-Batch document
void libtncxml_add_tnccs_reason(
    /*in*/ xmlDocPtr  doc,
    /*in*/ const char* language,
    /*in*/ const char* reason)
{
    xmlNodePtr root= xmlDocGetRootElement(doc);
    xmlNodePtr messagenode;
    xmlNodePtr n, n2, n3;

    DEBUG3("libtncxml_add_tnccs_error %s %s", language, reason);
    // Create a new TNCC-TNCS-Message node
    messagenode = xmlNewNode(NULL, BAD_CAST "TNCC-TNCS-Message");
    xmlAddChild(root, messagenode);

    // Add the message type number
    n = xmlNewNode(NULL, BAD_CAST "Type");
    xmlNodeSetContent(n, BAD_CAST "00000004");
    xmlAddChild(messagenode, n);

    n = xmlNewNode(NULL, BAD_CAST "XML");
    xmlAddChild(messagenode, n);

    n2 = xmlNewNode(NULL, BAD_CAST "TNCCS-ReasonStrings");

    // Could add multiple reasons here, if we had them
    n3 = xmlNewNode(NULL, BAD_CAST "ReasonString");
    xmlNewProp(n3, BAD_CAST "xml:lang", BAD_CAST language);
    xmlNodeSetContent(n3, BAD_CAST reason);
    xmlAddChild(n2, n3);

    xmlAddChild(n, n2);
}

////////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-TNCSContactInfo to an
// XML TNCCS-Batch document
void libtncxml_add_tnccs_tncscontactinfo(
    /*in*/ xmlDocPtr  doc,
    /*in*/ const char* address,
    /*in*/ const char* port)
{
    xmlNodePtr root= xmlDocGetRootElement(doc);
    xmlNodePtr messagenode;
    xmlNodePtr n, n2, n3;

    DEBUG3("libtncxml_add_tnccs_error %s %s", address, port);
    // Create a new TNCC-TNCS-Message node
    messagenode = xmlNewNode(NULL, BAD_CAST "TNCC-TNCS-Message");
    xmlAddChild(root, messagenode);

    // Add the message type number
    n = xmlNewNode(NULL, BAD_CAST "Type");
    xmlNodeSetContent(n, BAD_CAST "00000005");
    xmlAddChild(messagenode, n);

    n = xmlNewNode(NULL, BAD_CAST "XML");
    xmlAddChild(messagenode, n);

    n2 = xmlNewNode(NULL, BAD_CAST "TNCCS-TNCSContactInfo");

    // Could add multiple reasons here, if we had them
    n3 = xmlNewNode(NULL, BAD_CAST "ReasonString");
    xmlNewProp(n3, BAD_CAST "address", BAD_CAST address);
	xmlNewProp(n3, BAD_CAST "port", BAD_CAST port);
    xmlAddChild(n2, n3);

    xmlAddChild(n, n2);
}

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-Recommendation to an
// XML TNCCS-Batch document
void libtncxml_add_tnccs_recommendation(
    /*in*/ xmlDocPtr  doc,
    /*in*/ TNC_IMV_Action_Recommendation recommendation)
{
    xmlNodePtr root= xmlDocGetRootElement(doc);
    xmlNodePtr messagenode;
    xmlNodePtr n, n2;
    char* recommendation_string;

    DEBUG2("libtncxml_add_tnccs_recommendation %d", recommendation);
    // Create a new TNCC-TNCS-Message node
    messagenode = xmlNewNode(NULL, BAD_CAST "TNCC-TNCS-Message");
    xmlAddChild(root, messagenode);

    // Add the message type number
    n = xmlNewNode(NULL, BAD_CAST "Type");
    xmlNodeSetContent(n, BAD_CAST "00000001");
    xmlAddChild(messagenode, n);

    n = xmlNewNode(NULL, BAD_CAST "XML");
    xmlAddChild(messagenode, n);

    switch (recommendation)
    {
    case TNC_IMV_ACTION_RECOMMENDATION_ALLOW:
	recommendation_string = "allow";
	break;

    case TNC_IMV_ACTION_RECOMMENDATION_NO_ACCESS:
    case TNC_IMV_ACTION_RECOMMENDATION_NO_RECOMMENDATION:
	recommendation_string = "none";
	break;
	
    case TNC_IMV_ACTION_RECOMMENDATION_ISOLATE:
	recommendation_string = "isolate";
	break;
	
    default:
	// Should not happen. This string is not defined in the DTD
	recommendation_string = "invalid recommendation";
	break;
    }
	
    n2 = xmlNewNode(NULL, BAD_CAST "TNCCS-Recommendation");
    xmlNewProp(n2, BAD_CAST "type", BAD_CAST recommendation_string);
    xmlNodeSetContent(n2, BAD_CAST "");
    xmlAddChild(n, n2);
}

///////////////////////////////////////////////////////////
// Add an IMC-IMV-Message to the XML TNCCS-Batch document.
void libtncxml_add_imc_imv_message(
    /*in*/ xmlDocPtr  doc,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
    xmlNodePtr messagenode;
    xmlNodePtr root, n;
    char*      data;
    char       buf[10]; // big enough for a hex string

    root = xmlDocGetRootElement(doc);

    // Create a new IMC-IMV-Message node
    messagenode = xmlNewNode(NULL, BAD_CAST "IMC-IMV-Message");
    xmlAddChild(root, messagenode);

    // Add the message type number in hex
    n = xmlNewNode(NULL, BAD_CAST "Type");
    snprintf(buf, 10, "%08x", messageType);
    xmlNodeSetContent(n, BAD_CAST buf);
    xmlAddChild(messagenode, n);

    // Encode the message as a Base64 node
    n = xmlNewNode(NULL, BAD_CAST "Base64");
    encode_base64(message, messageLength, &data);
    xmlNodeSetContent(n, BAD_CAST data);
    xmlAddChild(messagenode, n);
    free(data);
}


