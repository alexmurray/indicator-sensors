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
#include "is-log.h"
#include "is-notify.h"
#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libpeas.h>
#include <locale.h>

static gboolean verbose = FALSE;

static void
on_extension_added(PeasExtensionSet *set, PeasPluginInfo *info,
                   IsActivatable *activatable, IsApplication *application)
{
  is_debug("main", "Activating plugin: %s", peas_plugin_info_get_name(info));
  is_activatable_activate(activatable);
}

static void
on_extension_removed(PeasExtensionSet *set, PeasPluginInfo *info,
                     IsActivatable *activatable, IsApplication *application)
{
  is_debug("main", "Deactivating plugin: %s", peas_plugin_info_get_name(info));
  is_activatable_deactivate(activatable);
}

static void
on_plugin_list_items_changed(GListModel *model, guint position, guint removed,
                             guint added, gpointer user_data)
{
  guint n = g_list_model_get_n_items(model);

  for (guint i = 0; i < n; i++)
  {
    PeasPluginInfo *info = PEAS_PLUGIN_INFO(g_list_model_get_item(model, i));
    is_debug("main", "Loading plugin: %s", peas_plugin_info_get_name(info));
    peas_engine_load_plugin(PEAS_ENGINE(model), info);
  }
}

static void
clear_sensor_icon_cache(void)
{
  GDir *dir = NULL;
  const gchar *filename;
  gchar *cache_dir =
      g_build_filename(g_get_user_cache_dir(), PACKAGE, "icons", NULL);

  dir = g_dir_open(cache_dir, 0, NULL);
  if (dir == NULL)
  {
    is_error("main", "Failed to open icon cache dir %s\n", cache_dir);
    goto exit;
  }

  for (filename = g_dir_read_name(dir); filename != NULL;
       filename = g_dir_read_name(dir))
  {
    gchar *path = g_build_filename(cache_dir, filename, NULL);
    int ret = g_remove(path);
    if (ret < 0)
    {
      is_error("main", "Error removing icon cache file: %s\n", path);
    }
    g_free(path);
  }
  g_dir_close(dir);

exit:
  g_free(cache_dir);
  return;
}

static GOptionEntry options[] = {{"verbose", 'v', 0, G_OPTION_ARG_NONE,
                                  &verbose, "Print more verbose debug output",
                                  NULL},
                                 {NULL}};

int
main(int argc, char **argv)
{
  GOptionContext *context;
  IsApplication *application;
  IsTemperatureSensorScale scale;
  GSettings *settings;
  IsManager *manager;
  gchar *plugin_dir;
  PeasEngine *engine;
  PeasExtensionSet *set;
  GError *error = NULL;
  gchar *locale_dir;

  /* Setup locale/gettext */
  setlocale(LC_ALL, "");

  locale_dir = g_build_filename(DATADIR, "locale", NULL);
  bindtextdomain(GETTEXT_PACKAGE, locale_dir);
  g_free(locale_dir);

  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  context = g_option_context_new("- hardware sensors monitor");
  g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);
  g_option_context_add_group(context, gtk_get_option_group(TRUE));

  if (!g_option_context_parse(context, &argc, &argv, &error))
  {
    g_print("command-line option parsing failed: %s\n", error->message);
    goto exit;
  }

  if (verbose)
  {
    is_log_set_level(IS_LOG_LEVEL_DEBUG);
  }

  /* clear the sensor icon cache directory to use any new icon theme */
  clear_sensor_icon_cache();

  gtk_init(&argc, &argv);

  /* make sure we create the application with the default settings */
  settings = g_settings_new("indicator-sensors.application");
  scale = g_settings_get_int(settings, "temperature-scale");
  application = g_object_new(IS_TYPE_APPLICATION, "manager", is_manager_new(),
                             "temperature-scale", scale, NULL);
  g_settings_bind(settings, "temperature-scale", application,
                  "temperature-scale", G_SETTINGS_BIND_DEFAULT);

  engine = peas_engine_get_default();
  g_signal_connect(engine, "items-changed",
                   G_CALLBACK(on_plugin_list_items_changed), NULL);
  /* add home dir to search path */
  plugin_dir =
      g_build_filename(g_get_user_config_dir(), PACKAGE, "plugins", NULL);
  is_debug("main", "Adding user plugin dir to search path: %s", plugin_dir);
  peas_engine_add_search_path(engine, plugin_dir, NULL);
  g_free(plugin_dir);

  /* add system path to search path */
  plugin_dir = g_build_filename(LIBDIR, PACKAGE, "plugins", NULL);
  is_debug("main", "Adding system plugin dir to search path: %s", plugin_dir);
  peas_engine_add_search_path(engine, plugin_dir, NULL);
  g_free(plugin_dir);

  /* create extension set and set manager as object */
  set = peas_extension_set_new(engine, IS_TYPE_ACTIVATABLE, "application",
                               application, NULL);

  /* activate all activatable extensions */
  peas_extension_set_foreach(
      set, (PeasExtensionSetForeachFunc)on_extension_added, application);

  /* and make sure to activate any ones which are found in the future */
  g_signal_connect(set, "extension-added", G_CALLBACK(on_extension_added),
                   application);
  g_signal_connect(set, "extension-removed", G_CALLBACK(on_extension_removed),
                   application);

  /* since all plugins are now inited show a notification if we detected
   * sensors but none are enabled - TODO: perhaps just open the pref's
   * dialog?? */
  manager = is_application_get_manager(application);
  GSList *sensors = is_manager_get_all_sensors_list(manager);
  if (sensors)
  {
    gchar **enabled_sensors = is_manager_get_enabled_sensors(manager);
    if (!g_strv_length(enabled_sensors))
    {
      GNotification *notification = is_notify(
          "no-sensors-enabled", IS_NOTIFY_LEVEL_INFO,
          _("No Sensors Enabled For Monitoring"),
          _("Sensors detected but none are enabled for monitoring. "
            "To enable monitoring of sensors open the Preferences "
            "window and select the sensors to monitor"));
      g_object_unref(notification);
    }
    g_strfreev(enabled_sensors);
    g_slist_foreach(sensors, (GFunc)g_object_unref, NULL);
    g_slist_free(sensors);
  }

  g_application_run(G_APPLICATION(application), argc, argv);

  g_object_unref(application);

exit:
  g_option_context_free(context);
  return 0;
}
