sources = [ 'is-udisks2-plugin.c' ]
deps = [ glib_dep, gio_dep, gtk_dep, libpeas_dep, udisks2_dep ]
is_udisks2_plugin = shared_module('udisks2',
                                    sources: sources,
                                    dependencies: deps,
                                    include_directories : root_include_dir,
                                    install: true,
                                    install_dir: get_option('libdir') / 'indicator-sensors/plugins')

install_data('udisks2.plugin',
             install_dir: get_option('libdir') / 'indicator-sensors/plugins')
