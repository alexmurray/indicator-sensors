## Process this file with automake to produce Makefile.in
INCLUDES = -DGNOMELOCALEDIR=\""$(datadir)/locale/"\" \
	-DG_LOG_DOMAIN=\""Sensors Applet"\" \
	-DPIXMAPS_DIR=\""$(datadir)/pixmaps/$(PACKAGE)/"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DPREFIX=\""$(prefix)"\" \
	-I$(top_srcdir) \
	$(GLIB_CFLAGS) $(GTK_CFLAGS) $(APPINDICATOR_CFLAGS)


LIBS = $(GLIB_LIBS) $(GTK_LIBS) $(APPINDICATOR_LIBS)

bin_PROGRAMS = indicator-sensors
indicator_sensors_SOURCES = indicator-sensors.c \
	is-indicator.h \
	is-indicator.c \
	is-sensor.h \
	is-sensor.c

