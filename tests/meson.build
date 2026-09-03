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
