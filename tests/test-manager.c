/*
 * Copyright (C) 2011-2025 Alex Murray <murray.alex@gmail.com>
 *
 * indicator-sensors is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * indicator-sensors is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with indicator-sensors.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "is-manager.h"
#include "is-sensor.h"

static void
test_manager_add_sensor_starts_disabled(void)
{
  /* IsManager derives from GtkTreeView, so is_manager_new() returns a
   * floating reference like any other widget - sink it so the matching
   * g_object_unref() below is a normal, non-floating unref. */
  IsManager *manager = g_object_ref_sink(is_manager_new());
  IsSensor *sensor = is_sensor_new("plugin/sensor1");

  g_assert_true(is_manager_add_sensor(manager, sensor));
  g_assert_cmpuint(is_manager_get_num_enabled_sensors(manager), ==, 0);

  g_object_unref(sensor);
  g_object_unref(manager);
}

static void
test_manager_add_duplicate_sensor_fails(void)
{
  /* IsManager derives from GtkTreeView, so is_manager_new() returns a
   * floating reference like any other widget - sink it so the matching
   * g_object_unref() below is a normal, non-floating unref. */
  IsManager *manager = g_object_ref_sink(is_manager_new());
  IsSensor *sensor = is_sensor_new("plugin/sensor1");

  g_assert_true(is_manager_add_sensor(manager, sensor));
  g_assert_false(is_manager_add_sensor(manager, sensor));

  g_object_unref(sensor);
  g_object_unref(manager);
}

static void
test_manager_set_enabled_sensors(void)
{
  /* IsManager derives from GtkTreeView, so is_manager_new() returns a
   * floating reference like any other widget - sink it so the matching
   * g_object_unref() below is a normal, non-floating unref. */
  IsManager *manager = g_object_ref_sink(is_manager_new());
  IsSensor *sensor1 = is_sensor_new("plugin/sensor1");
  IsSensor *sensor2 = is_sensor_new("plugin/sensor2");
  const gchar *enabled[] = {"plugin/sensor1", NULL};
  gchar **got;

  is_manager_add_sensor(manager, sensor1);
  is_manager_add_sensor(manager, sensor2);

  is_manager_set_enabled_sensors(manager, enabled);
  g_assert_cmpuint(is_manager_get_num_enabled_sensors(manager), ==, 1);

  got = is_manager_get_enabled_sensors(manager);
  g_assert_cmpuint(g_strv_length(got), ==, 1);
  g_assert_cmpstr(got[0], ==, "plugin/sensor1");
  g_strfreev(got);

  g_object_unref(sensor1);
  g_object_unref(sensor2);
  g_object_unref(manager);
}

static void
test_manager_sensor_enabled_before_being_added(void)
{
  /* a sensor's path may already be marked enabled (e.g. restored from
   * gsettings) before the sensor itself is added by a plugin - adding
   * it afterwards should pick up the existing enabled state. */
  /* IsManager derives from GtkTreeView, so is_manager_new() returns a
   * floating reference like any other widget - sink it so the matching
   * g_object_unref() below is a normal, non-floating unref. */
  IsManager *manager = g_object_ref_sink(is_manager_new());
  IsSensor *sensor = is_sensor_new("plugin/sensor1");
  const gchar *enabled[] = {"plugin/sensor1", NULL};

  is_manager_set_enabled_sensors(manager, enabled);
  is_manager_add_sensor(manager, sensor);

  g_assert_cmpuint(is_manager_get_num_enabled_sensors(manager), ==, 1);

  g_object_unref(sensor);
  g_object_unref(manager);
}

int
main(int argc, char *argv[])
{
  gtk_test_init(&argc, &argv, NULL);

  g_test_add_func("/manager/add-sensor-starts-disabled",
                  test_manager_add_sensor_starts_disabled);
  g_test_add_func("/manager/add-duplicate-sensor-fails",
                  test_manager_add_duplicate_sensor_fails);
  g_test_add_func("/manager/set-enabled-sensors",
                  test_manager_set_enabled_sensors);
  g_test_add_func("/manager/sensor-enabled-before-being-added",
                  test_manager_sensor_enabled_before_being_added);

  return g_test_run();
}
