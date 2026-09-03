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

#include "is-sensor.h"
#include "is-store.h"

static void
test_store_add_and_find_sensor(void)
{
  IsStore *store = is_store_new();
  IsSensor *sensor = is_sensor_new("plugin/device/sensor1");
  GtkTreeIter iter;

  g_assert_true(is_store_add_sensor(store, sensor, &iter));
  g_assert_true(is_store_get_iter(store, "plugin/device/sensor1", &iter));

  g_object_unref(sensor);
  g_object_unref(store);
}

static void
test_store_add_duplicate_fails(void)
{
  IsStore *store = is_store_new();
  IsSensor *sensor = is_sensor_new("plugin/device/sensor1");
  GtkTreeIter iter;

  g_assert_true(is_store_add_sensor(store, sensor, &iter));
  g_assert_false(is_store_add_sensor(store, sensor, &iter));

  g_object_unref(sensor);
  g_object_unref(store);
}

static void
test_store_remove_path(void)
{
  IsStore *store = is_store_new();
  IsSensor *sensor = is_sensor_new("plugin/device/sensor1");
  GtkTreeIter iter;

  is_store_add_sensor(store, sensor, &iter);
  g_assert_true(is_store_remove_path(store, "plugin/device/sensor1"));
  g_assert_false(is_store_get_iter(store, "plugin/device/sensor1", &iter));

  g_object_unref(sensor);
  g_object_unref(store);
}

static void
test_store_remove_unknown_path_fails(void)
{
  IsStore *store = is_store_new();

  g_assert_false(is_store_remove_path(store, "no/such/path"));

  g_object_unref(store);
}

static void
test_store_shared_hierarchy(void)
{
  IsStore *store = is_store_new();
  IsSensor *sensor1 = is_sensor_new("plugin/device/sensor1");
  IsSensor *sensor2 = is_sensor_new("plugin/device/sensor2");
  GtkTreeIter iter1, iter2, parent_iter;

  is_store_add_sensor(store, sensor1, &iter1);
  is_store_add_sensor(store, sensor2, &iter2);

  /* both sensors share the "plugin/device" intermediate entry */
  g_assert_true(is_store_get_iter(store, "plugin/device", &parent_iter));
  g_assert_cmpint(
      gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), &parent_iter), ==,
      2);

  g_object_unref(sensor1);
  g_object_unref(sensor2);
  g_object_unref(store);
}

int
main(int argc, char *argv[])
{
  gtk_test_init(&argc, &argv, NULL);

  g_test_add_func("/store/add-and-find-sensor", test_store_add_and_find_sensor);
  g_test_add_func("/store/add-duplicate-fails", test_store_add_duplicate_fails);
  g_test_add_func("/store/remove-path", test_store_remove_path);
  g_test_add_func("/store/remove-unknown-path-fails",
                  test_store_remove_unknown_path_fails);
  g_test_add_func("/store/shared-hierarchy", test_store_shared_hierarchy);

  return g_test_run();
}
