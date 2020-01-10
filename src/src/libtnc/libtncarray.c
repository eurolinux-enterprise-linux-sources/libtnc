// libtncarray.h
//
// Dynamic array code
// Copyright (C) 2007 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtncarray.c,v 1.1 2009/03/18 11:28:35 mikem Exp mikem $

#include <libtncconfig.h>
#include "libtncarray.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TNC_UInt32 libtnc_array_init(libtnc_array* array)
{
    array->data = NULL;
    array->size = 0;
    array->max = 0;
    return 1;
}

TNC_UInt32 libtnc_array_free(libtnc_array* array)
{
    free(array->data);
    return libtnc_array_init(array);
}

TNC_UInt32 libtnc_array_resize(libtnc_array* array, TNC_UInt32 size)
{
    if (array->size < size)
    {
	void** new = calloc(size, sizeof(void*));
	if (new)
	{
	    memcpy(new, array->data, array->size * sizeof(void*));
	    free(array->data);
	    array->data = new;
	    array->size = size;
	}
    }
    return size;
}

TNC_UInt32 libtnc_array_push(libtnc_array* array, void* data)
{
    TNC_UInt32 ret;

    if (array->max >= array->size)
    {
	libtnc_array_resize(array, array->size + LIBTNC_ARRAY_SIZE_INCREMENT);
    }
    ret = array->max++;
    array->data[ret] = data;
    return ret;
}

void* libtnc_array_pop(libtnc_array* array)
{
    if (array->max > 0)
	return array->data[--array->max];
    else
	return NULL;
}

void* libtnc_array_delete(libtnc_array* array, TNC_UInt32 index)
{
    if (index < array->max)
    {
	void* ret;
	ret = array->data[index];
	array->data[index] = NULL;
	return ret;
    }
    else
	return NULL;
    
}

void* libtnc_array_index(libtnc_array* array, TNC_UInt32 index)
{
    if (index < array->max)
	return array->data[index];
    else
	return NULL;
    
}

TNC_UInt32 libtnc_array_new(libtnc_array* array, void* data)
{
    TNC_UInt32 i;

    // First look for an empty slot
    for (i = 0; i < array->max; i++)
    {
	if (array->data[i] == NULL)
	{
	    // Empty
	    array->data[i] = data;
	    return i;
	}
    }
    // Non empty, push ontop the end
    return libtnc_array_push(array, data);
}

TNC_UInt32 libtnc_array_size(libtnc_array* array)
{
    return array->max;
}
