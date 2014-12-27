#
# Copyright (C) 2011-2014 David Bigagli
#

include $(top_srcdir)/common.mk

AM_CPPFLAGS += -I../
lib_LTLIBRARIES = liblsfint.la

liblsfint_la_SOURCES = admin.c cmdtime.c jidx.c lsftcl.c \
	resreq.c bitset.c conf.c list.c misc.c \
	userok.c window.c callex.c daemon.c listset.c \
	resourcecmd.c testbitset.c list2.c link.c \
	bitset.h intlibout.h jidx.h list.h listset.h  \
	lsftcl.h resreq.h tokdefs.h yparse.h \
	listerr.def lsbitseterr.def list2.h link.h \
	tree.c tree.h hash.c hash.h
liblsfint_la_LDFLAGS =  -no-undefined -version-info 0:1
