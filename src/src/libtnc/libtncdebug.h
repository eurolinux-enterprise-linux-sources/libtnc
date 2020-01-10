// libtncdebug.c
//
// Utility module for debugging of libtnc
//
// Copyright (C) 2007 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id: libtncdebug.h,v 1.1 2007/02/13 08:09:20 mikem Exp mikem $

#ifdef LIBTNCDEBUG
#include <stdio.h>
#define DBG1(X1) { \
        fprintf(stderr,"\n%s:%i:"X1"\n", __FILE__,__LINE__ \
            ); }
#define DBG2(X1,X2) { \
        fprintf(stderr,"\n%s:%i:"X1"\n", __FILE__,__LINE__ \
            ,X2); }
#define DBG3(X1,X2,X3) { \
        fprintf(stderr,"\n%s:%i:"X1"\n", __FILE__,__LINE__ \
            ,X2,X3); }
#define DBG4(X1,X2,X3,X4) { \
        fprintf(stderr,"\n%s:%i:"X1"\n", __FILE__,__LINE__ \
            ,X2,X3,X4); }
#define DBG5(X1,X2,X3,X4,X5) { \
        fprintf(stderr,"\n%s:%i:"X1"\n", __FILE__,__LINE__ \
            ,X2,X3,X4,X5); }
#define DBG6(X1,X2,X3,X4,X5,X6) { \
        fprintf(stderr,"\n%s:%i:"X1"\n", __FILE__,__LINE__ \
            ,X2,X3,X4,X5,X6); }

#else
#define DBG1(X1) {}
#define DBG2(X1,X2) {}
#define DBG3(X1,X2,X3) {}
#define DBG4(X1,X2,X3,X4) {}
#define DBG5(X1,X2,X3,X4,X5) {}
#define DBG6(X1,X2,X3,X4,X5,X6) {}
#endif

#define DEBUG1(X1)                    DBG1("DEBUG: " X1)
#define DEBUG2(X1,X2)                 DBG2("DEBUG: " X1,X2)
#define DEBUG3(X1,X2,X3)              DBG3("DEBUG: " X1,X2,X3)
#define DEBUG4(X1,X2,X3,X4)           DBG4("DEBUG: " X1,X2,X3,X4)
#define DEBUG5(X1,X2,X3,X4,X5)        DBG5("DEBUG: " X1,X2,X3,X4,X5)
#define DEBUG6(X1,X2,X3,X4,X5,X6)     DBG6("DEBUG: " X1,X2,X3,X4,X5,X6)

#define HINT1(X1)                     DBG1("HINT: " X1)
#define HINT2(X1,X2)                  DBG2("HINT: " X1,X2)
#define HINT3(X1,X2,X3)               DBG3("HINT: " X1,X2,X3)
#define HINT4(X1,X2,X3,X4)            DBG4("HINT: " X1,X2,X3,X4)
#define HINT5(X1,X2,X3,X4,X5)         DBG5("HINT: " X1,X2,X3,X4,X5)
#define HINT6(X1,X2,X3,X4,X5,X6)      DBG6("HINT: " X1,X2,X3,X4,X5,X6)

#define NOTE1(X1)                     DBG1("NOTE: " X1)
#define NOTE2(X1,X2)                  DBG2("NOTE: " X1,X2)
#define NOTE3(X1,X2,X3)               DBG3("NOTE: " X1,X2,X3)
#define NOTE4(X1,X2,X3,X4)            DBG4("NOTE: " X1,X2,X3,X4)
#define NOTE5(X1,X2,X3,X4,X5)         DBG5("NOTE: " X1,X2,X3,X4,X5)
#define NOTE6(X1,X2,X3,X4,X5,X6)      DBG6("NOTE: " X1,X2,X3,X4,X5,X6)

#define WARN1(X1)                     DBG1("WARN: " X1)
#define WARN2(X1,X2)                  DBG2("WARN: " X1,X2)
#define WARN3(X1,X2,X3)               DBG3("WARN: " X1,X2,X3)
#define WARN4(X1,X2,X3,X4)            DBG4("WARN: " X1,X2,X3,X4)
#define WARN5(X1,X2,X3,X4,X5)         DBG5("WARN: " X1,X2,X3,X4,X5)
#define WARN6(X1,X2,X3,X4,X5,X6)      DBG6("WARN: " X1,X2,X3,X4,X5,X6)

#define FAIL1(X1)                     DBG1("FAIL: " X1)
#define FAIL2(X1,X2)                  DBG2("FAIL: " X1,X2)
#define FAIL3(X1,X2,X3)               DBG3("FAIL: " X1,X2,X3)
#define FAIL4(X1,X2,X3,X4)            DBG4("FAIL: " X1,X2,X3,X4)
#define FAIL5(X1,X2,X3,X4,X5)         DBG5("FAIL: " X1,X2,X3,X4,X5)
#define FAIL6(X1,X2,X3,X4,X5,X6)      DBG6("FAIL: " X1,X2,X3,X4,X5,X6)


