project('indicator-sensors', 'c',
        license: 'GPL3+',
        version : '2.0',
        default_options: ['c_std=gnu11'])

glib_dep = dependency('glib-2.0', version : '>= 2.27')
gio_dep = dependency('gio-2.0', version : '>= 2.40')
gtk_dep = dependency('gtk+-3.0', version : '>= 3.0')
libpeas_dep = dependency('libpeas-2', version : '>= 2.0')
appindicator_dep = dependency('ayatana-appindicator3-0.1', version : '>= 0.0.7')
udisks2_dep = dependency('udisks2', version : '>= 2.0',
                         required: false)

cc = meson.get_compiler('c')
libsensors_dep = cc.find_library('sensors', required: false)

if get_option('enable-nvidia')
  x11_dep = dependency('x11', required: true)
  libxnvctrl_dep = cc.find_library('XNVCtrl', required: true)
endif

conf = configuration_data()
# Surround the version in quotes to make it a C string
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_NAME', 'Hardware Sensors Indicator')
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('LIBDIR', join_paths(get_option( 'prefix' ), get_option('libdir')))
conf.set_quoted('DATADIR', join_paths(get_option( 'prefix' ), get_option('datadir')))

configure_file(output : 'config.h',
               configuration : conf)

root_include_dir = include_directories('.')

i18n = import('i18n')
gnome = import('gnome')
add_project_arguments('-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()), language: 'c')

subdir('data')
subdir('icons')
subdir('plugins')
subdir('po')
subdir('indicator-sensors')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
)

summary({
  'fake_plugin': get_option('enable-fake'),
  'udisks2_plugin': udisks2_dep.found(),
  'libsensors_plugin': libsensors_dep.found(),
  'nvidia_plugin': get_option('enable-nvidia'),
}, section: 'Plugins')
