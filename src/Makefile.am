## Process this file with automake to produce Makefile.in
INCLUDES = -DGNOMELOCALEDIR=\""$(datadir)/locale/"\" \
	-DG_LOG_DOMAIN=\""indicator-sensors"\" \
	-DPIXMAPS_DIR=\""$(datadir)/pixmaps/$(PACKAGE)/"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DPREFIX=\""$(prefix)"\" \
	-I$(top_srcdir) \
	$(GLIB_CFLAGS) $(GTK_CFLAGS) $(APPINDICATOR_CFLAGS) $(LIBPEAS_CFLAGS)


LIBS = $(GLIB_LIBS) $(GTK_LIBS) $(APPINDICATOR_LIBS) $(LIBPEAS_LIBS)

bin_PROGRAMS = indicator-sensors
indicator_sensors_SOURCES = indicator-sensors.c \
	is-indicator.h \
	is-indicator.c \
	is-sensor.h \
	is-sensor.c \
	is-temperature-sensor.h \
	is-temperature-sensor.c \
	is-store.h \
	is-store.c \
	is-manager.h \
	is-manager.c \
	is-preferences-dialog.h \
	is-preferences-dialog.c

