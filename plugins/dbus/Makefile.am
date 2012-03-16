plugindir = $(libdir)/$(PACKAGE)/plugins/dbus

INCLUDES = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(GIO_CFLAGS)

plugin_LTLIBRARIES = libdbus.la

libdbus_la_SOURCES = \
	is-active-sensor-objectmanager-generated.h \
	is-active-sensor-objectmanager-generated.c \
	is-dbus-plugin.h		\
	is-dbus-plugin.c

libdbus_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libdbus_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(GIO_LIBS)

plugin_DATA = dbus.plugin

GDBUS_GENERATED = \
	is-active-sensor-objectmanager-generated.h \
	is-active-sensor-objectmanager-generated.c \
	is-active-sensor-objectmanager-generated-com.github.alexmurray.IndicatorSensors.ObjectManager.ActiveSensor.xml

$(GDBUS_GENERATED): is-active-sensor-objectmanager.xml Makefile
	$(AM_V_GEN) gdbus-codegen \
		--interface-prefix com.github.alexmurray.IndicatorSensors.ObjectManager. \
		--c-namespace Is \
		--c-generate-object-manager \
		--generate-c-code is-active-sensor-objectmanager-generated \
		--generate-docbook is-active-sensor-objectmanager-generated \
		$<

BUILT_SOURCES = $(GDBUS_GENERATED)

EXTRA_DIST =  $(plugin_DATA) \
	is-active-sensor-objectmanager.xml

CLEANFILES = $(GDBUS_GENERATED)
