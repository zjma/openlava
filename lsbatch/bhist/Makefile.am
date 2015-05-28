#
# Copyright (C) 2011-2014 David Bigagli
#

include $(top_srcdir)/common.mk

AM_CPPFLAGS += -I$(top_srcdir)/lsf  -I$(top_srcdir)/lsf/lib \
           -I$(top_srcdir)/lsbatch  -I$(top_srcdir)/lsbatch/lib -I./

bin_PROGRAMS = bhist
bhist_SOURCES  = bhist.c read.event.c bhist.h
bhist_LDADD = ../cmd/cmd.job.o \
	../cmd/cmd.misc.o ../cmd/cmd.jobid.o ../cmd/cmd.prt.o \
	../cmd/cmd.err.o ../cmd/cmd.hist.o \
	../lib/.libs/liblsbatch.a \
	../../lsf/lib/.libs/liblsf.a \
	../../lsf/intlib/.libs/liblsfint.a -lm -lnsl
if !CYGWIN
bhist_LDADD += -lnsl
endif
if SOLARIS
bhist_LDADD += -lsocket -lnsl
endif

