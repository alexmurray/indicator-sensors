plugindir = $(libdir)/$(PACKAGE)/plugins/dbus

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(GIO_CFLAGS)           \
	$(DEBUG_CFLAGS)

plugin_LTLIBRARIES = libdbus.la

libdbus_la_SOURCES = \
	is-active-sensor-generated.h \
	is-active-sensor-generated.c \
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
	is-active-sensor-generated.h \
	is-active-sensor-generated.c \
	is-active-sensor-generated-com.github.alexmurray.IndicatorSensors.ActiveSensor.xml

$(GDBUS_GENERATED): is-active-sensor.xml Makefile
	$(AM_V_GEN) gdbus-codegen \
		--interface-prefix com.github.alexmurray.IndicatorSensors. \
		--c-namespace Is \
		--c-generate-object-manager \
		--generate-c-code is-active-sensor-generated \
		--generate-docbook is-active-sensor-generated \
		$<

BUILT_SOURCES = $(GDBUS_GENERATED)

EXTRA_DIST =  $(plugin_DATA) \
	is-active-sensor.xml

CLEANFILES = $(GDBUS_GENERATED)
