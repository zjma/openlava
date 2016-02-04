#
# Copyright (C) 2011-2016 David Bigagli
#
#

AM_CPPFLAGS = -I$(top_srcdir)/lsf -I$(top_srcdir)/lsbatch

SUBDIRS = docker
noinst_PROGRAMS = client gethostbyaddr gethostbyname \
                  hdr msg pimme qstring readevent server sobufsiz \
	 	  master

client_SOURCES = client.c proto.h
if SOLARIS
client_LDADD = -lsocket -lnsl
endif

gethostbyaddr_SOURCES = gethostbyaddr.c
if SOLARIS
gethostbyaddr_LDADD = -lsocket -lnsl
endif

gethostbyname_SOURCES = gethostbyname.c
if SOLARIS
gethostbyname_LDADD = -lsocket -lnsl
endif

hdr_SOURCES = hdr.c
msg_SOURCES = msg.c
pimme_SOURCES = pimme.c
qstring_SOURCES = qstring.c

readevent_SOURCES = readevent.c
readevent_LDADD = ../lsbatch/lib/.libs/liblsbatch.a ../lsf/lib/.libs/liblsf.a -lnsl
if SOLARIS
readevent_LDADD += -lsocket -lnsl
endif

server_SOURCES = server.c proto.h
if SOLARIS
server_LDADD = -lsocket -lnsl
endif
sobufsiz_SOURCES = sobufsiz.c
if SOLARIS
sobufsiz_LDADD = -lsocket -lnsl
endif

master_SOURCES = master.c
master_LDADD = ../lsf/lib/.libs/liblsf.a -lnsl


