sources = [ 'is-nvidia-plugin.c' ]
deps = [ glib_dep, gio_dep, gtk_dep, libpeas_dep, x11_dep, libxnvctrl_dep ]
is_nvidia_plugin = shared_module('nvidia',
                                    sources: sources,
                                    dependencies: deps,
                                    include_directories : root_include_dir,
                                    install: true,
                                    install_dir: get_option('libdir') / 'indicator-sensors/plugins')

install_data('nvidia.plugin',
             install_dir: get_option('libdir') / 'indicator-sensors/plugins')
