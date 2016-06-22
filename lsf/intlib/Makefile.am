#
# Copyright (C) 2011-2014 David Bigagli
#

include $(top_srcdir)/common.mk

AM_CPPFLAGS += -I../
lib_LTLIBRARIES = liblsfint.la libtools.la
#noinst_PROGRAMS = treetest stest

liblsfint_la_SOURCES = admin.c cmdtime.c jidx.c lsftcl.c \
	resreq.c bitset.c conf.c list.c misc.c \
	userok.c window.c callex.c daemon.c listset.c \
	resourcecmd.c testbitset.c list2.c list2.h \
	bitset.h intlibout.h jidx.h list.h listset.h  \
	lsftcl.h resreq.h tokdefs.h yparse.h \
	link.c link.h listerr.def lsbitseterr.def \
	dlink.c dlink.h
liblsfint_la_LDFLAGS =  -no-undefined -version-info 0:1

libtools_la_SOURCES = tree.c tree.h list2.c list2.h \
	hash.c hash.h link.h link.c sshare.c sshare.h
libtools_la_LDFLAGS =  -no-undefined -version-info 0:1

#treetest_SOURCES = treetest.c
#treetest_LDADD = libtools.la link.o

#stest_SOURCES = stest.c
#stest_LDADD = libtools.la link.o
