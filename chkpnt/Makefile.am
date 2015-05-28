#
# Copyright (C) 20111-2015 David Bigagli
#

include $(top_srcdir)/common.mk

AM_CPPFLAGS += -I../lsf

sbin_PROGRAMS = echkpnt erestart

echkpnt_SOURCES = echkpnt.c echkpnt.env.c echkpnt.lib.c \
	echkpnt.lib.h echkpnt.env.h
echkpnt_LDADD = ../lsf/lib/.libs/liblsf.a -lnsl

erestart_SOURCES = erestart.c echkpnt.env.c echkpnt.lib.c \
	echkpnt.lib.h echkpnt.env.h
erestart_LDADD = ../lsf/lib/.libs/liblsf.a -lnsl
if SOLARIS
echkpnt_LDADD += -lsocket -lnsl
erestart_LDADD += -lsocket -lnsl
endif

