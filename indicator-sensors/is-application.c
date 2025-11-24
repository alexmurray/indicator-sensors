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

#include "is-application.h"
#include "is-indicator.h"
#include "is-log.h"
#include "is-manager.h"
#include "is-preferences-dialog.h"
#include "is-sensor-dialog.h"
#include <glib/gi18n.h>

#define DESKTOP_FILENAME PACKAGE ".desktop"

typedef struct _IsApplicationPrivate
{
  IsManager *manager;
  IsIndicator *indicator;
  GtkWidget *prefs_dialog;
  guint poll_timeout;
  glong poll_timeout_id;
  GFileMonitor *monitor;
  IsTemperatureSensorScale temperature_scale;
  GKeyFile *sensor_config;
  guint idle_write_id;
} IsApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IsApplication, is_application, G_TYPE_APPLICATION);

static void is_application_dispose(GObject *object);
static void is_application_finalize(GObject *object);
static void is_application_startup(GApplication *application);
static void is_application_activate(GApplication *application);
static void is_application_get_property(GObject *object, guint property_id,
                                        GValue *value, GParamSpec *pspec);
static void is_application_set_property(GObject *object, guint property_id,
                                        const GValue *value, GParamSpec *pspec);

// poll timeout should be less than the alarm timeout / hysteresis to ensure we
// get at least one more reading in before showing the notification otherwise
// there is no hysteresis
#define DEFAULT_POLL_TIMEOUT 4

/* properties */
enum
{
  PROP_MANAGER = 1,
  PROP_POLL_TIMEOUT,
  PROP_AUTOSTART,
  PROP_TEMPERATURE_SCALE,
  LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = {NULL};

static void
is_application_class_init(IsApplicationClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GApplicationClass *gapplication_class = G_APPLICATION_CLASS(klass);

  gobject_class->get_property = is_application_get_property;
  gobject_class->set_property = is_application_set_property;
  gobject_class->dispose = is_application_dispose;
  gobject_class->finalize = is_application_finalize;

  gapplication_class->startup = is_application_startup;
  gapplication_class->activate = is_application_activate;

  properties[PROP_MANAGER] = g_param_spec_object(
      "manager", "manager property", "manager property blurp.", IS_TYPE_MANAGER,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_MANAGER,
                                  properties[PROP_MANAGER]);

  properties[PROP_POLL_TIMEOUT] = g_param_spec_uint(
      "poll-timeout", "poll-timeout property", "poll-timeout property blurp.",
      0, G_MAXUINT, DEFAULT_POLL_TIMEOUT,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property(gobject_class, PROP_POLL_TIMEOUT,
                                  properties[PROP_POLL_TIMEOUT]);

  properties[PROP_AUTOSTART] =
      g_param_spec_boolean("autostart", "autostart property",
                           "start " PACKAGE " automatically on login.", FALSE,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property(gobject_class, PROP_AUTOSTART,
                                  properties[PROP_AUTOSTART]);

  /* TODO: convert to an enum type */
  properties[PROP_TEMPERATURE_SCALE] = g_param_spec_int(
      "temperature-scale", "temperature scale", "Sensor temperature scale.",
      IS_TEMPERATURE_SENSOR_SCALE_CELSIUS,
      IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT,
      IS_TEMPERATURE_SENSOR_SCALE_CELSIUS,
      G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_TEMPERATURE_SCALE,
                                  properties[PROP_TEMPERATURE_SCALE]);
}

static gboolean
update_sensors(IsApplication *self)
{
  IsApplicationPrivate *priv;
  GSList *enabled_sensors;
  gboolean ret;

  g_return_val_if_fail(IS_IS_APPLICATION(self), FALSE);

  priv = is_application_get_instance_private(self);
  enabled_sensors = is_manager_get_enabled_sensors_list(priv->manager);
  g_slist_foreach(enabled_sensors, (GFunc)is_sensor_update_value, NULL);
  g_slist_foreach(enabled_sensors, (GFunc)g_object_unref, NULL);
  /* return false if have no sensors left */
  ret = (g_slist_length(enabled_sensors) > 0);
  /* only keep going if have sensors to poll */
  if (!ret)
  {
    g_source_remove(priv->poll_timeout_id);
    priv->poll_timeout_id = 0;
  }
  g_slist_free(enabled_sensors);
  return ret;
}

static void
file_monitor_changed(GFileMonitor *monitor, GFile *file, GFile *other_file,
                     GFileMonitorEvent event_type, IsManager *self)
{
  g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_AUTOSTART]);
}

static void
is_application_init(IsApplication *self)
{
  IsApplicationPrivate *priv;
  gchar *path;
  GFile *file;
  GError *error = NULL;
  gboolean ret;

  priv = is_application_get_instance_private(self);

  g_application_set_application_id(G_APPLICATION(self),
                                   "com.github.alexmurray.IndicatorSensors");

  priv->poll_timeout = DEFAULT_POLL_TIMEOUT;
  path = g_build_filename(g_get_user_config_dir(), "autostart",
                          DESKTOP_FILENAME, NULL);
  file = g_file_new_for_path(path);
  priv->monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE, NULL, NULL);
  g_signal_connect(priv->monitor, "changed", G_CALLBACK(file_monitor_changed),
                   self);
  g_object_unref(file);
  g_free(path);
  priv->temperature_scale = IS_TEMPERATURE_SENSOR_SCALE_CELSIUS;

