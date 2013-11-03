plugindir = $(libdir)/$(PACKAGE)/plugins/libsensors

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(LIBSENSORS_CFLAGS)

plugin_LTLIBRARIES = liblibsensors.la

liblibsensors_la_SOURCES = \
	is-libsensors-plugin.h		\
	is-libsensors-plugin.c

liblibsensors_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
liblibsensors_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(LIBSENSORS_LIBS)

plugin_DATA = libsensors.plugin

EXTRA_DIST = $(plugin_DATA)
