sources = [ 'is-udisks-plugin.c' ]
deps = [ libatasmart_dep, glib_dep, gio_dep, gtk_dep, libpeas_dep ]
is_udisks_plugin = shared_module('udisks',
                                    sources: sources,
                                    dependencies: deps,
                                    include_directories : root_include_dir,
                                    install: true,
                                    install_dir: get_option('libdir') / 'indicator-sensors/plugins')

install_data('udisks.plugin',
             install_dir: get_option('libdir') / 'indicator-sensors/plugins')
