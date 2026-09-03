gschema_file = 'indicator-sensors.gschema.xml'
i18n.merge_file(
  input: gschema_file + '.in',
  output: gschema_file,
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas'
)

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
