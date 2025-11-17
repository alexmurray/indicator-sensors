/*
 * Copyright (C) 2014 Alex Murray <murray.alex@gmail.com>
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

#include "is-max-plugin.h"
#include <glib/gi18n.h>
#include <indicator-sensors/is-activatable.h>
#include <indicator-sensors/is-application.h>
#include <indicator-sensors/is-log.h>
#include <indicator-sensors/is-manager.h>
#include <stdlib.h>

typedef struct _IsMaxPluginPrivate
{
  IsApplication *application;
  IsSensor *sensor;
  IsSensor *max;
  gdouble max_value;
} IsMaxPluginPrivate;

static void is_activatable_iface_init(IsActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(
    IsMaxPlugin, is_max_plugin, G_TYPE_OBJECT, 0,
    G_ADD_PRIVATE_DYNAMIC(IsMaxPlugin)
        G_IMPLEMENT_INTERFACE_DYNAMIC(IS_TYPE_ACTIVATABLE,
                                      is_activatable_iface_init));

#define MAX_SENSOR_PATH "virtual/max"

enum
{
  PROP_APPLICATION = 1,
};

static void
is_max_plugin_set_property(GObject *object, guint prop_id, const GValue *value,
                           GParamSpec *pspec)
{
  IsMaxPlugin *plugin = IS_MAX_PLUGIN(object);
  IsMaxPluginPrivate *priv = is_max_plugin_get_instance_private(plugin);

  switch (prop_id)
  {
  case PROP_APPLICATION:
    priv->application = IS_APPLICATION(g_value_dup_object(value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
is_max_plugin_get_property(GObject *object, guint prop_id, GValue *value,
                           GParamSpec *pspec)
{
  IsMaxPlugin *plugin = IS_MAX_PLUGIN(object);
  IsMaxPluginPrivate *priv = is_max_plugin_get_instance_private(plugin);

  switch (prop_id)
  {
  case PROP_APPLICATION:
    g_value_set_object(value, priv->application);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
is_max_plugin_init(IsMaxPlugin *self)
{
}

static void
update_sensor_from_max(IsMaxPlugin *self)
{
  IsMaxPluginPrivate *priv;
  gchar *label;

  priv = is_max_plugin_get_instance_private(self);

  label = g_strdup_printf("↑%s", is_sensor_get_label(priv->max));
  is_sensor_set_label(priv->sensor, label);
  is_sensor_set_icon(priv->sensor, is_sensor_get_icon(priv->max));
  is_sensor_set_value(priv->sensor, is_sensor_get_value(priv->max));
  is_sensor_set_units(priv->sensor, is_sensor_get_units(priv->max));
  is_sensor_set_digits(priv->sensor, is_sensor_get_digits(priv->max));
  g_free(label);
}

static void
on_sensor_value_notify(IsSensor *sensor, GParamSpec *pspec, gpointer user_data)
{
  IsMaxPlugin *self;
  IsMaxPluginPrivate *priv;
  gdouble value;

  self = IS_MAX_PLUGIN(user_data);
  priv = is_max_plugin_get_instance_private(self);

  value = is_sensor_get_value(sensor);
  if (value - IS_SENSOR_VALUE_UNSET <= DBL_EPSILON)
  {
    is_debug("max", "sensor value for sensor %s is unset - ignoring",
             is_sensor_get_label(sensor));
    goto exit;
  }

  if (value > priv->max_value && sensor != priv->max)
  {
    // let's see if we can get away without taking a reference on sensor
    priv->max = sensor;

    is_message("max", "New highest value sensor: %s (value %f)",
               is_sensor_get_label(sensor), value);
  }

  if (sensor == priv->max)
  {
    priv->max_value = value;

    update_sensor_from_max(self);
  }

exit:
  return;
}

static void
on_sensor_enabled(IsManager *manager, IsSensor *sensor, gint index,
                  gpointer data)
{
  IsMaxPlugin *self = (IsMaxPlugin *)data;

  // don't bother monitoring non-temperature sensors
  if (IS_IS_TEMPERATURE_SENSOR(sensor))
  {
    is_debug("max", "sensor enabled: %s", is_sensor_get_label(sensor));
    on_sensor_value_notify(sensor, NULL, self);
    g_signal_connect(sensor, "notify::value",
                     G_CALLBACK(on_sensor_value_notify), self);
  }
}

static void
on_sensor_disabled(IsManager *manager, IsSensor *sensor, gpointer data)
{
  IsMaxPlugin *self = (IsMaxPlugin *)data;
  IsMaxPluginPrivate *priv = is_max_plugin_get_instance_private(self);

  // don't bother monitoring non-temperature sensors
  if (IS_IS_TEMPERATURE_SENSOR(sensor))
  {
    is_debug("max", "sensor disabled: %s", is_sensor_get_label(sensor));
    g_signal_handlers_disconnect_by_func(
        sensor, G_CALLBACK(on_sensor_value_notify), self);
    if (priv->max == sensor)
    {
      // get all sensors and find the one with the maximum value and switch to
      // this
      GSList *sensors, *_list;

      priv->max = NULL;
      priv->max_value = 0.0;

      is_sensor_set_label(priv->sensor, "Δ");
      is_sensor_set_icon(priv->sensor, IS_STOCK_CHIP);
      is_sensor_set_value(priv->sensor, 0.0);
      is_sensor_set_units(priv->sensor, "");
      is_sensor_set_digits(priv->sensor, 1);

      sensors = is_manager_get_enabled_sensors_list(manager);
      for (_list = sensors; _list != NULL; _list = _list->next)
      {
        if (IS_IS_TEMPERATURE_SENSOR(_list->data))
        {
          on_sensor_value_notify(IS_SENSOR(_list->data), NULL, self);
        }
      }
    }
  }
}

static void
is_max_plugin_activate(IsActivatable *activatable)
{
  IsMaxPlugin *self = IS_MAX_PLUGIN(activatable);
  IsMaxPluginPrivate *priv = is_max_plugin_get_instance_private(self);
  IsManager *manager;
  GSList *sensors, *_list;
  int i = 0;

  manager = is_application_get_manager(priv->application);

  // create our virtual sensor which mimics the current highest value sensor's
  // value and label
  is_debug("max", "creating virtual sensor");
  priv->sensor = is_sensor_new(MAX_SENSOR_PATH);
  is_sensor_set_label(priv->sensor, "Δ");
  is_sensor_set_icon(priv->sensor, IS_STOCK_CHIP);
  is_sensor_set_value(priv->sensor, 0.0);
  is_sensor_set_units(priv->sensor, "");
  is_sensor_set_digits(priv->sensor, 1);
  is_manager_add_sensor(manager, priv->sensor);

  is_debug("max", "attaching to signals");
  sensors = is_manager_get_enabled_sensors_list(manager);
  for (_list = sensors; _list != NULL; _list = _list->next)
  {
    IsSensor *sensor = IS_SENSOR(_list->data);
    on_sensor_enabled(manager, sensor, i, self);
    g_object_unref(sensor);
    i++;
  }
  g_slist_free(sensors);
  g_signal_connect(manager, "sensor-enabled", G_CALLBACK(on_sensor_enabled),
                   self);
  g_signal_connect(manager, "sensor-disabled", G_CALLBACK(on_sensor_disabled),
                   self);
}

static void
is_max_plugin_deactivate(IsActivatable *activatable)
{
  IsMaxPlugin *self = IS_MAX_PLUGIN(activatable);
  IsMaxPluginPrivate *priv = is_max_plugin_get_instance_private(self);
  IsManager *manager;
  GSList *sensors, *_list;

  is_debug("max", "dettaching from signals");

  manager = is_application_get_manager(priv->application);

  is_manager_remove_path(manager, MAX_SENSOR_PATH);
  sensors = is_manager_get_enabled_sensors_list(manager);
  for (_list = sensors; _list != NULL; _list = _list->next)
  {
    IsSensor *sensor = IS_SENSOR(_list->data);
    on_sensor_disabled(manager, sensor, self);
    g_object_unref(sensor);
  }
  g_slist_free(sensors);
  g_signal_handlers_disconnect_by_func(manager, G_CALLBACK(on_sensor_enabled),
                                       self);
  g_signal_handlers_disconnect_by_func(manager, G_CALLBACK(on_sensor_disabled),
                                       self);
}

static void
is_max_plugin_class_init(IsMaxPluginClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->get_property = is_max_plugin_get_property;
  gobject_class->set_property = is_max_plugin_set_property;

  g_object_class_override_property(gobject_class, PROP_APPLICATION,
                                   "application");
}

static void
is_activatable_iface_init(IsActivatableInterface *iface)
{
  iface->activate = is_max_plugin_activate;
  iface->deactivate = is_max_plugin_deactivate;
}

static void
is_max_plugin_class_finalize(IsMaxPluginClass *klass)
{
  /* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  is_max_plugin_register_type(G_TYPE_MODULE(module));

  peas_object_module_register_extension_type(module, IS_TYPE_ACTIVATABLE,
                                             IS_TYPE_MAX_PLUGIN);
}
