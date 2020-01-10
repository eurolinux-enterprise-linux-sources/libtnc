// hash.c
//
// Dynamic hash code
//
// At present, on the assumption that there will be few hash keys,
// hashes are simulated with arrays and linear searches
//
// Copyright (C) 2008 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtnchash.c,v 1.1 2009/03/18 11:28:35 mikem Exp mikem $

#include <libtncconfig.h>
#include "libtnchash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static libtnc_hash_node* 
// PERFORMANCE ALERT: linear search
find_node(libtnc_hash* hash, TNC_BufferReference key, TNC_UInt32 keylen)
{
    TNC_UInt32 size = libtnc_array_size(&hash->array);
    int i;

    for (i = 0; i < size; i++)
    {
	libtnc_hash_node *n = (libtnc_hash_node*)libtnc_array_index(&hash->array, i);
	// Caution, array can have gaps after deletes
	if (n && keylen == n->keylen && memcmp(key, n->key, keylen) == 0)
	    return n; // found
    }
    return NULL; // not found
}

// Free all the storage associated with a node
static void
node_free(libtnc_hash_node* n)
{
    free(n->key);
    free(n);
}

libtnc_hash* libtnc_hash_create()
{
    libtnc_hash* hash = calloc(1, sizeof(libtnc_hash));

    if (hash)
	libtnc_hash_init(hash);
    return hash;
}

TNC_UInt32 
libtnc_hash_destroy(libtnc_hash* hash, void(*destroy)(void*))
{
    libtnc_hash_free(hash, destroy);

    free(hash);
    return 1;
}

TNC_UInt32 
libtnc_hash_init(libtnc_hash* hash)
{
    return libtnc_array_init(&hash->array);
}

TNC_UInt32 
libtnc_hash_free(libtnc_hash* hash, void(*destroy)(void*))
{
    TNC_UInt32 size = libtnc_array_size(&hash->array);
    int i;

    // Free all the nodes
    for (i = 0; i < size; i++)
    {
	libtnc_hash_node *n = (libtnc_hash_node*)libtnc_array_index(&hash->array, i);
	if (n)
	{
	    if (destroy)
		(*destroy)(n->data);
	    node_free(n);
	}
    }
    // Then the array struct
    return libtnc_array_free(&hash->array);
}

void*
libtnc_hash_add(libtnc_hash* hash, TNC_BufferReference key, TNC_UInt32 keylen, void* data)
{
    libtnc_hash_node *n = find_node(hash, key, keylen);

    if (n)
    {
	void* oldData = n->data;
	n->data = data;
	return oldData;
    }
    else
    {
	if ((n = calloc(1, sizeof(libtnc_hash_node)))
	    && (n->key = calloc(keylen, 1)))
	{
	    memcpy(n->key, key, keylen);
	    n->keylen = keylen;
	    n->data = data;
	    libtnc_array_new(&hash->array, n);
	}
	return 0;
    }
}

void*
libtnc_hash_addS(libtnc_hash* hash, char* key, void* data)
{
    return libtnc_hash_add(hash, (TNC_BufferReference)key, (TNC_UInt32)strlen(key), data);
}

void* 
libtnc_hash_get(libtnc_hash* hash, TNC_BufferReference key, TNC_UInt32 keylen)
{
    libtnc_hash_node *n = find_node(hash, key, keylen);
    if (n)
	return n->data;
    else
	return NULL;
}

void* 
libtnc_hash_getS(libtnc_hash* hash, char* key)
{
    return libtnc_hash_get(hash, (TNC_BufferReference)key, (TNC_UInt32)strlen(key));
}

void*
libtnc_hash_delete(libtnc_hash* hash, TNC_BufferReference key, TNC_UInt32 keylen)
{
    TNC_UInt32 size = libtnc_array_size(&hash->array);
    int i;

    for (i = 0; i < size; i++)
    {
	libtnc_hash_node *n = (libtnc_hash_node*)libtnc_array_index(&hash->array, i);
	// Caution, array can have gaps after deletes
	if (n && keylen == n->keylen && memcmp(key, n->key, keylen) == 0)
	{
	    void* ret = n->data;
	    node_free(n);
	    libtnc_array_delete(&hash->array, i);
	    return ret;
	}
    }
    return NULL;
}

void*
libtnc_hash_deleteS(libtnc_hash* hash, char* key)
{
    return libtnc_hash_delete(hash, (TNC_BufferReference)key, (TNC_UInt32)strlen(key));
}
