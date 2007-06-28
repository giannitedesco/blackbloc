#!/bin/sh
aclocal
autoheader
automake --gnu -a -c -v
autoconf
test -x ./configure && ./configure $@
