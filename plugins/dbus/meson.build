gnome = import('gnome')

sources = [ 'is-dbus-plugin.c' ]
deps = [ glib_dep, gio_dep, gtk_dep, libpeas_dep ]

dbus_search_provider_built_sources = gnome.gdbus_codegen('is-org-gnome-shell-search-provider-generated',
                                                         sources: 'shell-search-provider-dbus-interfaces.xml',
                                                         interface_prefix: 'com.github.alexmurray.IndicatorSensors.',
                                                         namespace: 'Is',
                                                         install_header: false)
sources += dbus_search_provider_built_sources

active_sensor_built_sources = gnome.gdbus_codegen('is-active-sensor-generated',
                                                         sources: 'is-active-sensor.xml',
                                                         interface_prefix: 'com.github.alexmurray.IndicatorSensors.',
                                                         namespace: 'Is',
                                                         object_manager: true,
                                                         install_header: false)

sources += active_sensor_built_sources
is_dbus_plugin = shared_module('dbus',
                               sources: sources,
                                    dependencies: deps,
                                    include_directories : [ root_include_dir ] + [ include_directories('.')],
                                    install: true,
                                    install_dir: get_option('libdir') / 'indicator-sensors/plugins')

install_data('dbus.plugin',
             install_dir: get_option('libdir') / 'indicator-sensors/plugins')

