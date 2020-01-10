// libtncimc.c
//
// Utility module for loading and using IMC modules
//
// The caller is required to define these functions which may be
// called by any IMC
//  TNC_TNCC_SendMessage
//  TNC_TNCC_RequestHandshakeRetry
//
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtncimc.c,v 1.5 2009/03/18 11:28:35 mikem Exp mikem $

#include <libtncconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include <libtncimc.h>
#include <libtncdebug.h>
#include <mutex.h>

// Array of initialised libtnc_imc structures, appended to the array
// and indexed by their tmcID. This is ugly, and should really be able 
// to grow without bound but it will do for the meantime.
// They are protected by a mutex for thread safety
static TNC_IMCID nextTmcImcID = 0;
static libtnc_imc* imcs[NUM_IMCS];

///////////////////////////////////////////////////////////
// Given an IMC ID, return pointer to the corresponding libtnc_imc
static libtnc_imc* get_imc(
    /*in*/ TNC_IMCID imcID)
{
    libtnc_imc* ret = NULL;

    libtnc_mutex_lock();
    if (imcID >= 0 && imcID < NUM_IMCS)
	ret = imcs[imcID];
    libtnc_mutex_unlock();
    return ret;
}


///////////////////////////////////////////////////////////
// Given an IMC ID save the pointer for recovery later by get_imc
static void save_imc(
    /*in*/ libtnc_imc* self)
{
    libtnc_mutex_lock();
    self->imcID = nextTmcImcID++;
    imcs[self->imcID] = self;
    libtnc_mutex_unlock();
}

///////////////////////////////////////////////////////////
// Given an IMC remove the pointer for recovery later by get_imc
static void unsave_imc(
    /*in*/ libtnc_imc* self)
{
    libtnc_mutex_lock();
    if (self->imcID != -1)
	imcs[self->imcID] = NULL;
    libtnc_mutex_unlock();
}

///////////////////////////////////////////////////////////
// These may be called from within the IMC
TNC_Result TNC_TNCC_ReportMessageTypes(
    /*in*/ TNC_IMCID imcID,
    /*in*/ TNC_MessageTypeList supportedTypes,
    /*in*/ TNC_UInt32 typeCount)
{
    libtnc_imc* self;

    DEBUG3("TNC_TNCC_ReportMessageTypes %d %d", imcID, typeCount);
    // Find the structure for this IMC
    self = get_imc(imcID);
    if (!(self = get_imc(imcID)))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "TNC_TNCC_ReportMessageTypes could not find libtnc_imc for IMC ID %d\n", imcID);
	return TNC_RESULT_INVALID_PARAMETER;
    }

    // IMC may call this multiple times, so free any previous copy
    if (self->supportedTypes)
    {
	free(self->supportedTypes);
	self->supportedTypes = NULL;
    }

    // Remember the message types so we can correctly route message
    // to some or all IMCs
    self->typeCount = typeCount;
    if (typeCount)
    {
	int size = typeCount * sizeof(TNC_MessageType);
	if (!(self->supportedTypes = (void*)malloc(size)))

	{
	    libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			      "TNC_TNCC_ReportMessageTypes could not duplicate %d message types: %s\n", typeCount, strerror(errno));
	    return TNC_RESULT_FATAL;
	}
	// Save the supported types
	memcpy(self->supportedTypes, supportedTypes, size);
    }

    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Resolve function pointers
