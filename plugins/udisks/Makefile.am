plugindir = $(libdir)/$(PACKAGE)/plugins/udisks

INCLUDES = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(GIO_CFLAGS)		\
	$(LIBATASMART_CFLAGS)

plugin_LTLIBRARIES = libudisks.la

libudisks_la_SOURCES = \
	is-udisks-plugin.h	\
	is-udisks-plugin.c

libudisks_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libudisks_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(GIO_LIBS)		\
	$(LIBATASMART_LIBS)

plugin_DATA = udisks.plugin

EXTRA_DIST =  $(plugin_DATA)

