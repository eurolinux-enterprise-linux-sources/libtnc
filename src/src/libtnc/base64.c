// base64.c
//
// Encode and decode base64 encoded data.
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: base64.c,v 1.2 2007/05/06 01:06:02 mikem Exp $

#include <libtncconfig.h>
#include <string.h>
#include <stdlib.h>
#include <base64.h>

#define MAX_LINE  76 /* size of encoded lines */

static char basis_64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define XX      255	/* illegal base64 char */
#define EQ      254	/* padding */
#define INVALID XX

static unsigned char index_64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,EQ,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,

    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

// Result will be split into lines of at most MAX_LINE chars, terminated by eol
// Always NUL terminates the result
// Returns the length of the result (not counting the NUL termination)
// *result must be freed

// FHH
// base64 padding: add number of NULL-bytes to input, so that
// resulting number of input bytes % 3 = 0. For each NULL-byte added to the
// input, the base64 encoded data has a '=' at the end.
// i.e., there are 0, 1 or 2 NULL-bytes present.
// case 0 NULL-bytes
//      7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      |               |               |               |
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      c1              c2              c3
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      |     b3    |     b2    |     b1    |     b0    |
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      i.e. use c1, c2, c3 to get input data
// case 1 NULL-bytes, '='
//       7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      |               |               |0 0 0 0 0 0 0 0|
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      c1              c2              c3
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      |     b3    |     b2    |     b1    |     b0    |
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      i.e. use c1, c2 to get input data
// case 2 NULL-bytes. '=='
//       7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      |               |0 0 0 0 0 0 0 0|0 0 0 0 0 0 0 0|
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      c1              c2              c3
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      |     b3    |     b2    |     b1    |     b0    |
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     i.e. use only c1 to get input data 
//
size_t
encode_base64(
    /*in*/  unsigned char* str, 
    /*in*/  size_t len, 
    /*out*/ char** result)
{
    char *eol = "\n";     /* the end-of-line sequence to use */
    size_t eollen = strlen(eol); /* length of the EOL sequence */
    char *r;       /* result string */
    size_t rlen = ((len+2) / 3 * 4) + 1;   /* length of result string */
    unsigned char c1, c2, c3;
    size_t chunk;
    size_t rindex = 0;

    // add space for EOLs
    if (rlen) 
	rlen += ((rlen-1) / MAX_LINE + 1) * eollen;

    // Allocate result buffer
    r = malloc(rlen);

    // encode 
    for (chunk=0; len > 0; len = (len < 3) ? 0 : (len - 3), chunk++) 
    {
	if (chunk == (MAX_LINE/4)) 
	{
	    char *c = eol;
	    char *e = eol + eollen;
	    while (c < e)
		r[rindex++] = *c++;
	    chunk = 0;
	}
	c1 = *str++;
	c2 = *str++;
	r[rindex++] = basis_64[c1>>2];
	// FHH patch
	if(len == 1){
		r[rindex++] = basis_64[((c1 & 0x3)<< 4)];
	} else {
		r[rindex++] = basis_64[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)]; // does not work if len==1, c2 is not valid
	}

	if (len > 2) 
	{
	    c3 = *str++;
	    r[rindex++] = basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)];
	    r[rindex++] = basis_64[c3 & 0x3F];
	} 
	else if (len == 2) 
	{
	    r[rindex++] = basis_64[(c2 & 0xF) << 2];
	    r[rindex++] = '=';
	} 
	else 
	{ /* len == 1 */
	    r[rindex++] = '=';
	    r[rindex++] = '=';
	}
    }
    if (rlen) 
    {
	/* append eol to the result string */
	char *c = eol;
	char *e = eol + eollen;
	while (c < e)
	    r[rindex++] = *c++;
    }
    r[rindex] = '\0';  /* every SV in perl should be NUL-terminated */
    *result = r;
    return rindex;
}

// Always NUL terminates the result
// Returns the length of the result (not counting the NUL termination)
// *result must be freed
size_t
decode_base64(
    /*in*/  char* str, 
    /*out*/ unsigned char**result)
{
    size_t               len = strlen(str);
    unsigned char*       r;
    size_t               rlen = ((len * 3) / 4) + 1; // always enough
    char*                end = str + len;
    size_t                  rindex = 0;
    unsigned char        c[4];

    r = malloc(rlen);
    while (str < end) 
    {
	int i = 0;
	do {
	    unsigned char uc = index_64[*str++];
	    if (uc != INVALID)
		c[i++] = uc;

	    if (str == end) {
		if (i < 4) {
		    // Premature end of base64 data if i
		    if (i < 2) goto finished;
		    if (i == 2) c[2] = EQ;
		    c[3] = EQ;
		}
		break;
	    }
	} while (i < 4);
	
	if (c[0] == EQ || c[1] == EQ) {
	    // Premature padding of base64 data
	    break;
	}
	/* printf("c0=%d,c1=%d,c2=%d,c3=%d\n", c[0],c[1],c[2],c[3]);/**/
	
	r[rindex++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
	
	if (c[2] == EQ)
	    break;
	r[rindex++] = ((c[1] & 0x0F) << 4) | ((c[2] & 0x3C) >> 2);
	
	if (c[3] == EQ)
	    break;
	r[rindex++] = ((c[2] & 0x03) << 6) | c[3];
    }
    
 finished:
    r[rindex] = '\0'; // Make sure its null terminated
    *result = r;
    return rindex;
}

