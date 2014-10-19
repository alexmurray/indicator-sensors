plugindir = $(libdir)/$(PACKAGE)/plugins/dynamic

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(DYNAMIC_CFLAGS)

plugin_LTLIBRARIES = libdynamic.la

libdynamic_la_SOURCES = \
	is-dynamic-plugin.h		\
	is-dynamic-plugin.c

libdynamic_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libdynamic_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(DYNAMIC_LIBS)

plugin_DATA = dynamic.plugin

EXTRA_DIST = $(plugin_DATA)
