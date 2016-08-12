#
# Copyright (C) 2011-2015 David Bigagli
#

include $(top_srcdir)/common.mk

AM_CPPFLAGS += -I$(top_srcdir)/lsf

sbin_PROGRAMS = res nios
res_SOURCES  = \
res.c res.handler.c res.misc.c res.rf.c res.init.c \
res.pty.c res.tasklog.c rescom.h res.h resout.h
res_LDADD = ../lib/.libs/liblsf.a ../intlib/.libs/liblsfint.a -lm
if !CYGWIN
res_LDADD += -lnsl
endif

res_LDADD += ../lib/.libs/libazure.a


nios_SOURCES = nios.c nios.handler.c nios.h
#
# yep.. true
#
nios_LDADD = ../../lsbatch/lib/.libs/liblsbatch.a \
	../lib/.libs/liblsf.a ../intlib/.libs/liblsfint.a -lm
if !CYGWIN
nios_LDADD += -lnsl
endif
if SOLARIS
nios_LDADD += -lsocket
res_LDADD += -lsocket
endif

