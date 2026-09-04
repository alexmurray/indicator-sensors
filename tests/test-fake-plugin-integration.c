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

#include "is-activatable.h"
#include "is-application.h"
#include "is-manager.h"
#include "is-sensor.h"
#include <gio/gio.h>
#include <libpeas.h>

#ifndef FAKE_PLUGIN_DIR
#error "FAKE_PLUGIN_DIR must be defined at compile time"
#endif

typedef struct
{
  IsManager *manager;
  IsApplication *application;
  PeasEngine *engine;
  PeasExtensionSet *extensions;
  PeasPluginInfo *info;
  IsActivatable *fake;
} Fixture;

static void
on_extension_added(PeasExtensionSet *set, PeasPluginInfo *info,
                   IsActivatable *activatable, gpointer user_data)
{
  Fixture *fixture = user_data;

  fixture->fake = g_object_ref(activatable);
  is_activatable_activate(activatable);
}

static void
fixture_setup(Fixture *fixture, gconstpointer user_data)
{
  fixture->manager = g_object_ref_sink(is_manager_new());
  fixture->application =
      g_object_new(IS_TYPE_APPLICATION, "manager", fixture->manager, NULL);

  fixture->engine = peas_engine_get_default();
  peas_engine_add_search_path(fixture->engine, FAKE_PLUGIN_DIR, NULL);

  fixture->extensions =
      peas_extension_set_new(fixture->engine, IS_TYPE_ACTIVATABLE,
                             "application", fixture->application, NULL);
  g_signal_connect(fixture->extensions, "extension-added",
                   G_CALLBACK(on_extension_added), fixture);

  /* the search path above is scoped to just the fake plugin's own
   * build directory, so the engine will have discovered exactly one
   * plugin - find it via GListModel, the same interface production
   * code (indicator-sensors.c) uses, rather than guessing at its
   * module name */
  g_assert_cmpuint(g_list_model_get_n_items(G_LIST_MODEL(fixture->engine)), ==,
                   1);
  fixture->info = PEAS_PLUGIN_INFO(
      g_list_model_get_item(G_LIST_MODEL(fixture->engine), 0));
  g_assert_nonnull(fixture->info);

  /* loading the plugin fires "extension-added" above synchronously,
   * which activates it and populates the manager with fake sensors */
  g_assert_true(peas_engine_load_plugin(fixture->engine, fixture->info));
  g_assert_nonnull(fixture->fake);
}

static void
fixture_teardown(Fixture *fixture, gconstpointer user_data)
{
  if (peas_plugin_info_is_loaded(fixture->info))
    peas_engine_unload_plugin(fixture->engine, fixture->info);
  g_object_unref(fixture->info);

  g_clear_object(&fixture->fake);
  g_clear_object(&fixture->extensions);
  g_clear_object(&fixture->application);
  g_clear_object(&fixture->manager);
}

static void
test_fake_plugin_adds_sensors_with_plausible_values(Fixture *fixture,
                                                     gconstpointer user_data)
{
  GSList *sensors, *list;
  guint n_sensors;
  GPtrArray *paths;

  sensors = is_manager_get_all_sensors_list(fixture->manager);
  n_sensors = g_slist_length(sensors);
  g_assert_cmpuint(n_sensors, ==, 5);

  /* enable every fake sensor so the application's sensor-enabled
   * handler triggers an immediate (synchronous) value update, exactly
   * as happens when a user enables a sensor via the preferences
   * dialog */
  paths = g_ptr_array_new();
  for (list = sensors; list != NULL; list = list->next)
  {
    IsSensor *sensor = IS_SENSOR(list->data);

    g_assert_true(g_str_has_prefix(is_sensor_get_path(sensor), "fake/"));
    g_ptr_array_add(paths, (gpointer)is_sensor_get_path(sensor));
  }
  g_ptr_array_add(paths, NULL);

  is_manager_set_enabled_sensors(fixture->manager,
                                 (const gchar **)paths->pdata);
  g_assert_cmpuint(is_manager_get_num_enabled_sensors(fixture->manager), ==,
                   5);

  for (list = sensors; list != NULL; list = list->next)
  {
    IsSensor *sensor = IS_SENSOR(list->data);
    gdouble value = is_sensor_get_value(sensor);

    g_assert_cmpfloat(value, !=, IS_SENSOR_VALUE_UNSET);
    g_assert_cmpfloat(value, >=, is_sensor_get_low_value(sensor));
    g_assert_cmpfloat(value, <=, is_sensor_get_high_value(sensor));
  }

  g_ptr_array_free(paths, TRUE);
  g_slist_free_full(sensors, g_object_unref);
}

static void
test_fake_plugin_deactivate_removes_sensors(Fixture *fixture,
                                            gconstpointer user_data)
{
  GSList *sensors;

  sensors = is_manager_get_all_sensors_list(fixture->manager);
  g_assert_cmpuint(g_slist_length(sensors), ==, 5);
  g_slist_free_full(sensors, g_object_unref);

  is_activatable_deactivate(fixture->fake);

  sensors = is_manager_get_all_sensors_list(fixture->manager);
  g_assert_cmpuint(g_slist_length(sensors), ==, 0);
}

int
main(int argc, char *argv[])
{
  gtk_test_init(&argc, &argv, NULL);

  g_test_add("/fake-plugin/adds-sensors-with-plausible-values", Fixture, NULL,
            fixture_setup, test_fake_plugin_adds_sensors_with_plausible_values,
            fixture_teardown);
  g_test_add("/fake-plugin/deactivate-removes-sensors", Fixture, NULL,
            fixture_setup, test_fake_plugin_deactivate_removes_sensors,
            fixture_teardown);

  return g_test_run();
}
