plugindir = $(libdir)/$(PACKAGE)/plugins/aticonfig

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(AYATANA_APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(ATICONFIG_CFLAGS)     \
	$(DEBUG_CFLAGS)

plugin_LTLIBRARIES = libaticonfig.la

libaticonfig_la_SOURCES = \
	is-aticonfig-plugin.h		\
	is-aticonfig-plugin.c

libaticonfig_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libaticonfig_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(AYATANA_APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(ATICONFIG_LIBS)

plugin_DATA = aticonfig.plugin

EXTRA_DIST = $(plugin_DATA)
