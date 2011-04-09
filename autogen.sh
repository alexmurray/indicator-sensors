#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="indicator-sensors"

(test -f $srcdir/configure.ac) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
	echo -n "ERROR: gnome-autogen.sh not found in path: "
	echo "Please install gnome-common before running this script"
	exit 1
}

USE_GNOME2_MACROS=1 . gnome-autogen.sh
