// libtnctncs.h
//
// Implement the TNCS layer of IF-TNCSS, part
// of TNC, including XML based batch packing and unpacking
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtnctncs.c,v 1.4 2009/03/18 11:28:35 mikem Exp mikem $
//
// Changes 2010/10/27, Jan Stalhut, Decoit GmbH:
// Added TNCCS-TNCSContactInfo to inform the TNCC of the TNCS's IP and port
// as defined in IF-TNCCS v1.2, Revision 6.00, section 2.8.5.5

#include<string.h>

#include <libtncconfig.h>
#include <libtnctncs.h>
#include <libtncxml.h>
#include <libtncdebug.h>
#include <libtncarray.h>
#include <mutex.h>
#include <base64.h>

// Dynamic array of libtnc_tncs_connection indexed by connectionID
// Protected by mutex
// Need this because the lower layers only understand connection IDs, 
// and make calls into us, passing the connection ID. we need to be able to 
// resolve this to a connection context.
static libtnc_array connections = LIBTNC_ARRAY_INIT;

// Hom many IMVs we have loaded
static TNC_UInt32 num_imvs = 0;

///////////////////////////////////////////////////////////
// Initialise data to start a new batch of messages
// Later we will maybe add messages when the IMV layer calls into us
static
xmlDocPtr libtnc_tncs_StartBatch(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ int batchid)
{
    DEBUG2("libtnc_tncs_StartBatch %d", batchid);
    return libtncxml_new(batchid, "TNCC");
}