// Called by the IMC when it needs the address of a functions in here
// If the function is not known at this level call into the application to 
// see if it knows about it
TNC_Result TNC_TNCC_BindFunction(
    TNC_IMCID imcID,
    char *functionName,
    void **pOutfunctionPointer)
{
    DEBUG3("TNC_TNCC_BindFunction %d %s", imcID, functionName);

    // See if the calling application knowns about it
    if (libtnc_tncc_BindFunction(imcID, functionName, pOutfunctionPointer) 
	== TNC_RESULT_SUCCESS)
	return TNC_RESULT_SUCCESS;
    else if (!strcmp(functionName, "TNC_TNCC_ReportMessageTypes"))
	*pOutfunctionPointer = (void*)TNC_TNCC_ReportMessageTypes;
    else if (!strcmp(functionName, "TNC_TNCC_RequestHandshakeRetry"))
	*pOutfunctionPointer = (void*)TNC_TNCC_RequestHandshakeRetry;
    else if (!strcmp(functionName, "TNC_TNCC_SendMessage"))
	*pOutfunctionPointer = (void*)TNC_TNCC_SendMessage;
    else if (!strcmp(functionName, "TNC_9048_LogMessage"))
	*pOutfunctionPointer = (void*)libtnc_logMessage;
    else
	return TNC_RESULT_INVALID_PARAMETER;
    return TNC_RESULT_SUCCESS;
}

#ifdef WIN32
///////////////////////////////////////////////////////////
// \brief Given the name of a child key in the registry, pull out the data that is in it, and
//        load the IMC in to memory.
//
// @param[in] key   The KEY root to use for this query.
// @param[in] name   The name to add to the path to get the IMC data we need to load an IMC.
//
// \retval 0 on success
// \retval -1 on error
int load_imc_from_reg(HKEY key, char *name)
{
    LONG   retval        = 0;
    char   *basepath     = TNC_CONFIG_REGISTRY_KEY_IMC;
    char   *completepath = NULL;
    HKEY   phk = NULL;
    DWORD  mytype;
    DWORD  mylen         = TNC_MAX_REGISTRY_VALUE_LEN;
    LPVOID mydata[TNC_MAX_REGISTRY_VALUE_LEN];
    int    loaded        = 0;
    
    completepath = calloc(strlen(basepath) + strlen(name) + 2, 1);
    if (completepath == NULL)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "Couldn't allocate memory to store path to search for IMC '%s'.\n", name);
	return 0;
    }
    
    sprintf(completepath, "%s%s", basepath, name);
    if ((retval = RegOpenKeyEx(key, completepath, 0, KEY_READ, &phk)) != ERROR_SUCCESS)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "Failed to open key '%s'. (Error : %ld)\n", completepath, retval);
	return 0;
    }
    
    if (RegQueryValueEx(phk, "Path", 0, &mytype, (LPBYTE)&mydata, &mylen) != ERROR_SUCCESS)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "Couldn't get Path for IMC!\n");
    }
    else
    {
	if (libtnc_imc_new((char *)mydata))
	    loaded = 1;
    }
    
    RegCloseKey(phk);
    return loaded;
}

///////////////////////////////////////////////////////////
// Iterate over all the IMCs named in the registry and load each one
// Return the number of successfuly loaded modules
int load_imcs_from_key(HKEY key)
{
    HKEY     phk      = NULL;
    DWORD    i; 
    DWORD    nlen;
    char     name[TNC_MAX_REGISTRY_NAME_LEN];
    FILETIME mytime;
    LONG     retval   = 0;
    
    if ((retval = RegOpenKeyEx(key, TNC_CONFIG_REGISTRY_KEY_IMC, 
			       0, KEY_READ, &phk)) != ERROR_SUCCESS)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "Failed to open key %s. (Error : %ld)\n", TNC_CONFIG_REGISTRY_KEY_IMC, retval);
	return 0;
    }
    
    for (i = 0, nlen = TNC_MAX_REGISTRY_NAME_LEN;
	 (retval = RegEnumKeyEx(phk, i, (LPSTR)&name, &nlen, NULL, NULL, NULL, &mytime)) == ERROR_SUCCESS;
	 i++)
	
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR, "Registry key '%s' found.\n", name);
	if (load_imc_from_reg(key, name) > 0) 
	    retval++;
    }
    
    RegCloseKey(phk);
    return retval;
}
#endif

