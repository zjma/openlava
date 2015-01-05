#
# Copyright (C) 2011-2015 David Bigagli
#

include $(top_srcdir)/common.mk

AM_CPPFLAGS += -I$(top_srcdir)/lsf

sbin_PROGRAMS = eauth
eauth_SOURCES = eauth.c
eauth_LDADD = ../lsf/lib/liblsf.la ../lsf/intlib/liblsfint.la -lnsl
if SOLARIS
eauth_LDADD += -lsocket -lnsl
endif

