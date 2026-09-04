gschema_file = 'indicator-sensors.gschema.xml'
gschema_output = i18n.merge_file(
  input: gschema_file + '.in',
  output: gschema_file,
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas'
)

# compile the schema into the build tree too (not just on install) so
# it can be used uninstalled (e.g. by tests, via GSETTINGS_SCHEMA_DIR).
# gnome.compile_schemas() can't be used here since it only scans the
# source directory for *.gschema.xml files, but ours only exists there
# as the untranslated *.gschema.xml.in - the real one is generated into
# the build directory by i18n.merge_file() above
glib_compile_schemas = find_program('glib-compile-schemas')
custom_target('compile-schemas',
              input: gschema_output,
              output: 'gschemas.compiled',
              command: [glib_compile_schemas, '--targetdir',
                        meson.current_build_dir(), meson.current_build_dir()],
              build_by_default: true)

desktop_file = 'indicator-sensors.desktop'
desktop_output_file = i18n.merge_file(
  input: desktop_file + '.in',
  output: desktop_file,
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'applications'
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test(
    'validate-desktop',
    desktop_file_validate,
    args: [desktop_output_file]
  )
endif

appdata_file = 'indicator-sensors.appdata.xml'
i18n.merge_file(
  input: appdata_file + '.in',
  output: appdata_file,
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'appdata'
)