///////////////////////////////////////////////////////////
// Load the IMCs named in a config file in the format specified by IF-IMC
int libtnc_imc_load_config(
    /*in*/ const char* filename)
{
    FILE* f;

    DEBUG2("libtnc_imc_load_config %s", filename);
    if (f = fopen(filename, "r"))
    {
	int   good_load_count = 0;
	char  buf[1000];
	while (fgets(buf, 1000, f))
	{
	    char* name = NULL;
	    char* path = NULL;
	    char* p = buf;
	    
	    if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0'; // Clear trailing newlines
	    if (buf[0] == '\0' || buf[0] == '#')
		continue; // Skip empty and comment lines

	    if (strncmp(p, "IMC \"", 5) == 0)
	    {
		p += 5;
		name = p;
		while (*p && *p != '"')
		    p++;
		if (!*p)
		    continue;
		*p++ ='\0';
		while (*p && isspace(*p))
		    p++;
		if (!*p)
		    continue;
		path = p;
		if (libtnc_imc_new(path))
		    good_load_count++;
	    }
	}
	fclose(f);
	return good_load_count;
    }
    else
	return -1;
}

///////////////////////////////////////////////////////////
// Load IMCs named in the standard place for this platform
// On Unix, we look in /etc/tnc_config
// On Windows, and we look for the well known registry keys.
// Return the number of modules successfully loaded
int libtnc_imc_load_std_config()
{
    int retval = 0;

    DEBUG1("libtnc_imc_load_std_config");
#ifdef WIN32
    retval = load_imcs_from_key(HKEY_LOCAL_MACHINE);
    if (retval < 0) 
	retval = 0;
    retval += load_imcs_from_key(HKEY_CURRENT_USER);
#else
    retval = libtnc_imc_load_config(TNC_CONFIG_FILE_PATH);
#endif
    return retval;
}

///////////////////////////////////////////////////////////
// Load 0 or more IMC modules
// If any fail to load, return TNC_RESULT_FATAL
// If all load OK, return TNC_RESULT_SUCCESS
TNC_Result libtnc_imc_load_modules(
    /*in*/ const char* filenames[],
    /*in*/ TNC_UInt32 numfiles)
{
    TNC_UInt32 i;

    DEBUG2("libtnc_imc_load_modules %d", numfiles);
    for (i = 0; i < numfiles; i++)
    {
	if (!libtnc_imc_new(filenames[i]))
	    return TNC_RESULT_FATAL;
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Create a new libtnc_imc and load the named IMC module,
// resolve pointers to the public functions in the module.
libtnc_imc* libtnc_imc_new(
    /*in*/ const char* filename)
{
    libtnc_imc* self;

    DEBUG2("libtnc_imc_new %s", filename);
    libtnc_mutex_init();
    if (!(self = (libtnc_imc*)calloc(1, sizeof(libtnc_imc))))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could calloc libtnc_imc: %s\n", strerror(errno));
	return NULL;
    }

#ifdef WIN32
    if (!(self->dl = (void*)LoadLibrary(filename)))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could not LoadLibrary %s\n", filename);
	libtnc_imc_destroy(self);
	return NULL;
    }
    
    self->filename = _strdup(filename);
    self->imcID = -1;
    
    
    if (!(self->intialiseP = (TNC_IMC_InitializePointer)GetProcAddress(self->dl, "TNC_IMC_Initialize")))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could not resolve TNC_IMC_Initialize in %s!\n", filename);
	libtnc_imc_destroy(self);
	return NULL;
    }
    
    self->notifyConnectionChangeP = (TNC_IMC_NotifyConnectionChangePointer)GetProcAddress(self->dl, "TNC_IMC_NotifyConnectionChange");
    
    if (!(self->beginHandshakeP = (TNC_IMC_BeginHandshakePointer)GetProcAddress(self->dl, "TNC_IMC_BeginHandshake")))
	{
	    libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			      "libtnc_imc_new could not resolve TNC_IMC_BeginHandshake in %s!\n", filename);
	    libtnc_imc_destroy(self);
	    return NULL;
	}

    self->receiveMessageP = (TNC_IMC_ReceiveMessagePointer)GetProcAddress(self->dl, "TNC_IMC_ReceiveMessage");
    self->batchEndingP    = (TNC_IMC_BatchEndingPointer)GetProcAddress(self->dl, "TNC_IMC_BatchEnding");
    self->terminateP      = (TNC_IMC_TerminatePointer)GetProcAddress(self->dl, "TNC_IMC_Terminate");
    
    if (!(self->provideBindFunctionP = (TNC_IMC_ProvideBindFunctionPointer)GetProcAddress(self->dl, "TNC_IMC_ProvideBindFunction")))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could not resolve TNC_IMC_ProvideBindFunction in %s!\n", filename);
	libtnc_imc_destroy(self);
	return NULL;
    }
