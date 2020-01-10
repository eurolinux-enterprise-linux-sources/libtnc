// libtncconfig.h
//
// General configuration definitions for libtnc
// On Unix, uses the autoconfig config.h. On Windows, uses hardwired
// definitions
// Copyright (C) 2006 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtncimc.h,v 1.2 2006/05/24 04:54:27 mikem Exp mikem $

#ifdef WIN32
#include <windows.h>

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 0

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 0

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 0

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 0

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 0

/* Define if building under Windows */
#define HOST_IS_WINDOWS_VS2005 1

/* Define if building under Cygwin */
/* #undef HOST_IS_CYGWIN */

/* Define if building under Darwin on Mac OS X */
/* #undef HOST_IS_DARWIN */

/* enable internal debug messages */
/* #undef LIBTNCDEBUG */

/* Name of package */
#define PACKAGE "libtnc"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.19"

/* Define if using the dmalloc debugging malloc package */
/* #undef WITH_DMALLOC */

#else
#include <config.h>
#endif
