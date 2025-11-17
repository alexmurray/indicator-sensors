sources = [ 'is-fake-plugin.c' ]
deps = [ glib_dep, gio_dep, gtk_dep, libpeas_dep ]
is_fake_plugin = shared_module('fake',
                                    sources: sources,
                                    dependencies: deps,
                                    include_directories : root_include_dir,
                                    install: true,
                                    install_dir: get_option('libdir') / 'indicator-sensors/plugins')

install_data('fake.plugin',
             install_dir: get_option('libdir') / 'indicator-sensors/plugins')