  priv->sensor_config = g_key_file_new();
  path = g_build_filename(g_get_user_config_dir(), PACKAGE, "sensors", NULL);
  ret = g_key_file_load_from_file(
      priv->sensor_config, path,
      G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error);
  if (!ret)
  {
    is_warning("manager", "Failed to load sensor configs from file %s: %s",
               path, error->message);
    g_error_free(error);
  }
  g_free(path);
}

static void
is_application_get_property(GObject *object, guint property_id, GValue *value,
                            GParamSpec *pspec)
{
  IsApplication *self = IS_APPLICATION(object);

  switch (property_id)
  {
  case PROP_MANAGER:
    g_value_set_object(value, is_application_get_manager(self));
    break;
  case PROP_POLL_TIMEOUT:
    g_value_set_uint(value, is_application_get_poll_timeout(self));
    break;
  case PROP_AUTOSTART:
    g_value_set_boolean(value, is_application_get_autostart(self));
    break;
  case PROP_TEMPERATURE_SCALE:
    g_value_set_int(value, is_application_get_temperature_scale(self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void
sensor_enabled(IsManager *manager, IsSensor *sensor, gint i,
               IsApplication *self)
{
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  /* get sensor to update and start polling if not already running */
  is_sensor_update_value(sensor);
  if (!priv->poll_timeout_id)
  {
    priv->poll_timeout_id = g_timeout_add_seconds(
        priv->poll_timeout, (GSourceFunc)update_sensors, self);
  }
}

static void
sensor_disabled(IsManager *manager, IsSensor *sensor, IsApplication *self)
{
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  if (!is_manager_get_num_enabled_sensors(priv->manager))
  {
    g_source_remove(priv->poll_timeout_id);
    priv->poll_timeout_id = 0;
  }
}

static void
restore_sensor_config(IsApplication *self, IsSensor *sensor)
{
  IsApplicationPrivate *priv = is_application_get_instance_private(self);
  gchar *label;
  gdouble alarm_value, low_value, high_value;
  IsSensorAlarmMode alarm_mode;
  GError *error = NULL;

  label = g_key_file_get_string(priv->sensor_config, is_sensor_get_path(sensor),
                                "label", &error);
  if (label)
  {
    is_sensor_set_label(sensor, label);
    g_free(label);
  }
  g_clear_error(&error);

  /* restore alarm mode before alarm value so we don't accidentally
     trigger an alarm just before we might disable it */
  alarm_mode = g_key_file_get_int64(
      priv->sensor_config, is_sensor_get_path(sensor), "alarm-mode", &error);
  if (!error)
  {
    is_sensor_set_alarm_mode(sensor, alarm_mode);
  }
  g_clear_error(&error);

  alarm_value = g_key_file_get_double(
      priv->sensor_config, is_sensor_get_path(sensor), "alarm-value", &error);
  if (!error)
  {
    is_sensor_set_alarm_value(sensor, alarm_value);
  }
  g_clear_error(&error);

  low_value = g_key_file_get_double(
      priv->sensor_config, is_sensor_get_path(sensor), "low-value", &error);
  if (!error)
  {
    is_sensor_set_low_value(sensor, low_value);
  }
  g_clear_error(&error);

  high_value = g_key_file_get_double(
      priv->sensor_config, is_sensor_get_path(sensor), "high-value", &error);
  if (!error)
  {
    is_sensor_set_high_value(sensor, high_value);
  }
  g_clear_error(&error);
}

static gboolean
write_out_sensor_config(IsApplication *self)
{
  IsApplicationPrivate *priv;
  gchar *data, *path;
  gsize len;
  GError *error = NULL;
  gboolean ret;

  priv = is_application_get_instance_private(self);
  if (!priv->idle_write_id)
  {
    is_warning("application",
               "Not writing out sensor config as has already occurred");
    goto out;
  }

  /* ensure directory exists */
  path = g_build_filename(g_get_user_config_dir(), PACKAGE, NULL);
  g_mkdir_with_parents(path, 0755);
  g_free(path);

  data = g_key_file_to_data(priv->sensor_config, &len, NULL);
  path = g_build_filename(g_get_user_config_dir(), PACKAGE, "sensors", NULL);
  ret = g_file_set_contents(path, data, len, &error);
  if (!ret)
  {
    is_warning("application", "Failed to write sensor config to file %s: %s",
               path, error->message);
    g_error_free(error);
  }
  g_free(path);
  g_free(data);

  /* make sure we are not called again as an idle callback */
out:
  priv->idle_write_id = 0;
  return FALSE;
}

static void
sensor_label_notify(IsSensor *sensor, GParamSpec *pspec, IsApplication *self)
{
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  g_key_file_set_string(priv->sensor_config, is_sensor_get_path(sensor),
                        "label", is_sensor_get_label(sensor));
  if (!priv->idle_write_id)
  {
    priv->idle_write_id =
        g_idle_add((GSourceFunc)write_out_sensor_config, self);
  }
}

static void
sensor_alarm_value_notify(IsSensor *sensor, GParamSpec *pspec,
                          IsApplication *self)
{
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  g_key_file_set_double(priv->sensor_config, is_sensor_get_path(sensor),
                        "alarm-value", is_sensor_get_alarm_value(sensor));
  if (!priv->idle_write_id)
  {
    priv->idle_write_id =
        g_idle_add((GSourceFunc)write_out_sensor_config, self);
  }
}

static void
sensor_alarm_mode_notify(IsSensor *sensor, GParamSpec *pspec,
                         IsApplication *self)
{
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  g_key_file_set_int64(priv->sensor_config, is_sensor_get_path(sensor),
                       "alarm-mode", is_sensor_get_alarm_mode(sensor));
  if (!priv->idle_write_id)
  {
    priv->idle_write_id =
        g_idle_add((GSourceFunc)write_out_sensor_config, self);
  }
}

static void
sensor_low_value_notify(IsSensor *sensor, GParamSpec *pspec,
                        IsApplication *self)
{
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  g_key_file_set_double(priv->sensor_config, is_sensor_get_path(sensor),
                        "low-value", is_sensor_get_low_value(sensor));
  if (!priv->idle_write_id)
  {
    priv->idle_write_id =
        g_idle_add((GSourceFunc)write_out_sensor_config, self);
  }
}

static void
sensor_high_value_notify(IsSensor *sensor, GParamSpec *pspec,
                         IsApplication *self)
{
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  g_key_file_set_double(priv->sensor_config, is_sensor_get_path(sensor),
                        "high-value", is_sensor_get_high_value(sensor));
  if (!priv->idle_write_id)
  {
    priv->idle_write_id =
        g_idle_add((GSourceFunc)write_out_sensor_config, self);
  }
}

static void
sensor_added(IsManager *manager, IsSensor *sensor, IsApplication *self)
{
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  /* set scale first before restoring config so we don't re-convert
   * fahrenheit values */
  if (IS_IS_TEMPERATURE_SENSOR(sensor))
  {
    is_temperature_sensor_set_scale(IS_TEMPERATURE_SENSOR(sensor),
                                    priv->temperature_scale);
  }
  restore_sensor_config(self, sensor);
  g_signal_connect(sensor, "notify::label", G_CALLBACK(sensor_label_notify),
                   self);
  g_signal_connect(sensor, "notify::alarm-value",
                   G_CALLBACK(sensor_alarm_value_notify), self);
  g_signal_connect(sensor, "notify::alarm-mode",
                   G_CALLBACK(sensor_alarm_mode_notify), self);
  g_signal_connect(sensor, "notify::low-value",
                   G_CALLBACK(sensor_low_value_notify), self);
  g_signal_connect(sensor, "notify::high-value",
                   G_CALLBACK(sensor_high_value_notify), self);
}

static void
sensor_removed(IsManager *manager, IsSensor *sensor, IsApplication *self)
{
  /* TODO: disconnect signal handlers */
}

static void
is_application_set_property(GObject *object, guint property_id,
                            const GValue *value, GParamSpec *pspec)
{
  IsApplication *self = IS_APPLICATION(object);
  IsApplicationPrivate *priv = is_application_get_instance_private(self);
  GSettings *settings;

  switch (property_id)
  {
  case PROP_MANAGER:
    g_assert(!priv->manager);
    priv->manager = g_object_ref(g_value_get_object(value));

    g_signal_connect(priv->manager, "sensor-added", G_CALLBACK(sensor_added),
                     self);
    g_signal_connect(priv->manager, "sensor-removed",
                     G_CALLBACK(sensor_removed), self);
    g_signal_connect(priv->manager, "sensor-enabled",
                     G_CALLBACK(sensor_enabled), self);
    g_signal_connect(priv->manager, "sensor-disabled",
                     G_CALLBACK(sensor_disabled), self);

    /* bind the enabled-sensors property of the manager as well so
     * we save / restore the list of enabled sensors too */
    settings = g_settings_new("indicator-sensors.manager");
    g_settings_bind(settings, "enabled-sensors", priv->manager,
                    "enabled-sensors", G_SETTINGS_BIND_DEFAULT);
    g_object_set_data_full(G_OBJECT(priv->manager), "gsettings", settings,
                           (GDestroyNotify)g_object_unref);
    break;
  case PROP_POLL_TIMEOUT:
    is_application_set_poll_timeout(self, g_value_get_uint(value));
    break;
  case PROP_AUTOSTART:
    is_application_set_autostart(self, g_value_get_boolean(value));
    break;
  case PROP_TEMPERATURE_SCALE:
    is_application_set_temperature_scale(self, g_value_get_int(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void
is_application_dispose(GObject *object)
{
  IsApplication *self = (IsApplication *)object;
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  if (priv->poll_timeout_id)
  {
    g_source_remove(priv->poll_timeout_id);
    priv->poll_timeout_id = 0;
  }
  g_object_unref(priv->monitor);

  if (priv->prefs_dialog)
  {
    gtk_widget_destroy(priv->prefs_dialog);
    priv->prefs_dialog = NULL;
  }
  G_OBJECT_CLASS(is_application_parent_class)->dispose(object);
}

static void
is_application_finalize(GObject *object)
{
  IsApplication *self = (IsApplication *)object;
  IsApplicationPrivate *priv = is_application_get_instance_private(self);

  g_object_unref(priv->manager);

  G_OBJECT_CLASS(is_application_parent_class)->finalize(object);
}

static void
on_prefs_action(GtkMenuItem *item, gpointer data)
{
  IsApplication *self = IS_APPLICATION(data);
  is_application_show_preferences(self);
}

static void
on_about_action(GtkMenuItem *item, gpointer data)
{
  IsApplication *self = IS_APPLICATION(data);
  is_application_show_about(self);
}

static void
on_quit_action(GtkMenuItem *item, gpointer data)
{
  IsApplication *self = IS_APPLICATION(data);
  is_application_quit(self);
}

static void
is_application_startup(GApplication *application)
{
  G_APPLICATION_CLASS(is_application_parent_class)->startup(application);

  is_debug("application", "Application starting up");

  IsApplication *self = IS_APPLICATION(application);
  IsApplicationPrivate *priv;

  priv = is_application_get_instance_private(IS_APPLICATION(application));

  GtkWidget *menu = gtk_menu_new();
  GtkWidget *separator_item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator_item);
  GtkWidget *prefs_item = gtk_menu_item_new_with_label(_("Preferences…"));
  g_signal_connect(prefs_item, "activate", G_CALLBACK(on_prefs_action), self);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), prefs_item);

  GtkWidget *about_item = gtk_menu_item_new_with_label(_("About…"));
  g_signal_connect(about_item, "activate", G_CALLBACK(on_about_action), self);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), about_item);

  GtkWidget *quit_item = gtk_menu_item_new_with_label(_("Quit"));
  g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit_action), self);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);
  gtk_widget_show_all(menu);

  priv->indicator = is_indicator_new(self, GTK_MENU(menu));

  GSettings *settings = g_settings_new("indicator-sensors.indicator");
  gchar *primary_sensor_path =
      g_settings_get_string(settings, "primary-sensor");
  is_indicator_set_primary_sensor_path(priv->indicator, primary_sensor_path);
  g_free(primary_sensor_path);
  is_indicator_set_display_flags(priv->indicator,
                                 g_settings_get_int(settings, "display-flags"));
  g_settings_bind(settings, "primary-sensor", priv->indicator,
                  "primary-sensor-path", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind(settings, "display-flags", priv->indicator, "display-flags",
                  G_SETTINGS_BIND_DEFAULT);
  g_object_set_data_full(G_OBJECT(priv->indicator), "gsettings", settings,
                         (GDestroyNotify)g_object_unref);
}

static void
is_application_activate(GApplication *application)
{
  G_APPLICATION_CLASS(is_application_parent_class)->activate(application);
  is_debug("application", "Application activated");
  g_application_hold(application);
}

static void
prefs_dialog_response(IsPreferencesDialog *dialog, gint response_id,
                      IsApplication *self)
{
  IsApplicationPrivate *priv;

  priv = is_application_get_instance_private(self);

  if (response_id == IS_PREFERENCES_DIALOG_RESPONSE_SENSOR_PROPERTIES)
  {
    IsSensor *sensor = is_manager_get_selected_sensor(priv->manager);
    if (sensor)
    {
      GtkWidget *sensor_dialog = is_sensor_dialog_new(sensor);
      g_signal_connect(sensor_dialog, "response",
                       G_CALLBACK(gtk_widget_destroy), NULL);
      g_signal_connect(sensor_dialog, "delete-event",
                       G_CALLBACK(gtk_widget_destroy), NULL);
      gtk_widget_show_all(sensor_dialog);
      g_object_unref(sensor);
    }
  }
  else
  {
    gtk_widget_hide(GTK_WIDGET(dialog));
  }
}

guint
is_application_get_poll_timeout(IsApplication *self)
{
  IsApplicationPrivate *priv;

  g_return_val_if_fail(IS_IS_APPLICATION(self), 0);

  priv = is_application_get_instance_private(self);
  return priv->poll_timeout;
}

void
is_application_set_poll_timeout(IsApplication *self, guint poll_timeout)
{
  IsApplicationPrivate *priv;

  g_return_if_fail(IS_IS_APPLICATION(self));

  priv = is_application_get_instance_private(self);
  if (priv->poll_timeout != poll_timeout)
  {
    priv->poll_timeout = poll_timeout;
    if (priv->poll_timeout_id)
    {
      g_source_remove(priv->poll_timeout_id);
      priv->poll_timeout_id = g_timeout_add_seconds(
          priv->poll_timeout, (GSourceFunc)update_sensors, self);
    }
    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_POLL_TIMEOUT]);
  }
}

