appdir = $(datadir)/applications
app_in_files = indicator-sensors.desktop.in
app_DATA = $(app_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

gsettings_SCHEMAS = indicator-sensors.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

CLEANFILES = \
        $(gsettings_SCHEMAS)

DISTCLEANFILES = $(app_DATA) \
        $(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

EXTRA_DIST = $(app_in_files) \
	indicator-sensors.gschema.xml.in
