lib_LTLIBRARIES = libosc_imc.la libosc_imv.la
libosc_imc_la_SOURCES = osc_imc.c
libosc_imv_la_SOURCES = osc_imv.c policy_tree.c policy.y policy_lexer.l
LIBS =  @LIBS@ `xml2-config --libs` ../libtnc/libtnc.la  @lt_cv_dlopen_libs@
check_PROGRAMS = test_osc
test_osc_SOURCES = test_osc.c
INCLUDES = `xml2-config --cflags` -I$(top_srcdir)/src/libtnc
EXTRA_DIST = policy_test.dat test_tnc_config test_tnc_config.cygwin test_tnc_config.darwin osc_imc.h osc_imv.h osc.h policy_tree.h
TESTS = $(check_PROGRAMS)
CLEANFILES = *~
BUILT_SOURCES = policy.h policy_lexer.c
AM_YFLAGS = -d
include $(top_srcdir)/Makefile.global
