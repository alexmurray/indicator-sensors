statusiconsdir = $(datadir)/icons/hicolor/22x22/status
statusicons_DATA = \
	hicolor/22x22/status/very-low-value-icon.png \
	hicolor/22x22/status/low-value-icon.png \
	hicolor/22x22/status/normal-value-icon.png \
	hicolor/22x22/status/high-value-icon.png \
	hicolor/22x22/status/very-high-value-icon.png

deviceiconsdir = $(datadir)/icons/hicolor/22x22/devices
deviceicons_DATA = \
	hicolor/22x22/devices/indicator-sensors-battery.png \
	hicolor/22x22/devices/indicator-sensors-case.png \
	hicolor/22x22/devices/indicator-sensors-chip.png \
	hicolor/22x22/devices/indicator-sensors-cpu.png \
	hicolor/22x22/devices/indicator-sensors-gpu.png \
	hicolor/22x22/devices/indicator-sensors-fan.png \
	hicolor/22x22/devices/indicator-sensors-disk.png \
	hicolor/22x22/devices/indicator-sensors-memory.png

appicon256dir = $(datadir)/icons/hicolor/256x256/apps
appicon256_DATA = hicolor/256x256/apps/indicator-sensors.png
appicon48dir = $(datadir)/icons/hicolor/48x48/apps
appicon48_DATA = hicolor/48x48/apps/indicator-sensors.png
appicon32dir = $(datadir)/icons/hicolor/32x32/apps
appicon32_DATA = hicolor/32x32/apps/indicator-sensors.png
appicon24dir = $(datadir)/icons/hicolor/24x24/apps
appicon24_DATA = hicolor/24x24/apps/indicator-sensors.png
appicon16dir = $(datadir)/icons/hicolor/16x16/apps
appicon16_DATA = hicolor/16x16/apps/indicator-sensors.png
appicon_files = \
	$(appicon256_DATA) \
	$(appicon48_DATA) \
	$(appicon32_DATA) \
	$(appicon24_DATA) \
	$(appicon16_DATA) \
	hicolor/indicator-sensors.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update_icon_cache
uninstall-hook: update_icon_cache
update_icon_cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = \
	$(statusicons_DATA) \
	$(deviceicons_DATA) \
	$(appicon_files)
