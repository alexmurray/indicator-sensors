plugindir = $(libdir)/$(PACKAGE)/plugins/fake

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS)

plugin_LTLIBRARIES = libfake.la

libfake_la_SOURCES = \
	is-fake-plugin.h		\
	is-fake-plugin.c

libfake_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libfake_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS)

plugin_DATA = fake.plugin

EXTRA_DIST = $(plugin_DATA)
