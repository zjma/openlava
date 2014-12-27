#!/bin/sh
set -x
rm -f config.cache
aclocal 
libtoolize --automake --copy --force
autoconf
autoheader
automake --add-missing --copy
#set +x
#test -n "$NOCONFIGURE" || ./configure "$@"
