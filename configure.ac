# ================= initialization =================== #
AC_PREREQ([2.68])
AC_INIT([Hardware Sensors Indicator],[0.8],[https://bugs.launchpad.net/indicator-sensors],[indicator-sensors])

AC_CONFIG_HEADERS([indicator-sensors/config.h])
AC_CONFIG_SRCDIR([indicator-sensors/indicator-sensors.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 dist-bzip2])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

# ============== basic compiler settings ============= #
AC_PROG_CC
AC_PROG_INSTALL
AC_HEADER_STDC

# ================= initialize libtool =============== #
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

# =========== take care of some localization ========= #
IT_PROG_INTLTOOL([0.40.0])

GETTEXT_PACKAGE=indicator-sensors
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

# ========== export compiler / linker options ======== #
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(LIBS)

# ============== look for dependencies =============== #
# check for headers needed for standard interfaces
#AC_CHECK_HEADERS()

GLIB_REQUIRED=2.28.0
GIO_REQUIRED=2.26.0
GTK_REQUIRED=3.0.0
APPINDICATOR_REQUIRED=0.0.7
LIBPEAS_REQUIRED=0.7.2
LIBNOTIFY_REQUIRED=0.7.2
LIBUDISKS2_REQUIRED=2.0.0
LIBATASMART_REQUIRED=0.16

PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GIO, gio-2.0 >= $GIO_REQUIRED)
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= $GTK_REQUIRED)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(LIBPEAS, libpeas-1.0 >= $LIBPEAS_REQUIRED)
AC_SUBST(LIBPEAS_CFLAGS)
AC_SUBST(LIBPEAS_LIBS)

PKG_CHECK_MODULES(LIBPEASGTK, libpeas-gtk-1.0 >= $LIBPEAS_REQUIRED)
AC_SUBST(LIBPEASGTK_CFLAGS)
AC_SUBST(LIBPEASGTK_LIBS)

PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= $LIBNOTIFY_REQUIRED)
AC_SUBST(LIBNOTIFY_CFLAGS)
AC_SUBST(LIBNOTIFY_LIBS)

PKG_CHECK_MODULES(APPINDICATOR, appindicator3-0.1 >= $APPINDICATOR_REQUIRED,
  AC_DEFINE(HAVE_APPINDICATOR, 1, [appindicator is available]),
  AC_DEFINE(HAVE_APPINDICATOR, 0, [appindicator is not available]))
AM_CONDITIONAL(APPINDICATOR, test -n "$APPINDICATOR_LIBS")

AC_SUBST(APPINDICATOR_CFLAGS)
AC_SUBST(APPINDICATOR_LIBS)

GLIB_GSETTINGS

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

PKG_CHECK_MODULES(LIBATASMART, libatasmart >= $LIBATASMART_REQUIRED,
		  HAVE_LIBATASMART="yes", HAVE_LIBATASMART="no")
AC_SUBST(LIBATASMART_CFLAGS)
AC_SUBST(LIBATASMART_LIBS)
AM_CONDITIONAL(UDISKS, test -n "$LIBATASMART_CFLAGS")

PKG_CHECK_MODULES(LIBUDISKS2, udisks2 >= $LIBUDISKS2_REQUIRED,
                  HAVE_LIBUDISKS2="yes", HAVE_LIBUDISKS2="no")
AC_SUBST(LIBUDISKS2_CFLAGS)
AC_SUBST(LIBUDISKS2_LIBS)
AM_CONDITIONAL(UDISKS2, test -n "$LIBUDISKS2_CFLAGS")

# enable fake plugin if requested
AC_ARG_ENABLE(fake,
AS_HELP_STRING([--enable-fake], [Build plugin to generate fake sensors for testing]))
AM_CONDITIONAL(FAKE, test "x$enable_fake" = "xyes")

# ============ enable extra warnings etc ============= #
if test "$USE_MAINTAINER_MODE" = "yes"; then
   CFLAGS+=" -ggdb"
   CFLAGS+=" -Wall -Wextra"
   CFLAGS+=" -Wno-missing-field-initializers"
   CFLAGS+=" -Wno-unused-parameter"
   CFLAGS+=" -Wmissing-declarations"
   CFLAGS+=" -Wredundant-decls"
   CFLAGS+=" -Wmissing-noreturn"
   CFLAGS+=" -Wshadow"
   CFLAGS+=" -Wpointer-arith"
   CFLAGS+=" -Wcast-align"
   CFLAGS+=" -Wwrite-strings"
   CFLAGS+=" -Winline"
   CFLAGS+=" -Wformat-nonliteral"
   CFLAGS+=" -Wformat-security"
   CFLAGS+=" -Wswitch-enum"
   CFLAGS+=" -Wswitch-default"
   CFLAGS+=" -Winit-self"
   CFLAGS+=" -Wmissing-include-dirs"
   CFLAGS+=" -Wundef"
   CFLAGS+=" -Waggregate-return"
   CFLAGS+=" -Wmissing-format-attribute"
   CFLAGS+=" -Wnested-externs"
fi

AC_SUBST(CFLAGS)

# ========== extra debugging options ================= #
m4_define([debug_default],[minimum])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
                              [turn on debugging @<:@default=debug_default@:>@])],
                              [],
                              [enable_debug=debug_default])
AS_CASE([$enable_debug],
        [yes], [ test "$cflags_set" = set || CFLAGS="$CFLAGS -O0 -g3 -ggdb" DEBUG_CFLAGS="-DDEBUG" ],
        [minimum], [ DEBUG_CFLAGS="-DG_DISABLE_CAST_CHECKS" ],
        [no], [ DEBUG_CFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS" ],
        [AC_MSG_ERROR([Unknown argument to --enable-debug])])
AC_SUBST(DEBUG_CFLAGS)

# =========== we need glib-genmarshal ================ #
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

GNOME_COMPILE_WARNINGS(maximum)

# Use --enable-maintainer-mode to disabled deprecated symbols
GNOME_MAINTAINER_MODE_DEFINES

# ==================== plugin stuff ================== #
PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

# ================= generate files =================== #
AC_CONFIG_FILES([
	Makefile
	data/Makefile
	data/indicator-sensors.gschema.xml.in
	indicator-sensors/Makefile
	pixmaps/Makefile
	plugins/Makefile
	plugins/aticonfig/Makefile
	plugins/dbus/Makefile
	plugins/dynamic/Makefile
	plugins/fake/Makefile
	plugins/libsensors/Makefile
	plugins/max/Makefile
	plugins/nvidia/Makefile
	plugins/udisks/Makefile
	plugins/udisks2/Makefile
	po/Makefile.in
])
AC_OUTPUT