#else
    // Unix/Linux etc
    if (!(self->dl = dlopen(filename, RTLD_NOW)))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could not dlopen %s: %s\n", filename, dlerror());
	libtnc_imc_destroy(self);
	return NULL;
    }

    self->filename = strdup(filename);
    self->imcID = -1;

    // Get pointers to public functions inside the module
    // Only some of them are mandatory, bail out if they are not available
    if (!(self->intialiseP = dlsym(self->dl, "TNC_IMC_Initialize")))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could not resolve TNC_IMC_Initialize in %s: %s\n", filename, dlerror());
	libtnc_imc_destroy(self);
	return NULL;
    }
    self->notifyConnectionChangeP 
	= dlsym(self->dl, "TNC_IMC_NotifyConnectionChange");
    if (!(self->beginHandshakeP = dlsym(self->dl, "TNC_IMC_BeginHandshake")))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could not resolve TNC_IMC_BeginHandshake in %s: %s\n", filename, dlerror());
	libtnc_imc_destroy(self);
	return NULL;
    }
    self->receiveMessageP = dlsym(self->dl, "TNC_IMC_ReceiveMessage");
    self->batchEndingP    = dlsym(self->dl, "TNC_IMC_BatchEnding");
    self->terminateP      = dlsym(self->dl, "TNC_IMC_Terminate");
    if (!(self->provideBindFunctionP = dlsym(self->dl, "TNC_IMC_ProvideBindFunction")))
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could not resolve TNC_IMC_ProvideBindFunction in %s: %s\n", filename, dlerror());
	libtnc_imc_destroy(self);
	return NULL;
    }
#endif

    // Initialise the module
    if ((*self->intialiseP)(nextTmcImcID, TNC_IFIMC_VERSION_1, TNC_IFIMC_VERSION_1, 
			    &self->version) != TNC_RESULT_SUCCESS)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could not Initialize '%s'\n", filename);
	libtnc_imc_destroy(self);
	return NULL;
    }

    // Make sure we can recover it later given just the ID
    save_imc(self);

    // Tell the module about our bind function, and let it resolve the 
    // functions it needs by calling our TNC_TNCC_BindFunction
    if ((*self->provideBindFunctionP)(self->imcID, TNC_TNCC_BindFunction) 
	!= TNC_RESULT_SUCCESS)
    {
	libtnc_logMessage(TNC_LOG_SEVERITY_ERR,
			  "libtnc_imc_new could ProvideBindFunction '%s'\n", filename);
	libtnc_imc_destroy(self);
	return NULL;
    }

    return self;
}

///////////////////////////////////////////////////////////
TNC_Result libtnc_imc_destroy(
    /*in*/ libtnc_imc* self)
{
    DEBUG1("libtnc_imc_destroy");
    if (!self)
	return TNC_RESULT_INVALID_PARAMETER;

    // Terminate the module
    unsave_imc(self);

#ifdef WIN32
	// Unload the shared library
	if (self->dl)
		FreeLibrary(self->dl);
#else
    // Unload the shared library
    if (self->dl)
	dlclose(self->dl);
#endif
    
    // Recover memory
    free(self->filename);
    free(self->supportedTypes);
    free(self);

    return TNC_RESULT_SUCCESS;
}


