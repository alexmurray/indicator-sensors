appdir = $(datadir)/applications
app_in_files = indicator-sensors.desktop.in
app_DATA = $(app_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(app_in_files)
DISTCLEANFILES = $(app_DATA)