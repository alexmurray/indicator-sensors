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

# also copy the plugin descriptor next to the built module in the build
# tree, so libpeas can discover it uninstalled (used by the fake-plugin
# integration test)
configure_file(input: 'fake.plugin', output: 'fake.plugin', copy: true)
