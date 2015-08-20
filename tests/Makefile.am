#
# Copyright (C) 2015 David Bigagli
#

noinst_PROGRAMS = tests
AM_CFLAGS=-I ../lsf
tests_SOURCES = tests.c libtests.c tests.h
tests_LDADD = ../lsbatch/lib/liblsbatch.la ../lsf/lib/liblsf.la \
	../lsf/intlib/liblsfint.la -lm -lnsl
