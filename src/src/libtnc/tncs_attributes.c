// tncs_attributes.c
//
// Default implementations of TNC_TNCS_SetAttribute, TNC_TNCS_GetAttribute,
// which will be linked if the caller does not implement 
// attribute setting and getting.
// Caution: implements global per-TNCS attributes, not per-connection or per-imv attributes
//
// Copyright (C) 2007 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: tncs_attributes.c,v 1.2 2007/05/06 01:06:02 mikem Exp $
//
// Changes 2010/10/27, Jan Stalhut, Decoit GmbH:
// Added TNCCS-TNCSContactInfo to inform the TNCC of the TNCS's IP and port
// as defined in IF-TNCCS v1.2, Revision 6.00, section 2.8.5.5

#include <libtncconfig.h>
#include <string.h>
#include <libtncimv.h>
#include <libtncdebug.h>

// Storage for our attributes
// revisit: should use dynamic arrays?
#define MAX_ATTRID 5
static TNC_BufferReference values[MAX_ATTRID+1];
static TNC_UInt32          lengths[MAX_ATTRID+1];

TNC_Result TNC_TNCS_GetAttribute(
    /*in*/  TNC_IMVID imvID,
    /*in*/  TNC_ConnectionID connectionID,
    /*in*/  TNC_AttributeID attributeID,
    /*in*/  TNC_UInt32 bufferLength,
    /*out*/ TNC_BufferReference buffer,
    /*out*/ TNC_UInt32 *pOutValueLength)
{
    DEBUG4("TNC_TNCS_GetAttribute %d %d %d", imvID, connectionID, attributeID);
    
    // Valid ID?
    if (attributeID <= 0 || attributeID > MAX_ATTRID)
	return TNC_RESULT_INVALID_PARAMETER;
	
    // No value yet?
    if (!values[attributeID])
	return TNC_RESULT_INVALID_PARAMETER;

    // Actually copy
    if (bufferLength && buffer && bufferLength >= lengths[attributeID])
	memcpy(buffer, values[attributeID], lengths[attributeID]);

    if (pOutValueLength)
	*pOutValueLength = lengths[attributeID];

    return TNC_RESULT_SUCCESS;
}


TNC_Result TNC_TNCS_SetAttribute(
    /*in*/  TNC_IMVID imvID,
    /*in*/  TNC_ConnectionID connectionID,
    /*in*/  TNC_AttributeID attributeID,
    /*in*/  TNC_UInt32 bufferLength,
    /*in*/  TNC_BufferReference buffer)
{
    // Valid ID?
    if (attributeID <= 0 || attributeID > MAX_ATTRID)
	return TNC_RESULT_INVALID_PARAMETER;

    // Need some or some more space
    // Note we let it shrink without reallocing
    if (lengths[attributeID] < bufferLength)
	values[attributeID] = (TNC_BufferReference)realloc(values[attributeID], bufferLength);
    lengths[attributeID] = bufferLength;
    if (bufferLength && buffer)
	memcpy(values[attributeID], buffer, bufferLength);

    return TNC_RESULT_SUCCESS;
}

