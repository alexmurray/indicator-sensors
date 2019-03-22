desktopdir = $(datadir)/applications
desktop_in_files = indicator-sensors.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_XML_NOMERGE_RULE@
appdatadir = $(datadir)/metainfo
dist_appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = indicator-sensors.appdata.xml.in

@INTLTOOL_DESKTOP_RULE@

gsettings_SCHEMAS = indicator-sensors.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

CLEANFILES = \
        $(gsettings_SCHEMAS) \
	indicator-sensors.appdata.xml

DISTCLEANFILES = $(desktop_DATA) \
        $(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

EXTRA_DIST = $(desktop_in_files) \
	$(appdata_in_files) \
	indicator-sensors.gschema.xml.in
