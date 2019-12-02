plugindir = $(libdir)/$(PACKAGE)/plugins/dbus

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	$(AYATANA_APPINDICATOR_CFLAGS)	\
	$(LIBPEAS_CFLAGS) 	\
	$(GIO_CFLAGS)           \
	$(DEBUG_CFLAGS)

plugin_LTLIBRARIES = libdbus.la

libdbus_la_SOURCES = \
	is-active-sensor-generated.h \
	is-active-sensor-generated.c \
	is-org-gnome-shell-search-provider-generated.c \
	is-org-gnome-shell-search-provider-generated.h \
	is-dbus-plugin.h \
	is-dbus-plugin.c

libdbus_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libdbus_la_LIBADD  = 	\
	$(GLIB_LIBS)		\
	$(GTK_LIBS) 		\
	$(AYATANA_APPINDICATOR_LIBS)	\
	$(LIBPEAS_LIBS) 	\
	$(GIO_LIBS)

plugin_DATA = dbus.plugin

dbus_search_provider_built_sources = \
	is-org-gnome-shell-search-provider-generated.c \
	is-org-gnome-shell-search-provider-generated.h \
	$(NULL)

$(dbus_search_provider_built_sources) : Makefile.am shell-search-provider-dbus-interfaces.xml
	$(AM_V_GEN) gdbus-codegen \
		--interface-prefix com.github.alexmurray.IndicatorSensors. \
		--c-namespace Is \
		--generate-c-code is-org-gnome-shell-search-provider-generated \
		shell-search-provider-dbus-interfaces.xml \
		$(NULL)

active_sensor_built_sources = \
	is-active-sensor-generated.h \
	is-active-sensor-generated.c \
	is-active-sensor-generated-com.github.alexmurray.IndicatorSensors.ActiveSensor.xml

$(active_sensor_built_sources): Makefile.am is-active-sensor.xml
	$(AM_V_GEN) gdbus-codegen \
		--interface-prefix com.github.alexmurray.IndicatorSensors. \
		--c-namespace Is \
		--c-generate-object-manager \
		--generate-c-code is-active-sensor-generated \
		--generate-docbook is-active-sensor-generated \
		is-active-sensor.xml \
		$(NULL)

searchproviderdir = $(datadir)/gnome-shell/search-providers
searchprovider_DATA = is-search-provider.ini

BUILT_SOURCES = $(dbus_search_provider_built_sources) \
	$(active_sensor_built_sources)

EXTRA_DIST =  $(plugin_DATA) \
	shell-search-provider-dbus-interfaces.xml \
	is-active-sensor.xml \
	$(NULL)

CLEANFILES = $(dbus_search_provider_built_sources) \
	$(active_sensor_built_sources)
