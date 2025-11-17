sources = [ 'is-dynamic-plugin.c' ]
deps = [ glib_dep, gio_dep, gtk_dep, libpeas_dep ]
is_dynamic_plugin = shared_module('dynamic',
                                    sources: sources,
                                    dependencies: deps,
                                    include_directories : root_include_dir,
                                    install: true,
                                    install_dir: get_option('libdir') / 'indicator-sensors/plugins')

install_data('dynamic.plugin',
             install_dir: get_option('libdir') / 'indicator-sensors/plugins')
