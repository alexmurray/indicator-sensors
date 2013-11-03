## Process this file with automake to produce Makefile.in
AM_CPPFLAGS = -DGNOMELOCALEDIR=\""$(datadir)/locale/"\" \
	-DG_LOG_DOMAIN=\""indicator-sensors"\" \
	-DPIXMAPS_DIR=\""$(datadir)/pixmaps/$(PACKAGE)/"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DPREFIX=\""$(prefix)"\" \
	-I$(top_srcdir) \
	$(GLIB_CFLAGS) $(GIO_CFLAGS) $(GTK_CFLAGS) $(APPINDICATOR_CFLAGS) $(LIBPEAS_CFLAGS) $(LIBNOTIFY_CFLAGS)


LIBS = $(GLIB_LIBS) $(GIO_LIBS) $(GTK_LIBS) $(APPINDICATOR_LIBS) $(LIBPEAS_LIBS) $(LIBNOTIFY_LIBS)

marshallers.h: marshallers.list
	glib-genmarshal --header $< > $@

marshallers.c: marshallers.list
	glib-genmarshal --body $< > $@

BUILT_SOURCES = marshallers.h marshallers.c

bin_PROGRAMS = indicator-sensors
indicator_sensors_SOURCES = \
	indicator-sensors.c \
	is-application.c \
	is-application.h \
	is-log.h \
	is-log.c \
	is-notify.h \
	is-notify.c \
	is-indicator.h \
	is-indicator.c \
	is-sensor.h \
	is-sensor.c \
	is-temperature-sensor.h \
	is-temperature-sensor.c \
	is-fan-sensor.h \
	is-fan-sensor.c \
	is-store.h \
	is-store.c \
	is-manager.h \
	is-manager.c \
	is-preferences-dialog.h \
	is-preferences-dialog.c \
	is-sensor-dialog.h \
	is-sensor-dialog.c

EXTRA_DIST = marshallers.list
DISTCLEANFILES = $(BUILT_SOURCES)