IsApplication *
is_application_new(void)
{
  return g_object_new(IS_TYPE_APPLICATION, "manager", is_manager_new(), NULL);
}

IsManager *
is_application_get_manager(IsApplication *self)
{
  IsApplicationPrivate *priv;
  g_return_val_if_fail(IS_IS_APPLICATION(self), NULL);

  priv = is_application_get_instance_private(self);
  return priv->manager;
}

#define AUTOSTART_KEY "X-GNOME-Autostart-enabled"

gboolean
is_application_get_autostart(IsApplication *self)
{
  GKeyFile *key_file;
  gchar *path;
  GError *error = NULL;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_APPLICATION(self), FALSE);

  key_file = g_key_file_new();
  path = g_build_filename(g_get_user_config_dir(), "autostart",
                          DESKTOP_FILENAME, NULL);
  ret = g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, &error);
  if (!ret)
  {
    is_debug("application", "Failed to load autostart desktop file '%s': %s",
             path, error->message);
    g_error_free(error);
    goto out;
  }
  ret = g_key_file_get_boolean(key_file, G_KEY_FILE_DESKTOP_GROUP,
                               AUTOSTART_KEY, &error);
  if (error)
  {
    is_debug("application",
             "Failed to get key '%s' from autostart desktop file '%s': %s",
             AUTOSTART_KEY, path, error->message);
    g_error_free(error);
  }

