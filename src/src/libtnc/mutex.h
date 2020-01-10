// mutex.h
//
// Mutex code
// Copyright (C) 2007 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: mutex.h,v 1.1 2007/05/06 01:06:02 mikem Exp $

#ifndef _MUTEX_H
#define _MUTEX_H
#ifdef __cplusplus
extern "C" {
#endif

// Initialise the libtnc mutexes in this process
// Call at least once at the start of your program (multiple calls OK)
extern void libtnc_mutex_init();

// Lock the mutex
extern void libtnc_mutex_lock();

// Unlock the mutex
extern void libtnc_mutex_unlock();

#ifdef __cplusplus
}
#endif
#endif
