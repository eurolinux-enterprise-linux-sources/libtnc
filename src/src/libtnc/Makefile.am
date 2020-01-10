lib_LTLIBRARIES = libtnc.la
libtnc_la_SOURCES = libtncxml.c libtnctncc.c libtnctncs.c libtncimc.c libtncimv.c libtnc_log.c base64.c mutex.c tncc_receive.c tncs_receive.c tncs_attributes.c tncc_bind.c tncs_bind.c libtncarray.c libtnchash.c
include_HEADERS = libtncdebug.h libtncxml.h libtnctncc.h libtnctncs.h libtnc.h libtncimc.h libtncimv.h tncif.h tncifimc.h tncifimv.h libtncarray.h libtnchash.h 
LDADD = $(top_builddir)/src/sample/libsample_imc.la  $(top_builddir)/src/sample/libsample_imv.la -L. .libs/libtnc.a
check_PROGRAMS = test_static test_dynamic test_tnccs
test_static_SOURCES = test_static.c
test_dynamic_SOURCES = test_dynamic.c
test_tnccs_SOURCES = test_tnccs.c
INCLUDES = `xml2-config --cflags`
LIBS = @LIBS@ `xml2-config --libs` @lt_cv_dlopen_libs@
EXTRA_DIST = test_tnc_config test_tnc_config.cygwin test_tnc_config.darwin base64.h mutex.h
TESTS = $(check_PROGRAMS)
CLEANFILES = *~
include $(top_srcdir)/Makefile.global