out:
  g_free(path);
  g_key_file_free(key_file);
  return ret;
}

void
is_application_set_autostart(IsApplication *self, gboolean autostart)
{
  GKeyFile *key_file;
  gboolean ret;
  gchar *autostart_file;
  GError *error = NULL;

  g_return_if_fail(IS_IS_APPLICATION(self));

  key_file = g_key_file_new();
  autostart_file = g_build_filename(g_get_user_config_dir(), "autostart",
                                    DESKTOP_FILENAME, NULL);
  ret = g_key_file_load_from_file(
      key_file, autostart_file,
      G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error);
  if (!ret)
  {
    gchar *application_file;
    gchar *autostart_dir;

    is_debug("application", "Failed to load autostart desktop file '%s': %s",
             autostart_file, error->message);
    g_clear_error(&error);
    /* make sure autostart dir exists */
    autostart_dir =
        g_build_filename(g_get_user_config_dir(), "autostart", NULL);
    ret = g_mkdir_with_parents(autostart_dir, 0755);
    if (ret != 0)
    {
      is_warning("application", "Failed to create autostart directory '%s'",
                 autostart_dir);
    }
    g_free(autostart_dir);

    application_file = g_build_filename("applications", DESKTOP_FILENAME, NULL);
    ret = g_key_file_load_from_data_dirs(
        key_file, application_file, NULL,
        G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error);
    if (!ret)
    {
      is_warning("application", "Failed to load application desktop file: %s",
                 error->message);
      g_clear_error(&error);
      /* create file by hand */
      g_key_file_set_string(key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_TYPE, "Application");
      g_key_file_set_string(key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_NAME, _(PACKAGE_NAME));
      g_key_file_set_string(key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME,
                            _(PACKAGE_NAME));
      g_key_file_set_string(key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_EXEC, PACKAGE);
      g_key_file_set_string(key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_ICON, PACKAGE);
      g_key_file_set_string(key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_CATEGORIES, "System;");
    }
    g_free(application_file);
  }
  g_key_file_set_boolean(key_file, G_KEY_FILE_DESKTOP_GROUP, AUTOSTART_KEY,
                         autostart);
  ret = g_file_set_contents(
      autostart_file, g_key_file_to_data(key_file, NULL, NULL), -1, &error);
  if (!ret)
  {
    is_warning("application", "Failed to write autostart desktop file '%s': %s",
               autostart_file, error->message);
    g_clear_error(&error);
  }
  g_free(autostart_file);
  g_key_file_free(key_file);
}

