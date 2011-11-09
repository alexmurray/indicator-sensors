pixmapsdir = $(datadir)/icons/hicolor/22x22/status
pixmaps_DATA = \
	very-low-value-icon.png \
	low-value-icon.png \
	normal-value-icon.png \
	high-value-icon.png \
	very-high-value-icon.png

deviceiconsdir = $(datadir)/icons/hicolor/22x22/devices
deviceicons_DATA = \
	indicator-sensors-battery.png \
	indicator-sensors-case.png \
	indicator-sensors-chip.png \
	indicator-sensors-cpu.png \
	indicator-sensors-gpu.png \
	indicator-sensors-fan.png \
	indicator-sensors-drive-harddisk.png \
	indicator-sensors-memory.png

appicondir = $(datadir)/icons/hicolor/48x48/apps
appicon_DATA = indicator-sensors.png

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
	$(pixmaps_DATA) \
	$(deviceicons_DATA) \
	$(appicon_DATA)
