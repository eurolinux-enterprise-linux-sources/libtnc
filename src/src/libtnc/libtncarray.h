// libtncarray.h
//
// Dynamic array code
// Copyright (C) 2007 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtncarray.h,v 1.1 2009/03/18 11:28:35 mikem Exp mikem $

#ifndef _ARRAY_H
#define _ARRAY_H
#ifdef __cplusplus
extern "C" {
#endif

#include <tncif.h>

// How much to enlarge the array by
#define LIBTNC_ARRAY_SIZE_INCREMENT 100

// Holds data for a dynamically sized array
typedef struct
{
    void**         data;   // Allocated data
    TNC_UInt32     size;   // Size of allocated data in slots
    TNC_UInt32     max;    // Number of items used, index of the next slot
} libtnc_array;

// Initialiser for arrays
#define    LIBTNC_ARRAY_INIT {0, 0, 0}


// Initialise a new array and set an initial size of 0
// Returns 1
extern TNC_UInt32 libtnc_array_init(libtnc_array* array);

// Free any storage associated with the array, and clear it back to an empty state
// Returns 1
extern TNC_UInt32 libtnc_array_free(libtnc_array* array);

// Enlarge the array to be at least as large as size
// Return the new size or 0 if failure
extern TNC_UInt32 libtnc_array_resize(libtnc_array* array, TNC_UInt32 size);

// Push a value onto the end of the array and returns its index
extern TNC_UInt32 libtnc_array_push(libtnc_array* array, void* data);

// Remove a value from the end of the array
// Return NULL if empty
extern void* libtnc_array_pop(libtnc_array* array);

// Delete an item from the array (making it available to libtnc_array_insert
// Return the item previsously at that index
extern void* libtnc_array_delete(libtnc_array* array, TNC_UInt32 index);

// Get the data item at the given index
extern void* libtnc_array_index(libtnc_array* array, TNC_UInt32 index);

// Find the first available empty slot and allocate it to this data item
extern TNC_UInt32 libtnc_array_new(libtnc_array* array, void* data);

#ifdef __cplusplus
}
#endif
#endif