IsTemperatureSensorScale
is_application_get_temperature_scale(IsApplication *self)
{
  IsApplicationPrivate *priv;
  g_return_val_if_fail(IS_IS_APPLICATION(self),
                       IS_TEMPERATURE_SENSOR_SCALE_INVALID);

  priv = is_application_get_instance_private(self);
  return priv->temperature_scale;
}

static gboolean
set_temperature_sensor_scale_and_unref(IsSensor *sensor,
                                       IsTemperatureSensorScale *scale)
{
  if (sensor)
  {
    if (IS_IS_TEMPERATURE_SENSOR(sensor))
    {
      is_temperature_sensor_set_scale(IS_TEMPERATURE_SENSOR(sensor), *scale);
    }
    g_object_unref(sensor);
  }
  return FALSE;
}

void
is_application_set_temperature_scale(IsApplication *self,
                                     IsTemperatureSensorScale scale)
{
  IsApplicationPrivate *priv;

  g_return_if_fail(IS_IS_APPLICATION(self));
  g_return_if_fail(scale == IS_TEMPERATURE_SENSOR_SCALE_CELSIUS ||
                   scale == IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT);

  priv = is_application_get_instance_private(self);

  /* set scale on all temperature sensors */
  if (priv->temperature_scale != scale)
  {
    priv->temperature_scale = scale;
    GSList *sensors = is_manager_get_all_sensors_list(priv->manager);
    g_slist_foreach(sensors, (GFunc)set_temperature_sensor_scale_and_unref,
                    &priv->temperature_scale);
    g_slist_free(sensors);

    g_object_notify_by_pspec(G_OBJECT(self),
                             properties[PROP_TEMPERATURE_SCALE]);
  }
}

