// mutex.c
//
// Mutex code
//
// On Unix, requires pthreads
// On Windows, requires Windows mutex objects
//
// Copyright (C) 2007 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: mutex.c,v 1.1 2007/05/06 01:06:02 mikem Exp $

#include <libtncconfig.h>
#include <libtnc.h>

#if HAVE_PTHREAD
#include <pthread.h>
#include <errno.h>
#include <string.h>
static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

#elif defined WIN32
static int init_count = 0;
static HANDLE mutex1;
#endif


// Initialise the libtnc mutexes in this process
// Call at least once at the start of your program (multiple calls OK)
void libtnc_mutex_init()
{
#ifdef WIN32
    if (init_count++ == 0)
    {
	if ((mutex1 = CreateMutex(NULL, FALSE, NULL)) == NULL)
	{
	    libtnc_logMessage(TNC_LOG_SEVERITY_ERR, 
			      "CreateMutex failed: %d\n", GetLastError());
	}
    }
#endif	
}

// Lock the mutex
void libtnc_mutex_lock()
{
//    libtnc_logMessage(TNC_LOG_SEVERITY_DEBUG, "libtnc_mutex_lock\n");
#ifdef HAVE_PTHREAD
    if (pthread_mutex_lock(&mutex1) != 0)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, 
			  "pthread_mutex_lock failed: %s\n", strerror(errno));
    }
#elif defined WIN32
    if (WaitForSingleObject(mutex1, 1000L) != WAIT_OBJECT_0)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, 
			  "WaitForSingleObject failed: %d\n", GetLastError());
    }

#endif
}

// Unlock the mutex
void libtnc_mutex_unlock()
{
//    libtnc_logMessage(TNC_LOG_SEVERITY_DEBUG, "libtnc_mutex_unlock\n");
#ifdef HAVE_PTHREAD
    if (pthread_mutex_unlock(&mutex1) != 0)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, 
			  "pthread_mutex_unlock failed: %s\n", strerror(errno));
    }
#elif defined WIN32
    if (!ReleaseMutex(mutex1))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, 
			  "ReleaseMutex failed: %d\n", GetLastError());
    }

#endif
}

