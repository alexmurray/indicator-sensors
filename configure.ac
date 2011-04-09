# ================= initialization =================== #

AC_INIT([Indicator Sensors], [0.1], [murray.alex@gmail.com],
[indicator-sensors])

AC_CONFIG_SRCDIR([src/indicator-sensors.c])
AM_CONFIG_HEADER([src/config.h])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

# ============== basic compiler settings ============= #
AC_PROG_CC
AC_HEADER_STDC

# =================== for plugins ==================== #
AC_PROG_LIBTOOL

# =========== take care of some localization ========= #
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=indicator-sensors
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL

# ========== export compiler / linker options ======== #
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(LIBS)

# ============== look for dependencies =============== #
# check for headers needed for standard interfaces
#AC_CHECK_HEADERS()

GLIB_REQUIRED=2.24.0
GTK_REQUIRED=2.24.0
APPINDICATOR_REQUIRED=0.0.7
LIBPEAS_REQUIRED=0.7.2

PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_REQUIRED)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(APPINDICATOR, appindicator-0.1 >= $APPINDICATOR_REQUIRED)
AC_SUBST(APPINDICATOR_CFLAGS)
AC_SUBST(APPINDICATOR_LIBS)

PKG_CHECK_MODULES(LIBPEAS, libpeas-1.0 >= $LIBPEAS_REQUIRED)
AC_SUBST(LIBPEAS_CFLAGS)
AC_SUBST(LIBPEAS_LIBS)

AC_CHECK_HEADERS(regex.h)
AC_CHECK_HEADERS(sensors/sensors.h,
  AC_CHECK_LIB(sensors, sensors_init,[
  AC_DEFINE(HAVE_LIBSENSORS,1,[libsensors is available])
  if test "x${SENSORS_DIR}" != "x"; then
          LIBSENSORS_CFLAGS="-I${SENSORS_DIR}/include"
          LIBSENSORS_LIBS="-L${SENSORS_DIR}/lib -lsensors"
  else
          LIBSENSORS_LIBS="-lsensors"
  fi]))

AM_CONDITIONAL(LIBSENSORS, test -n "$LIBSENSORS_LIBS")

AC_SUBST(LIBSENSORS_CFLAGS)
AC_SUBST(LIBSENSORS_LIBS)

# do nvidia by default if available
AC_ARG_WITH(nvidia,
[  --with-nvidia[=DIR]   use nvidia in DIR],[
        if test "$withval" != "no" -a "$withval" != "yes"; then
                NVIDIA_DIR=$withval
                CFLAGS="${CFLAGS} -I${NVIDIA_DIR}/include"
                LIBS="${LIBS} -L${NVIDIA_DIR}/lib"
        fi
	with_nvidia=$withval],[
	with_nvidia="yes"
])

if test "$with_nvidia" = "no"; then
        echo "Disabling nvidia support"
else
	AC_ARG_WITH(x11,
		[  --with-x11[=DIR]   use x11 in DIR],[
        	if test "$withval" != "no" -a "$withval" != "yes"; then
                   	X11_DIR=$withval
		   	CFLAGS="${CFLAGS} -I${X11_DIR}/include"
			LIBS="${LIBS} -L${X11_DIR}/lib"
        	fi
		with_x11=$withval],[
		with_x11="yes"
	])

	AC_CHECK_HEADERS(X11/Xlib.h,
                AC_CHECK_LIB(X11, XOpenDisplay, [ HAVE_X11=true ]))
	# can't embed next AC_CHECK_HEADERS in true case of AC_CHECK_LIB
	if test "${HAVE_X11}" = "true"; then
	        LIBS="${LIBS} -lX11 -lXext"
		# need to specifically include X11/Xlib header when
		# doing checks for NVCtrl headers
	        AC_CHECK_HEADERS(NVCtrl/NVCtrl.h NVCtrl/NVCtrlLib.h, [ HAVE_NVIDIA=true ], [],
	        [
	        #include <X11/Xlib.h>
         	])
	        if test "${HAVE_NVIDIA}" = "true"; then
	                AC_CHECK_LIB(XNVCtrl, XNVCTRLQueryExtension, [
                        NVIDIA_LIBS="-lXNVCtrl"
	                AC_DEFINE(HAVE_NVIDIA,1,[nvidia sensors available])
	                ])
	        fi
	fi
fi
AM_CONDITIONAL(NVIDIA, test -n "$NVIDIA_LIBS")

AC_SUBST(NVIDIA_CFLAGS)
AC_SUBST(NVIDIA_LIBS)

AC_SUBST(CFLAGS)
# for help docs stuff
AC_PATH_PROG(XSLTPROC, xsltproc, no)
if test x"$XSLTPROC" = xno; then
  AC_MSG_ERROR([xsltproc executable not found in your path - should be installed with libxslt])
fi

GNOME_COMPILE_WARNINGS(maximum)

# ================ gnome-doc-utils stuff ============= #
GNOME_DOC_INIT

# ==================== plugin stuff ================== #
PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

# ================= generate files =================== #
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	pixmaps/Makefile
	plugins/Makefile
	plugins/libsensors/Makefile
	po/Makefile.in
])
AC_OUTPUT