///////////////////////////////////////////////////////////
// Tell all IMCs about NotifyConnectionChange
TNC_Result libtnc_imc_NotifyConnectionChange(
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_ConnectionState newState)
{
    TNC_IMCID i;

    DEBUG3("libtnc_imc_NotifyConnectionChange %d %d", connectionID, newState);
    for (i = 0; i < nextTmcImcID; i++)
    {
	libtnc_imc* self = imcs[i];
	if (!self) continue; // IMC not there any more
	if (self->notifyConnectionChangeP)
	    (*self->notifyConnectionChangeP)(self->imcID, connectionID, newState);
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result libtnc_imc_BeginHandshake(
    /*in*/ TNC_ConnectionID connectionID)
{
    TNC_IMCID i;

    DEBUG2("libtnc_imc_BeginHandshake %d", connectionID);
    for (i = 0; i < nextTmcImcID; i++)
    {
	libtnc_imc* self = imcs[i];
	if (!self) continue; // IMC not there any more
//	printf("IMC ID : %d\n", i);
	if (self->beginHandshakeP)
	    (*self->beginHandshakeP)(self->imcID, connectionID);
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
// Send the message to all IMCs that have registered interest
// in this message type
TNC_Result libtnc_imc_ReceiveMessage(
    /*in*/ TNC_ConnectionID connectionID,
    /*in*/ TNC_BufferReference messageBuffer,
    /*in*/ TNC_UInt32 messageLength,
    /*in*/ TNC_MessageType messageType)
{
    // Info about this message type
    TNC_MessageSubtype mt = messageType        & TNC_SUBTYPE_ANY;
    TNC_VendorID       mv = (messageType >> 8) & TNC_VENDORID_ANY;
    TNC_IMCID i;

    DEBUG2("libtnc_imc_ReceiveMessage %d", connectionID);
    // Prevent illegal message types being delivered
    if (   mt == TNC_SUBTYPE_ANY
	|| mv == TNC_VENDORID_ANY)
	return TNC_RESULT_INVALID_PARAMETER;

    for (i = 0; i < nextTmcImcID; i++)
    {
	TNC_UInt32 j;
	libtnc_imc* self = imcs[i];
	if (!self) continue; // IMC not there any more

	// Is there a delivery function?
	if (!self->receiveMessageP) continue; // Cant deliver anyway
	    
	// See if this IMC wants this type of message
	for (j = 0; j < self->typeCount; j++)
	{
	    // Info about this supported message type
	    TNC_MessageSubtype smt = self->supportedTypes[j] & TNC_SUBTYPE_ANY;
	    TNC_VendorID       smv = (self->supportedTypes[j] >> 8)&TNC_VENDORID_ANY;

	    // Send if it matches at least one supported message type
	    if (self->supportedTypes[j] == messageType
		|| (smt == TNC_SUBTYPE_ANY 
		    && (mv == smv || smv == TNC_VENDORID_ANY))
		|| (smv == TNC_VENDORID_ANY 
		    && (mt == smt || smt == TNC_SUBTYPE_ANY)))
	    {
		(*self->receiveMessageP)
		    (self->imcID, connectionID, messageBuffer, messageLength, messageType);
		break;
	    }
	}
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result libtnc_imc_BatchEnding(
    /*in*/ TNC_ConnectionID connectionID)
{
    TNC_IMCID i;

    DEBUG2("libtnc_imc_BatchEnding %d", connectionID);
    for (i = 0; i < nextTmcImcID; i++)
    {
	libtnc_imc* self = imcs[i];
	if (!self) continue; // IMC not there any more
	if (self->batchEndingP)
	    (*self->batchEndingP)(self->imcID, connectionID);
    }
    return TNC_RESULT_SUCCESS;
}

///////////////////////////////////////////////////////////
TNC_Result libtnc_imc_Terminate()
{
    TNC_IMCID i;

    DEBUG1("libtnc_imc_Terminate");
    for (i = 0; i < nextTmcImcID; i++)
    {
	libtnc_imc* self = imcs[i];
	if (!self) continue; // IMC not there any more
	if (self->terminateP)
	    (*self->terminateP)(self->imcID);
	libtnc_imc_destroy(self);
    }
    nextTmcImcID = 0; // Can start renumbering
    return TNC_RESULT_SUCCESS;
}

