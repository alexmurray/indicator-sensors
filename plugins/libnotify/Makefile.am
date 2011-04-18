plugindir = $(libdir)/$(PACKAGE)/plugins/libnotify

INCLUDES = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(LIBNOTIFY_CFLAGS)

plugin_LTLIBRARIES = liblibnotify.la

liblibnotify_la_SOURCES = \
	is-libnotify-plugin.h		\
	is-libnotify-plugin.c

liblibnotify_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
liblibnotify_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(LIBNOTIFY_LIBS)

plugin_DATA = libnotify.plugin

EXTRA_DIST = $(plugin_DATA)
