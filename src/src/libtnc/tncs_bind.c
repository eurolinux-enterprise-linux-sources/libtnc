// tncs_bind.c
//
// Default implementation of applicaiotn level bind function
// This is called if the IMC wishes to bind a function but TNC_TNCS_BindFunction 
// has never heard of it.
// This default implementation is linked if the application has no 
// application-specific implementation.
//
// Copyright (C) 2007 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: tncs_bind.c,v 1.1 2007/05/06 01:06:02 mikem Exp $

#include <libtncconfig.h>
#include <libtncimv.h>


///////////////////////////////////////////////////////////
// Default implemetation does nothing
TNC_Result libtnc_tncs_BindFunction(
    TNC_IMVID imcID,
    char *functionName,
    void **pOutfunctionPointer)
{
    return TNC_RESULT_INVALID_PARAMETER;
}
