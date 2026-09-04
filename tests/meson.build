is_dir = '..' / 'indicator-sensors'

test_deps = [glib_dep, gio_dep, gtk_dep]
test_include_dirs = [root_include_dir, include_directories(is_dir)]
# gtk_test_init() unconditionally tries to connect to the AT-SPI
# accessibility bus - on systems without an AT-SPI D-Bus service
# available (e.g. minimal CI images) this is a fatal error, so
# disable the ATK bridge's attempt to connect to it entirely
test_env = environment({'NO_AT_BRIDGE': '1'})

test_temperature_sensor = executable(
  'test-temperature-sensor',
  [
    'test-temperature-sensor.c',
    is_dir / 'is-temperature-sensor.c',
    is_dir / 'is-sensor.c',
    is_dir / 'is-log.c',
    is_dir / 'is-notify.c',
  ],
  dependencies: test_deps,
  include_directories: test_include_dirs,
)
test('temperature-sensor', test_temperature_sensor, env: test_env)

test_sensor = executable(
  'test-sensor',
  [
    'test-sensor.c',
    is_dir / 'is-sensor.c',
    is_dir / 'is-log.c',
    is_dir / 'is-notify.c',
  ],
  dependencies: test_deps,
  include_directories: test_include_dirs,
)
test('sensor', test_sensor, env: test_env)

test_store = executable(
  'test-store',
  [
    'test-store.c',
    is_dir / 'is-store.c',
    is_dir / 'is-sensor.c',
    is_dir / 'is-log.c',
    is_dir / 'is-notify.c',
  ],
  dependencies: test_deps,
  include_directories: test_include_dirs,
)
test('store', test_store, env: test_env)

test_manager = executable(
  'test-manager',
  [
    'test-manager.c',
    is_dir / 'is-manager.c',
    is_dir / 'is-store.c',
    is_dir / 'is-sensor.c',
    is_dir / 'is-log.c',
    is_dir / 'is-notify.c',
  ] + marshalled_files,
  dependencies: test_deps,
  include_directories: test_include_dirs,
)
test('manager', test_manager, env: test_env)

# the fake-plugin integration test loads the real, built fake plugin via
# libpeas, so it needs the same source set as the indicator-sensors
# executable itself (minus indicator-sensors.c) plus the fake plugin
# to actually be built
if get_option('enable-fake')
  fake_plugin_dir = meson.project_build_root() / 'plugins' / 'fake'

  test_fake_plugin_integration = executable(
    'test-fake-plugin-integration',
    [
      'test-fake-plugin-integration.c',
      is_dir / 'is-activatable.c',
      is_dir / 'is-application.c',
      is_dir / 'is-log.c',
      is_dir / 'is-notify.c',
      is_dir / 'is-indicator.c',
      is_dir / 'is-sensor.c',
      is_dir / 'is-temperature-sensor.c',
      is_dir / 'is-fan-sensor.c',
      is_dir / 'is-store.c',
      is_dir / 'is-manager.c',
      is_dir / 'is-preferences-dialog.c',
      is_dir / 'is-sensor-dialog.c',
    ] + marshalled_files,
    c_args: ['-DFAKE_PLUGIN_DIR="@0@"'.format(fake_plugin_dir)],
    dependencies: [glib_dep, gio_dep, gtk_dep, appindicator_dep, libpeas_dep],
    include_directories: test_include_dirs,
  )
  # constructing a real IsApplication binds its manager's
  # "enabled-sensors" property to the real indicator-sensors.manager
  # GSettings schema - force an in-memory backend so the test never
  # touches the real, installed application's saved settings, and
  # point at the schema compiled into the build tree by the
  # 'compile-schemas' custom_target in data/meson.build since the
  # schema is not otherwise installed anywhere in an uninstalled build
  fake_plugin_integration_env = environment({'NO_AT_BRIDGE': '1'})
  fake_plugin_integration_env.set('GSETTINGS_BACKEND', 'memory')
  fake_plugin_integration_env.set('GSETTINGS_SCHEMA_DIR',
                                  meson.project_build_root() / 'data')
  test('fake-plugin-integration', test_fake_plugin_integration,
       depends: [is_fake_plugin], timeout: 30,
       env: fake_plugin_integration_env)
endif
