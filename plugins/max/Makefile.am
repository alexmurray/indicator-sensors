plugindir = $(libdir)/$(PACKAGE)/plugins/max

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(AYATANA_APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(MAX_CFLAGS)       \
	$(DEBUG_CFLAGS)

plugin_LTLIBRARIES = libmax.la

libmax_la_SOURCES = \
	is-max-plugin.h		\
	is-max-plugin.c

libmax_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libmax_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(AYATANA_APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(MAX_LIBS)

plugin_DATA = max.plugin

EXTRA_DIST = $(plugin_DATA)
