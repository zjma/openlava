#
# Copyright (C) 2011-2015 David Bigagli
#

include $(top_srcdir)/common.mk

AM_CPPFLAGS += -I$(top_srcdir)/lsf  -I$(top_srcdir)/lsf/lib \
           -I$(top_srcdir)/lsbatch  -I$(top_srcdir)/lsbatch/lib -I./

sbin_PROGRAMS = mbatchd sbatchd
mbatchd_SOURCES  = \
mbd.comm.c mbd.host.c mbd.jgrp.c mbd.main.c mbd.proxy.c mbd.resource.c \
mbd.dep.c mbd.init.c mbd.job.c mbd.misc.c mbd.queue.c mbd.serv.c \
mbd.policy.c mbd.grp.c mbd.jarray.c mbd.log.c mbd.requeue.c mbd.window.c \
elock.c misc.c mail.c daemons.c daemons.xdr.c \
mbd.h daemonout.h daemons.h jgrp.h proxy.h mbd.profcnt.def 

mbatchd_LDADD = ../lib/.libs/liblsbatch.a \
                ../../lsf/lib/.libs/liblsf.a \
                ../../lsf/intlib/.libs/liblsfint.a \
		../../lsf/intlib/.libs/libtools.a -lm -lnsl
if !CYGWIN
mbatchd_LDADD += -lnsl
endif

sbatchd_SOURCES = sbd.comm.c sbd.file.c sbd.job.c sbd.main.c \
                  sbd.misc.c sbd.policy.c sbd.serv.c sbd.sig.c sbd.xdr.c \
                  elock.c mail.c misc.c daemons.c daemons.xdr.c \
                  sbd.h daemonout.h daemons.h 

sbatchd_LDADD = ../lib/.libs/liblsbatch.a \
                ../../lsf/lib/.libs/liblsf.a \
                ../../lsf/intlib/.libs/liblsfint.a -lm -lnsl
if !CYGWIN
sbatchd_LDADD += -lnsl
endif
if SOLARIS
mbatchd_LDADD += -lsocket -lnsl
sbatchd_LDADD += -lsocket -lnsl
endif
if HWLOC
sbatchd_LDADD += -lhwloc -lnuma
sbatchd_SOURCES += sbd.numa.c
endif

lib_LTLIBRARIES = libfairshare.la libpreempt.la 
libfairshare_la_SOURCES = libfairshare.c fairshare.h 
libfairshare_la_LDFLAGS = -no-undefined -version-info 0:1
libfairshare_la_LIBADD = $(top_builddir)/lsf/intlib/libtools.la \
	$(top_builddir)/lsf/lib/liblsf.la

libpreempt_la_SOURCES = libpreempt.c preempt.h
libpreempt_la_LDFLAGS = -no-undefined -version-info 0:1
libpreempt_la_LIBADD =  $(top_builddir)/lsf/intlib/libtools.la \
	$(top_builddir)/lsf/lib/liblsf.la $(top_builddir)/lsbatch/lib/liblsbatch.la
