// base64.h
//
// Encode and decode base64 encoded data.
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: base64.h,v 1.1 2007/04/15 04:56:13 mikem Exp $

#ifndef _BASE64_H
#define _BASE64_H
#ifdef __cplusplus
extern "C" {
#endif

// Result will be split into lines of at most MAX_LINE chars, terminated by eol
// Always NUL terminates the result
// Returns the length of the result (not counting the NUL termination)
// *result must be freed
extern size_t encode_base64(
    /*in*/  unsigned char* str, 
    /*in*/  size_t len, 
    /*out*/ char**result);


// Always NUL terminates the result
// Returns the length of the result (not counting the NUL termination)
// *result must be freed
extern size_t decode_base64(
    /*in*/  char* str, 
    /*out*/ unsigned char**result);

#ifdef __cplusplus
}
#endif
#endif
