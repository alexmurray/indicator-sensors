plugindir = $(libdir)/$(PACKAGE)/plugins/udisks2

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(AYATANA_APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(GIO_CFLAGS)		\
	$(LIBUDISKS2_CFLAGS)    \
	$(DEBUG_CFLAGS)

plugin_LTLIBRARIES = libudisks2.la

libudisks2_la_SOURCES = \
	is-udisks2-plugin.h	\
	is-udisks2-plugin.c

libudisks2_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libudisks2_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(AYATANA_APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(GIO_LIBS)		\
	$(LIBUDISKS2_LIBS)

plugin_DATA = udisks2.plugin

EXTRA_DIST =  $(plugin_DATA)