///////////////////////////////////////////////////////////
// Add an TNCC-TNCS-Message type TNCSS-Error to the 
// XML TNCSS-Batch document currently
// under construction
static
TNC_Result libtnc_tncs_add_tnccs_error(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ const char* type,
    /*in*/ const char* message)
{
    DEBUG3("libtnc_tncs_add_tnccs_error %s %s", type, message);
    libtncxml_add_tnccs_error(conn->cur_doc, type, message);
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Add an IMC-IMV-Message to the XML TNCSS-Batch document currently
// under construction
static
TNC_Result libtnc_tncs_add_imc_imv_message(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)

{
    DEBUG1("libtnc_tncs_add_imc_imv_message");

    conn->imv_imc_count++;
    libtncxml_add_imc_imv_message(conn->cur_doc, message, messageLength, messageType);
    return TNC_RESULT_SUCCESS;
}

static
TNC_Result libtnc_tncs_recommend(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ TNC_UInt32 recommendation)
{
    TNC_BufferReference reasonString = 0;
    TNC_UInt32          reasonStringLength = 0;
    TNC_BufferReference reasonLang = 0;
    TNC_UInt32          reasonLangLength = 0;
    TNC_BufferReference contactAddress = 0;
    TNC_UInt32          contactAddressLength = 0;
    TNC_BufferReference contactPort = 0;
    TNC_UInt32          contactPortLength = 0;
    
    libtncxml_add_tnccs_recommendation(conn->cur_doc, recommendation);
    
    // Version 1.2 compliant IMVs may also have set Reason String and
    // Reason Language attributes, which we might use here, if available
    // This implementation only keeps one Reason String
    // Find out how much we need, alloc that much space then get the value
    if (TNC_TNCS_GetAttribute(0, conn->connectionID, TNC_ATTRIBUTEID_REASON_LANGUAGE, 0, 0, &reasonLangLength) == TNC_RESULT_SUCCESS
	&& reasonLangLength
	&& (reasonLang = malloc(reasonLangLength)))
    {
	reasonLang[reasonLangLength] = '\0';
	TNC_TNCS_GetAttribute(0, conn->connectionID, TNC_ATTRIBUTEID_REASON_LANGUAGE, reasonLangLength, reasonLang, &reasonLangLength);
    }
    if (TNC_TNCS_GetAttribute(0, conn->connectionID, TNC_ATTRIBUTEID_REASON_STRING, 0, 0, &reasonStringLength) == TNC_RESULT_SUCCESS
	&& reasonStringLength
	&& (reasonString = malloc(reasonStringLength)))
    {
	reasonString[reasonStringLength] = '\0';
	TNC_TNCS_GetAttribute(0, conn->connectionID, TNC_ATTRIBUTEID_REASON_STRING, reasonStringLength, reasonString, &reasonStringLength);
    }

    if (TNC_TNCS_GetAttribute(0, conn->connectionID, TNC_ATTRIBUTEID_CONTACT_ADDRESS, 0, 0, &contactAddressLength) == TNC_RESULT_SUCCESS
	&& contactAddressLength 
	&& (contactAddress = malloc(contactAddressLength)))
    {
	contactAddress[contactAddressLength] = '\0';
	TNC_TNCS_GetAttribute(0, conn->connectionID, TNC_ATTRIBUTEID_CONTACT_ADDRESS, contactAddressLength, contactAddress, &contactAddressLength);
    }

    if (TNC_TNCS_GetAttribute(0, conn->connectionID, TNC_ATTRIBUTEID_CONTACT_PORT, 0, 0, &contactPortLength) == TNC_RESULT_SUCCESS
	&& contactPortLength && 
	(contactPort = malloc(contactPortLength)))
    {
	contactPort[contactPortLength] = '\0';
	TNC_TNCS_GetAttribute(0, conn->connectionID, TNC_ATTRIBUTEID_CONTACT_PORT, contactPortLength, contactPort, &contactPortLength);
    }

    if (reasonLang && reasonString)
	libtncxml_add_tnccs_reason(conn->cur_doc, (const char*)reasonLang, (const char*)reasonString);
    if (reasonLang)
	free(reasonLang);
    if (reasonString)
	free(reasonString);
    
    if (contactAddress && contactPort)
	libtncxml_add_tnccs_tncscontactinfo(conn->cur_doc, (const char*)contactAddress, (const char*)contactPort);
    if (contactAddress)
	free(contactAddress);
    if (contactPort)
	free(contactPort);

    return TNC_RESULT_SUCCESS;
}


///////////////////////////////////////////////////////////
static
TNC_Result libtnc_tncs_EndBatch(
    /*in*/ libtnc_tncs_connection* conn)
{
    xmlChar*   xmlbuff;
    size_t     buffersize;
    TNC_Result ret;

    DEBUG2("libtnc_tncs_EndBatch %d", conn->connectionID);
    // Call back into the caller with the finished buffer to send
    xmlDocDumpFormatMemory(conn->cur_doc, &xmlbuff, (int*)&buffersize, 1);
    ret = TNC_TNCS_SendBatch(conn, (char *)xmlbuff, buffersize);
    xmlFree(xmlbuff);
    xmlFreeDoc(conn->cur_doc);
    conn->cur_doc = NULL;
    return ret;
}

TNC_Result libtnc_tncs_HaveRecommendation(
	/*in*/ libtnc_tncs_connection* conn,
	/*out*/ TNC_IMV_Action_Recommendation* recommendation,
	/*out*/ TNC_IMV_Evaluation_Result* evaluation)
{
    if (conn->have_recommendation)
    {
	if (recommendation)
	    *recommendation = conn->recommendation;
	if (evaluation)
	    *evaluation = conn->evaluation;
	return TNC_RESULT_SUCCESS;
    }
    else
    {
	return TNC_RESULT_FATAL;
    }
}

///////////////////////////////////////////////////////////
// Called by the IMV layer
TNC_Result TNC_TNCS_RequestHandshakeRetry(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_RetryReason reason)
{
    DEBUG4("TNC_TNCS_RequestHandshakeRetry %d %d %d", imvID, connectionID, reason);

    // REVISIT
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Called by the IMV layer
// Here we amalgamate the recommendations from all the installed IMVs into
// the recommendation connection variable, based on the setting of recommendationPolicy
TNC_Result TNC_TNCS_ProvideRecommendation(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_IMV_Action_Recommendation recommendation,
    /*in*/ TNC_IMV_Evaluation_Result evaluation)
{
    libtnc_tncs_connection* conn;
    TNC_UInt32              i;

    DEBUG5("TNC_TNCS_ProvideRecommendation  %d %d %d %d", imvID, connectionID, recommendation, evaluation);

    libtnc_mutex_lock();
    conn = libtnc_array_index(&connections, connectionID);
    libtnc_mutex_unlock();

    // Save the recommendation
    conn->recommendations[imvID] = recommendation;
    conn->have_recommendations[imvID]++;

    // See if we have a final recommendation
    for (i = 0; i < num_imvs; i++)
    {
	if (!conn->have_recommendations[i])
	    break;
    }

    if (i == num_imvs && num_imvs > 0)
    {
	TNC_UInt32 prov_rec = conn->recommendations[0]; // Provisional

	// Have recommendations from all IMVs, now figure out what it is
	conn->have_recommendation++;

	if (conn->recommendationPolicy == LIBTNC_TNCS_RECOMMENDATION_POLICY_ALL)
	{
	    // LIBTNC_TNCS_RECOMMENDATION_POLICY_ALL
	    for (i = 1; i < num_imvs; i++)
	    {
		if (conn->recommendations[i] != prov_rec)
		    break;
	    }
	    if (i == num_imvs)
		conn->recommendation = prov_rec; // They were all the same
	}
	else if (conn->recommendationPolicy == LIBTNC_TNCS_RECOMMENDATION_POLICY_ANY)
	{
	    // LIBTNC_TNCS_RECOMMENDATION_POLICY_ANY
	    for (i = 1; i < num_imvs; i++)
	    {
		if (conn->recommendations[i] == TNC_IMV_ACTION_RECOMMENDATION_ALLOW)
		    prov_rec = TNC_IMV_ACTION_RECOMMENDATION_ALLOW;
		else if (   conn->recommendations[i] == TNC_IMV_ACTION_RECOMMENDATION_ISOLATE
			    && prov_rec != TNC_IMV_ACTION_RECOMMENDATION_ALLOW)
		    prov_rec = TNC_IMV_ACTION_RECOMMENDATION_ISOLATE;
		else
		    prov_rec = TNC_IMV_ACTION_RECOMMENDATION_NO_ACCESS;
	    }
	    conn->recommendation = prov_rec;
	}
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Called by the IMV layer when an IMC-IMV message is to be
// sent. Add it to the current batch
TNC_Result TNC_TNCS_SendMessage(
    /*in*/ TNC_IMVID imvID,
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference message,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
    libtnc_tncs_connection* conn;

    DEBUG6("TNC_TNCS_SendMessage %d %d %s %d %x", imvID, connectionID, message, messageLength, messageType);
    libtnc_mutex_lock();
    conn = libtnc_array_index(&connections, connectionID);
    libtnc_mutex_unlock();
    return libtnc_tncs_add_imc_imv_message(conn, message, messageLength, messageType);
}

///////////////////////////////////////////////////////////
// Handle a TNCC-TNCS-Message received from TNCC
TNC_Result libtnc_tncs_ReceiveMessage(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ unsigned char* message, 
    /*in*/ size_t messagelen)
{
    TNC_VendorID       mv = (messageType >> 8) & TNC_VENDORID_ANY;

    DEBUG3("libtnc_tncs_ReceiveMessage %d %x", conn->connectionID, messageType);
    // Vendor specific?
    if (mv != 0)
	return TNC_TNCS_ReceiveMessage(conn, messageType, message, messagelen);

    // There are no base64 encoded TNCC-TNCS message. All the ones defined
    // by TNC use XML, which will go to libtnc_tncs_tncs_ReceiveMessageXML
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Handle a TNCC-TNCS-Message encoded in an XML node received from TNCC
TNC_Result libtnc_tncs_ReceiveMessageXML(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ TNC_UInt32 messageType, 
    /*in*/ xmlDocPtr  doc,
    /*in*/ xmlNodePtr cur,
    /*in*/ xmlNsPtr   ns)
{
    TNC_VendorID       mv = (messageType >> 8) & TNC_VENDORID_ANY;

    DEBUG2("libtnc_tncs_ReceiveMessageXML %x", messageType);
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
	return TNC_TNCS_ReceiveMessageXML(conn, messageType, doc, cur, ns);
    
    if (!xmlStrcmp(cur->name, (const xmlChar *) "TNCSS-Error") 
	     && cur->ns == ns)
    {
	// should be type 2
	xmlChar* rtype = xmlGetProp(cur, (const xmlChar *)"type");
	xmlChar* content = xmlNodeGetContent(cur);

	// Expect 'batch-too-long', 'malformed-batch' etc
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "TNCSS-Error %s: %s\n", rtype, content);
	xmlFree(rtype);
	xmlFree(content);
    }
    else if (!xmlStrcmp(cur->name, (const xmlChar *) "TNCCS-PreferredLanguage") 
	     && cur->ns == ns)
    {
	// Tell the IMVs about the clinets preferred language
	xmlChar* content = xmlNodeGetContent(cur);
	TNC_TNCS_SetAttribute(0, conn->connectionID, TNC_ATTRIBUTEID_PREFERRED_LANGUAGE, 
			      strlen((const char *)content) + 1, content);
	xmlFree(content);
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Initialise the TNCS layer using a named config file
// Call once before any other TNCS functions
// Use libtnc_tncs_Initialize in production code
TNC_Result libtnc_tncs_Initialize(
    /*in*/const char* filename)
{
    int load_count = 0;

    DEBUG2("libtnc_tncs_Initialize %s", filename);
    if ((load_count = libtnc_imv_load_config(filename)) < 0)
    {
	// Problem loading
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncs_Initialize error: failed to load IMV configuration\n");
	num_imvs = 0;
	return TNC_RESULT_FATAL;
    }
    else
    {
	// Success
	num_imvs = load_count;
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Initialise the TNCS layer using the config for the standard places
TNC_Result libtnc_tncs_InitializeStd()
{
    int load_count = 0;

    DEBUG1("libtnc_tncs_InitializeStd");
    if ((load_count = libtnc_imv_load_std_config()) < 0)
    {
	// Problem loading
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncs_Initialize error: failed to load IMV configuration\n");
	return TNC_RESULT_FATAL;
    }
    else
    {
	// Success
	num_imvs = load_count;
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Terminate the TNCS layer
TNC_Result libtnc_tncs_Terminate()
{
    DEBUG1("libtnc_tncs_Terminate");
    TNC_UInt32 i;

    for(i = 0; i < connections.max; i++)
    {
	free(connections.data[i]);
        connections.data[i] = NULL;
    }

    libtnc_array_free(&connections);
    return libtnc_imv_Terminate();
}

///////////////////////////////////////////////////////////
// Create and initialise a new connection context
libtnc_tncs_connection* libtnc_tncs_CreateConnection(void* appData)
{
    libtnc_tncs_connection* ret;

    if ((ret = (libtnc_tncs_connection*)calloc
	 (1, sizeof(libtnc_tncs_connection))) != NULL)
    {
	TNC_UInt32 i;

	// Lock the connections array
	libtnc_mutex_lock();
	ret->connectionID = libtnc_array_new(&connections, ret);
	libtnc_mutex_unlock();
	ret->appData = appData;
	ret->recommendationPolicy = LIBTNC_TNCS_RECOMMENDATION_POLICY_ALL;
	// Clear the saved recommendations
	for (i = 0; i < num_imvs; i++)
	    ret->recommendations[i] = TNC_IMV_ACTION_RECOMMENDATION_NO_RECOMMENDATION;
	ret->recommendation = TNC_IMV_ACTION_RECOMMENDATION_NO_RECOMMENDATION;
	ret->evaluation = TNC_IMV_EVALUATION_RESULT_DONT_KNOW;

    }
    return ret;
}

///////////////////////////////////////////////////////////
// Return connection context application data
void* libtnc_tncs_ConnectionAppData(libtnc_tncs_connection* conn)
{
    return conn->appData;
}

///////////////////////////////////////////////////////////
// Return connection context application data
int libtnc_tncs_SetRecommendationPolicy(libtnc_tncs_connection* conn, int policy)
{
    int ret = conn->recommendationPolicy;
    conn->recommendationPolicy = policy;
    return ret;
}

///////////////////////////////////////////////////////////
// Delete a connection context
// return its connection id
TNC_ConnectionID libtnc_tncs_DeleteConnection(libtnc_tncs_connection* conn)
{
    TNC_ConnectionID connectionID = 0;
    // Lock the connections array
    libtnc_mutex_lock();
    connectionID = conn->connectionID;
    libtnc_imv_NotifyConnectionChange(conn->connectionID, TNC_CONNECTION_STATE_DELETE);
    libtnc_array_delete(&connections, conn->connectionID);
    libtnc_mutex_unlock();
    xmlFreeDoc(conn->cur_doc);
    free(conn);

    return connectionID;
}

///////////////////////////////////////////////////////////
// Start a new handshaking session
// May result in the IMV sending some messages into the first batch?
TNC_Result libtnc_tncs_BeginSession(
    /*in*/ libtnc_tncs_connection* conn)
{
    DEBUG2("libtnc_tncs_BeginSession %d", conn->connectionID);
//    conn->cur_doc = libtnc_tncs_StartBatch(conn, 1); // First batch is always id 1
    libtnc_imv_NotifyConnectionChange(conn->connectionID, TNC_CONNECTION_STATE_CREATE);
    libtnc_imv_NotifyConnectionChange(conn->connectionID, TNC_CONNECTION_STATE_HANDSHAKE);
//    return libtnc_tncs_EndBatch(conn);
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Deliver an XML encapsulated batch of messages to TNCS from the TNCC
// Some messages may be TNCC-TNCS messages, and some may be IMC-IMV messages
TNC_Result libtnc_tncs_ReceiveBatch(
    /*in*/ libtnc_tncs_connection* conn,
    /*in*/ const char*  messageBuffer,
    /*in*/ size_t messageLength)
{
    xmlDocPtr        doc;
    xmlNodePtr       cur;
    xmlNsPtr         ns;
    xmlChar*         batchid;
    xmlChar*         recipient;
    int              imc_imv_count = 0;

    DEBUG2("libtnc_tncs_ReceiveBatch %d", conn->connectionID);
    conn->imv_imc_count = 0;
    if ((doc = xmlParseMemory(messageBuffer, messageLength)) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncs_ReceiveBatch error: failed to XML parse message\n");
	return TNC_RESULT_FATAL;
    }

    // Check out the document
    if ((cur = xmlDocGetRootElement(doc)) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncs_ReceiveBatch error: empty document\n");
	return TNC_RESULT_FATAL;
    }

    if ((ns = xmlSearchNsByHref
	 (doc, cur, (const xmlChar *) "http://www.trustedcomputinggroup.org/IWG/TNC/1_0/IF_TNCCS#")) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncs_ReceiveBatch error: TNCCS namespace not found\n");
	return TNC_RESULT_FATAL;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *)"TNCCS-Batch"))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncs_ReceiveBatch error: wrong document type '%s', expected TNCCS-Batch\n", cur->name);
	return TNC_RESULT_FATAL;
    }

    if ((batchid = xmlGetProp(cur, (const xmlChar *) "BatchId")) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncs_ReceiveBatch error: BatchId missing\n");
	return TNC_RESULT_FATAL;
    }
    if ((recipient = xmlGetProp(cur, (const xmlChar *) "Recipient")) == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncs_ReceiveBatch error: Recipient missing\n");
	return TNC_RESULT_FATAL;
    }

    // Check recipient
    if (xmlStrcmp(recipient, (const xmlChar *) "TNCS") )
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "libtnc_tncs_ReceiveBatch error: message recipient expected TNCS, got %s\n", recipient);
	return TNC_RESULT_FATAL;
    }

    // Start a new reply batch with the next batch id
    // We hold the current doc in a global variable so that if any IMVs 
    // call TNC_TNCS_SendMessage during the processing of this batch, 
    // we will have the document reference availbale
    conn->cur_doc = libtnc_tncs_StartBatch(conn, atoi((char*)batchid) + 1);

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
	    size_t           decodedlen;
	    
	    // Message from TNCC for TNCS (this module)
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
		    xmlmessage = mcur->xmlChildrenNode;
		}
		else if (!xmlStrcmp(mcur->name, (const xmlChar *) "Base64") 
			 && mcur->ns == ns)
		{
		    // Base64 encoded TNCC-TNCS-Message
		    xmlChar*       content = xmlNodeGetContent(mcur);
		    
		    decodedlen = decode_base64((char *)content, &decoded);
		    xmlFree(content);
		}
		mcur = mcur->next;
	    }

	    // Should now have type and xmlmessage, or type and decoded
	    // Now despatch the message
	    if (decoded)
	    {
		libtnc_tncs_ReceiveMessage(conn, type, decoded, decodedlen);
		free(decoded);
	    }
	    else if (xmlmessage)
	    {
		libtnc_tncs_ReceiveMessageXML(conn, type, doc, xmlmessage, ns);
	    }

	}
	else if ((!xmlStrcmp(cur->name, (const xmlChar *) "IMC-IMV-Message")) 
		 && (cur->ns == ns)) 
	{
	    // Message from IMC to IMV
	    xmlNodePtr       mcur;
	    TNC_UInt32       type = 0;
	    unsigned char*   decoded = NULL;
	    size_t           decodedlen;
	    
	    // Message from TNCC for TNCS (this module)
	    imc_imv_count++;
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
		    
		    decodedlen = decode_base64((char *)content, &decoded);
		    xmlFree(content);
		}
		mcur = mcur->next;
	    }

	    // Should now have type and xmlmessage, or type and decoded
	    // Now despatch the message
	    if (decoded)
	    {
		if (libtnc_imv_ReceiveMessage(conn->connectionID, decoded, decodedlen, type)
		    != TNC_RESULT_SUCCESS)
		{
		    libtnc_logMessage(TNC_LOG_SEVERITY_WARNING, "libtnc_imv_ReceiveMessage failed to handle IMV message\n");
		}
		free(decoded);
	    }
	}
	cur = cur->next;
    }
    xmlFreeDoc(doc);

    // Have delivered all the messages, tell the IMVs so
    // This may provoke some outgoing messages or a recommendation
    libtnc_imv_BatchEnding(conn->connectionID);

    if (conn->have_recommendation)
    {
	// All the IMVs have made a decision, so send it
	libtnc_tncs_recommend(conn, conn->recommendation);
    }
    else if (imc_imv_count == 0 || conn->imv_imc_count == 0)
    {
	// Nothing from the TNCC, which means the TNCC wants to end the handshake?
	// Nothing to be sent to the TNCC, which means the TNCS wants to end the handshake?
	// Ask for the final recommendation
	// Calling SolicitRecommendation will result in TNC_TNCS_ProvideRecommendation
	// being called for each IMV. This will result in recommendation being set
	// according to the consensus of all the IMVs according the recommendation policy.
	libtnc_imv_SolicitRecommendation(conn->connectionID);
	libtnc_tncs_recommend(conn, conn->recommendation);
    }

    // Perhaps finish any reply batch and send it
    return libtnc_tncs_EndBatch(conn);
}

