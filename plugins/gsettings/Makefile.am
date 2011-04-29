plugindir = $(libdir)/$(PACKAGE)/plugins/gsettings

INCLUDES = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(GIO_CFLAGS)

plugin_LTLIBRARIES = libgsettings.la

libgsettings_la_SOURCES = \
	is-gsettings-plugin.h		\
	is-gsettings-plugin.c

libgsettings_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libgsettings_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(GIO_LIBS)

plugin_DATA = gsettings.plugin

gsettings_SCHEMAS = indicator-sensors.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

CLEANFILES = \
        $(gsettings_SCHEMAS)

DISTCLEANFILES = \
        $(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

EXTRA_DIST =  $(plugin_DATA) \
	indicator-sensors.gschema.xml.in

