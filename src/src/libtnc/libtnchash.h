// hash.h
//
// Dynamic hash code
//
// Copyright (C) 2008 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtnchash.h,v 1.2 2009/08/12 09:43:53 mikem Exp mikem $

#ifndef _HASH_H
#define _HASH_H
#ifdef __cplusplus
extern "C" {
#endif

#include <tncif.h>
#include "libtncarray.h"

// Holds data for a hash
typedef struct
{
    libtnc_array array; // We fake the hash with arrays and linear search
} libtnc_hash;

// Holds data for a hash element
typedef struct
{
    TNC_BufferReference  key;    // Key, binary
    TNC_UInt32           keylen; // Length of the key
    void*               data;   // user data stored by the node
} libtnc_hash_node;

// Initialiser for arrays
#define    LIBTNC_HASH_INIT {{0, 0, 0}}
#define    LIBTNC_HASH_NODE_INIT {0, 0}

// Allocate a new hash structure, intialised to 0
extern libtnc_hash* libtnc_hash_create();

// Free the contents and then the previously created hash
// If destroy is not NULL, call it as destroy(userdata) for each item
extern TNC_UInt32 libtnc_hash_destroy(libtnc_hash* hash, void(*destroy)(void*));

// Initialise a new array and set an initial size of 0
// Returns 1
extern TNC_UInt32 libtnc_hash_init(libtnc_hash* hash);

// Free any storage associated with the hash, and clear it back to an empty state
// Does not free whatever the stored data pointer point to
extern TNC_UInt32 libtnc_hash_free(libtnc_hash* hash, void(*destroy)(void*));

// Add data to the hash
// Return the value of the preexisting key if there was one
extern void* libtnc_hash_add(libtnc_hash* hash, TNC_BufferReference key, TNC_UInt32 keylen, void* data);
extern void* libtnc_hash_addS(libtnc_hash* hash, char* key, void* data);

// Return the data associated with the hash key
extern void* libtnc_hash_get(libtnc_hash* hash, TNC_BufferReference key, TNC_UInt32 keylen);
extern void* libtnc_hash_getS(libtnc_hash* hash, char* key);

// Remove data from the hash
// Return the data for the hash key
extern void* libtnc_hash_delete(libtnc_hash* hash, TNC_BufferReference key, TNC_UInt32 keylen);
extern void* libtnc_hash_deleteS(libtnc_hash* hash, char* key);

#ifdef __cplusplus
}
#endif
#endif