void
is_application_show_preferences(IsApplication *self)
{
  IsApplicationPrivate *priv;

  g_return_if_fail(IS_IS_APPLICATION(self));

  priv = is_application_get_instance_private(self);

  if (!priv->prefs_dialog)
  {
    priv->prefs_dialog = is_preferences_dialog_new(self);
    g_signal_connect(priv->prefs_dialog, "response",
                     G_CALLBACK(prefs_dialog_response), self);
    g_signal_connect(priv->prefs_dialog, "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    gtk_widget_show_all(priv->prefs_dialog);
  }
  gtk_window_present(GTK_WINDOW(priv->prefs_dialog));
}

void
is_application_show_about(IsApplication *self)
{
  const gchar *const authors[] = {"Alex Murray <murray.alex@gmail.com>", NULL};

  g_return_if_fail(IS_IS_APPLICATION(self));

  gtk_show_about_dialog(
      NULL, "program-name", _("Hardware Sensors Indicator"), "authors", authors,
      "comments", _("Indicator for GNOME / Unity showing hardware sensors."),
      "copyright", "Copyright © 2011-2025 Alex Murray", "logo-icon-name",
      PACKAGE, "version", VERSION, "website",
      "https://github.com/alexmurray/indicator-sensors", NULL);
}

void
is_application_quit(IsApplication *self)
{
  g_return_if_fail(IS_IS_APPLICATION(self));
  g_application_release(G_APPLICATION(self));
}

gboolean
is_application_get_show_indicator(IsApplication *self)
{
  IsApplicationPrivate *priv;

  g_return_val_if_fail(IS_IS_APPLICATION(self), FALSE);

  priv = is_application_get_instance_private(self);
  return priv->indicator != NULL;
}
