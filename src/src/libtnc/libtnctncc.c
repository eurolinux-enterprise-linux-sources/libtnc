// libtnctncc.h
//
// Implement the TNCC layer of IF-TNCCS, part
// of TNC, including XML based batch packing and unpacking
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtnctncc.c,v 1.4 2009/03/18 11:28:35 mikem Exp mikem $
//
// Changes 2010/10/27, Jan Stalhut, Decoit GmbH:
// Added TNCCS-TNCSContactInfo to inform the TNCC of the TNCS's IP and port
// as defined in IF-TNCCS v1.2, Revision 6.00, section 2.8.5.5

#include <libtncconfig.h>
#include <libtnctncc.h>
#include <libtncxml.h>
#include <libtncdebug.h>
#include <libtncarray.h>

// The clients preferred language (NULL if not set)
static const char* preferredLanguage = NULL;

// Dynamic array of libtnc_tncc_connection indexed by connectionID
// Protected by mutex
// Need this because the lower layers only understand connection IDs, 
// and make calls into us, passing the connection ID. we need to be able to 
// resolve this to a connection context.
static libtnc_array connections = LIBTNC_ARRAY_INIT;

///////////////////////////////////////////////////////////
// Initialise data to start a new batch of messages
// Later we will maybe add messages when the IMC layer calls into us
static
xmlDocPtr libtnc_tncc_StartBatch(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ int batchid)
{
    DEBUG2("libtnc_tncc_StartBatch %d", batchid);
    return libtncxml_new(batchid, "TNCS");
}

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCCS-Error to the 
// XML TNCCS-Batch document currently
// under construction
static
TNC_Result libtnc_tncc_add_tnccs_error(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ const char* type,
    /*in*/ const char* message)
{
    DEBUG3("libtnc_tncc_add_tnccs_error %s %s", type, message);
    libtncxml_add_tnccs_error(conn->cur_doc, type, message);
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Add an IMC-IMV-Message to the XML TNCCS-Batch document currently
// under construction
static
TNC_Result libtnc_tncc_add_imc_imv_message(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)

{
    DEBUG1("libtnc_tncc_add_imc_imv_message");
    libtncxml_add_imc_imv_message(conn->cur_doc, message, messageLength, messageType);
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
static
TNC_Result libtnc_tncc_EndBatch(
    /*in*/ libtnc_tncc_connection* conn)
{
    xmlChar*   xmlbuff;
    size_t     buffersize;
    TNC_Result ret;

    DEBUG2("libtnc_tncc_EndBatch %d", conn->connectionID);
    // Call back into the caller with the finished buffer to send
    xmlDocDumpFormatMemory(conn->cur_doc, &xmlbuff, (int*)&buffersize, 1);
    ret = TNC_TNCC_SendBatch(conn, (char *)xmlbuff, buffersize);
    xmlFree(xmlbuff);
    xmlFreeDoc(conn->cur_doc);
    conn->cur_doc = NULL;
    return ret;
}

///////////////////////////////////////////////////////////
// Called by the IMC layer
TNC_Result TNC_TNCC_RequestHandshakeRetry(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_RetryReason reason)
{
    DEBUG4("TNC_TNCC_RequestHandshakeRetry %d %d %d", imcID, connectionID, reason);

    // REVISIT
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Called by the IMC layer when an IMC-IMV message is to be
// sent. Add it to the current batch
TNC_Result TNC_TNCC_SendMessage(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
    libtnc_tncc_connection* conn;

    DEBUG6("TNC_TNCC_SendMessage %d %d %s %d %x", imcID, connectionID, message, messageLength, messageType);

    libtnc_mutex_lock();
    conn = libtnc_array_index(&connections, connectionID);
    libtnc_mutex_unlock();
    return libtnc_tncc_add_imc_imv_message(conn, message, messageLength, messageType);
}

///////////////////////////////////////////////////////////
// Handle a TNCC-TNCS-Message received from TNCS
TNC_Result libtnc_tncc_ReceiveMessage(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ unsigned char* message, 
    /*in*/ size_t messagelen)
{
    TNC_VendorID       mv = (messageType >> 8) & TNC_VENDORID_ANY;

    DEBUG3("libtnc_tncc_ReceiveMessage %d %x", conn->connectionID, messageType);
    // Vendor specific?
    if (mv != 0)
	return TNC_TNCC_ReceiveMessage(conn, messageType, message, messagelen);

    // There are no base64 encoded TNCC-TNCS message. All the ones defined
    // by TNC use XML, which will go to libtnc_tncc_ReceiveMessageXML
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Handle a TNCC-TNCS-Message encoded in an XML node received from TNCS
TNC_Result libtnc_tncc_ReceiveMessageXML(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ xmlDocPtr  doc,
    /*in*/ xmlNodePtr cur,
    /*in*/ xmlNsPtr   ns)
{
    TNC_VendorID       mv = (messageType >> 8) & TNC_VENDORID_ANY;

    DEBUG2("libtnc_tncc_ReceiveMessageXML %x", messageType);
    // Skip empty and blank nodes
    while (cur && xmlIsBlankNode(cur))
	cur = cur->next;
    if (!cur)
	return TNC_RESULT_FATAL; // Nothing there?

#ifdef LIBTNCDEBUG
    libtncxml_nodeDump(doc, cur);
#endif
    // Vendor specific?
    if (mv != 0)
	return TNC_TNCC_ReceiveMessageXML(conn, messageType, doc, cur, ns);

    if (!xmlStrcmp(cur->name, (const xmlChar *) "TNCCS-Recommendation") 
	&& cur->ns == ns)
    {
	// should be type 1
	xmlChar* rtype = xmlGetProp(cur, (const xmlChar *)"type");
	xmlChar* content = xmlNodeGetContent(cur);

	// Expect 'allow', 'none' or 'isolate'
	if (!xmlStrcasecmp(rtype, (const xmlChar *) "allow"))
	    libtnc_imc_NotifyConnectionChange(conn->connectionID, 
					      TNC_CONNECTION_STATE_ACCESS_ALLOWED);
	else if (!xmlStrcasecmp(rtype, (const xmlChar *) "none"))
	    libtnc_imc_NotifyConnectionChange(conn->connectionID, 
					      TNC_CONNECTION_STATE_ACCESS_NONE);
	else if (!xmlStrcasecmp(rtype, (const xmlChar *) "isolate"))
	    libtnc_imc_NotifyConnectionChange(conn->connectionID, 
					      TNC_CONNECTION_STATE_ACCESS_ISOLATED);
	libtnc_imc_NotifyConnectionChange(conn->connectionID, TNC_CONNECTION_STATE_DELETE);

	// This is the end of this handshake
	// Make sure we dont send the current batch
	xmlFreeDoc(conn->cur_doc);
	conn->cur_doc = NULL;

	xmlFree(rtype);
	xmlFree(content);
    }
    else if (!xmlStrcmp(cur->name, (const xmlChar *) "TNCCS-Error") 
	     && cur->ns == ns)
    {
	// should be type 2
	xmlChar* rtype = xmlGetProp(cur, (const xmlChar *)"type");
	xmlChar* content = xmlNodeGetContent(cur);

	// Expect 'batch-too-long', 'malformed-batch' etc
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "TNCCS-Error %s: %s\n", rtype, content);
	xmlFree(rtype);
	xmlFree(content);
    }
    else if (!xmlStrcmp(cur->name, (const xmlChar *) "TNCCS-ReasonStrings") 
	     && cur->ns == ns)
    {
	xmlNodePtr       mcur;

	// should be type 4
	// Expect 0 or more ReasonString nodes inside
	mcur = cur->xmlChildrenNode;
	while (mcur != NULL)
	{
	    if (!xmlStrcmp(mcur->name, (const xmlChar *) "ReasonString") 
		&& mcur->ns == ns)
	    {
		xmlChar* rtype = xmlGetProp(mcur, (const xmlChar *)"lang");
		xmlChar* content = xmlNodeGetContent(mcur);

		// REVISIT: do more with this?
		libtnc_logMessage(TNC_LOG_SEVERITY_INFO, "TNCCS-ReasonString %s: %s\n", rtype, content);
		xmlFree(rtype);
		xmlFree(content);
	    }
	    mcur = mcur->next;
	}
    }
    else if (!xmlStrcmp(cur->name, (const xmlChar *) "TNCCS-TNCSContactInfo") 
	&& cur->ns == ns)
    {
		xmlNodePtr       mcur;

		// should be type 5
		xmlChar* address = xmlGetProp(cur, (const xmlChar *)"address");
		xmlChar* port = xmlGetProp(cur, (const xmlChar *)"port");

		// REVISIT: do more with this?
		libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "TNCCS-TNCSContactInfo: %s, Port: %s\n", address, port);
		xmlFree(address);
		xmlFree(port);
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Initialise the TNCC layer
TNC_Result libtnc_tncc_Initialize(
    /*in*/const char* filename)
{
    DEBUG2("libtnc_tncc_Initialize %s", filename);
    if (libtnc_imc_load_config(filename) < 0)
    {
	// Problem loading
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncc_Initialize error: failed to load IMC configuration\n");
	return TNC_RESULT_FATAL;
    }

    // Success
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Initialise the TNCC layer
TNC_Result libtnc_tncc_InitializeStd()
{
    DEBUG1("libtnc_tncc_InitializeStd");
    if (libtnc_imc_load_std_config() < 0)
    {
	// Problem loading
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncc_InitializeStd error: failed to load IMC configuration\n");
	return TNC_RESULT_FATAL;
    }

    // Success
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Set the clients preferred language
TNC_Result libtnc_tncc_PreferredLanguage(
    /*in*/const char* language)
{
    DEBUG2("libtnc_tncc_PreferredLanguage %s", language);

    preferredLanguage = language; // Should alloc and copy?
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Terminate the TNCC layer
TNC_Result libtnc_tncc_Terminate()
{
    DEBUG1("libtnc_tncc_Terminate");
    TNC_UInt32 i;

    for(i = 0; i < connections.max; i++)
    {
	free(connections.data[i]);
        connections.data[i] = NULL;
    }

    libtnc_array_free(&connections);
    return libtnc_imc_Terminate();
}

///////////////////////////////////////////////////////////
// Create a new connection context
libtnc_tncc_connection* libtnc_tncc_CreateConnection(void* appData)
{
    libtnc_tncc_connection* ret;

    if ((ret = (libtnc_tncc_connection*)calloc
	 (1, sizeof(libtnc_tncc_connection))) != NULL)
    {
	// Lock the connections array
	libtnc_mutex_lock();
	ret->connectionID = libtnc_array_new(&connections, ret);
	libtnc_mutex_unlock();
	ret->appData = appData;
    }
    return ret;
}

///////////////////////////////////////////////////////////
// Return connection context application data
void* libtnc_tncc_ConnectionAppData(libtnc_tncc_connection* conn)
{
    return conn->appData;
}

///////////////////////////////////////////////////////////
// Delete a connection context
// return its connection id
TNC_ConnectionID libtnc_tncc_DeleteConnection(libtnc_tncc_connection* conn)
{
    TNC_ConnectionID connectionID = 0;
    // Lock the connections array
    libtnc_mutex_lock();
    connectionID = conn->connectionID;
    libtnc_imc_NotifyConnectionChange(conn->connectionID, TNC_CONNECTION_STATE_DELETE);
    libtnc_array_delete(&connections, conn->connectionID);
    libtnc_mutex_unlock();
    xmlFreeDoc(conn->cur_doc);
    free(conn);

    return connectionID;
}

///////////////////////////////////////////////////////////
// Start a new handshaking session
// May result in the IMC sending some messages into the first batch
TNC_Result libtnc_tncc_BeginSession(
    /*in*/ libtnc_tncc_connection* conn)
{
    DEBUG1("libtnc_tncc_BeginSession");

    conn->cur_doc = libtnc_tncc_StartBatch(conn, 1); // First batch is always id 1
    if (preferredLanguage)
	libtncxml_add_tnccs_preferred_language(conn->cur_doc, preferredLanguage);
    libtnc_imc_NotifyConnectionChange(conn->connectionID, TNC_CONNECTION_STATE_CREATE);
    libtnc_imc_NotifyConnectionChange(conn->connectionID, TNC_CONNECTION_STATE_HANDSHAKE);
    libtnc_imc_BeginHandshake(conn->connectionID);
    return libtnc_tncc_EndBatch(conn);
}

///////////////////////////////////////////////////////////
// Deliver an XML encapsulated batch of messages to TNCC from the TNCS
// Some messages may be TNCC-TNCS messages, and some may be IMC-IMV messages
TNC_Result libtnc_tncc_ReceiveBatch(
    /*in*/ libtnc_tncc_connection* conn,
    /*in*/ const char*  messageBuffer,
    /*in*/ size_t messageLength)
{
    xmlDocPtr        doc;
    xmlNodePtr       cur;
    xmlNsPtr         ns;
    xmlChar*         batchid;
    xmlChar*         recipient;
    int              imv_imc_count = 0;

    DEBUG2("libtnc_tncc_ReceiveBatch %d", conn->connectionID);
    if ((doc = xmlParseMemory(messageBuffer, messageLength)) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncc_ReceiveBatch error: failed to XML parse message\n");
	return TNC_RESULT_FATAL;
    }

#ifdef LIBTNCDEBUG
    libtncxml_docDump(doc);
#endif

    // Check out the document
    if ((cur = xmlDocGetRootElement(doc)) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncc_ReceiveBatch error: empty document\n");
	return TNC_RESULT_FATAL;
    }

    if ((ns = xmlSearchNsByHref
	 (doc, cur, (const xmlChar *) "http://www.trustedcomputinggroup.org/IWG/TNC/1_0/IF_TNCCS#")) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncc_ReceiveBatch error: TNCCS namespace not found\n");
	return TNC_RESULT_FATAL;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *)"TNCCS-Batch"))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncc_ReceiveBatch error: wrong document type '%s', expected TNCCS-Batch\n", cur->name);
	return TNC_RESULT_FATAL;
    }

    if ((batchid = xmlGetProp(cur, (const xmlChar *) "BatchId")) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncc_ReceiveBatch error: BatchId missing\n");
	return TNC_RESULT_FATAL;
    }
    if ((recipient = xmlGetProp(cur, (const xmlChar *) "Recipient")) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncc_ReceiveBatch error: Recipient missing\n");
	return TNC_RESULT_FATAL;
    }

    // Check recipient
    if (xmlStrcmp(recipient, (const xmlChar *) "TNCC") )
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncc_ReceiveBatch error: message recipient expected TNCC, got %s\n", recipient);
	return TNC_RESULT_FATAL;
    }

    // Start a new reply batch with the next batch id
    // We hold the current doc in a global variable so that if any IMCs 
    // call TNC_TNCC_SendMessage during the processing of this batch, 
    // we will have the document reference availbale
    conn->cur_doc = libtnc_tncc_StartBatch(conn, atoi((char*)batchid) + 1);

    xmlFree(batchid);
    xmlFree(recipient);

    // Now walk the tree, handling message nodes as we go
    for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next)
    {
	// Ignore empty or blank nodes
	if (xmlIsBlankNode(cur))
	    continue;

	// Expect a list of TNCC-TNCS-Message and IMC-IMV-Message
        if (!xmlStrcmp(cur->name, (const xmlChar *) "TNCC-TNCS-Message") 
	    && cur->ns == ns) 
	{
	    xmlNodePtr       mcur;
	    TNC_UInt32       type = 0;
	    xmlNodePtr       xmlmessage = NULL;
	    unsigned char*   decoded = NULL;
	    size_t              decodedlen;
	    
	    // Message from TNCS for TNCC (this module)
	    mcur = cur->xmlChildrenNode;
	    while (mcur != NULL)
	    {
		if (!xmlStrcmp(mcur->name, (const xmlChar *) "Type") 
		    && mcur->ns == ns)
		{
		    xmlChar* content = xmlNodeGetContent(mcur);

		    type = strtol((const char*)content, NULL, 16);
		    xmlFree(content);
		}
		else if (!xmlStrcmp(mcur->name, (const xmlChar *) "XML") 
			 && mcur->ns == ns)
		{
		    // Skip empty, blank nodes
		    xmlmessage = mcur->xmlChildrenNode;
		}
		else if (!xmlStrcmp(mcur->name, (const xmlChar *) "Base64") 
			 && mcur->ns == ns)
		{
		    // Base64 encoded TNCC-TNCS-Message
		    xmlChar*       content = xmlNodeGetContent(mcur);
		    
		    decodedlen = decode_base64(content, &decoded);
		    xmlFree(content);
		}
		mcur = mcur->next;
	    }

	    // Should now have type and xmlmessage, or type and decoded
	    // Now despatch the message
	    if (decoded)
	    {
		libtnc_tncc_ReceiveMessage(conn, type, decoded, decodedlen);
		free(decoded);
	    }
	    else if (xmlmessage)
	    {
		libtnc_tncc_ReceiveMessageXML(conn, type, doc, xmlmessage, ns);
	    }

	}
	else if ((!xmlStrcmp(cur->name, (const xmlChar *) "IMC-IMV-Message")) 
		 && (cur->ns == ns)) 
	{
	    // Message from IMV to IMC
	    xmlNodePtr       mcur;
	    TNC_UInt32       type = 0;
	    unsigned char*   decoded = NULL;
	    size_t              decodedlen;
	    
	    // Message from TNCS for TNCC (this module)
	    imv_imc_count++;
	    mcur = cur->xmlChildrenNode;
	    while (mcur != NULL)
	    {
		if (!xmlStrcmp(mcur->name, (const xmlChar *) "Type") 
		    && mcur->ns == ns)
		{
		    xmlChar* content = xmlNodeGetContent(mcur);

		    type = strtoul((const char*)content, NULL, 16);
		    xmlFree(content);
		}
		else if (!xmlStrcmp(mcur->name, (const xmlChar *) "Base64") 
			 && mcur->ns == ns)
		{
		    // Base64 encoded TNCC-TNCS-Message
		    xmlChar*       content = xmlNodeGetContent(mcur);
		    
		    decodedlen = decode_base64(content, &decoded);
		    xmlFree(content);
		}
		mcur = mcur->next;
	    }

	    // Should now have type and xmlmessage, or type and decoded
	    // Now despatch the message
	    if (decoded)
	    {
		if (libtnc_imc_ReceiveMessage(conn->connectionID, decoded, decodedlen, type)
		    != TNC_RESULT_SUCCESS)
		{
		    libtnc_logMessage(TNC_LOG_SEVERITY_WARNING, "libtnc_imc_ReceiveMessage failed to handle IMC message\n");
		}
		free(decoded);
	    }
	}
    }
    xmlFreeDoc(doc);

    // If we still have a batch (ie if the TNCS has not sent a recommendation)
    // finish any reply batch and send it
    if (conn->cur_doc)
    {
	libtnc_imc_BatchEnding(conn->connectionID);
	libtnc_tncc_EndBatch(conn);
    }

    return TNC_RESULT_SUCCESS;
}

