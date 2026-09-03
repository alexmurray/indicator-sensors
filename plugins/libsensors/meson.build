sources = [ 'is-libsensors-plugin.c' ]
deps = [ glib_dep, gio_dep, gtk_dep, libpeas_dep, libsensors_dep ]
is_libsensors_plugin = shared_module('libsensors',
                                    sources: sources,
                                    dependencies: deps,
                                    include_directories : root_include_dir,
                                    install: true,
                                    install_dir: get_option('libdir') / 'indicator-sensors/plugins')

install_data('libsensors.plugin',
             install_dir: get_option('libdir') / 'indicator-sensors/plugins')
