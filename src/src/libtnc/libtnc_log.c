// libtnc_log.c
//
// Optional error logging function for libtnc
// Replace this with your own function to provide
// application specific logging messages
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtnc_log.c,v 1.3 2007/05/06 01:06:02 mikem Exp $

#include <libtncconfig.h>
#include <stdio.h>
#include <stdarg.h>
#include <libtnc.h>

TNC_Result libtnc_logMessage(
    /*in*/ TNC_UInt32 severity,
    /*in*/ const char* format, ...)
{
    va_list arg;
    va_start(arg, format);
    vfprintf(stderr, format, arg);
    return TNC_RESULT_SUCCESS;
}
