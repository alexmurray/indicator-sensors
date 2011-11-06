plugindir = $(libdir)/$(PACKAGE)/plugins/aticonfig

INCLUDES = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(ATICONFIG_CFLAGS)

plugin_LTLIBRARIES = libaticonfig.la

libaticonfig_la_SOURCES = \
	is-aticonfig-plugin.h		\
	is-aticonfig-plugin.c

libaticonfig_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libaticonfig_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(ATICONFIG_LIBS)

plugin_DATA = aticonfig.plugin

EXTRA_DIST = $(plugin_DATA)
