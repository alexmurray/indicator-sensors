/*
 * Copyright (C) 2014 Alex Murray <alexmurray@fastmail.fm>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "is-dynamic-plugin.h"
#include <stdlib.h>
#include <math.h>
#include <indicator-sensors/is-application.h>
#include <indicator-sensors/is-manager.h>
#include <indicator-sensors/is-log.h>
#include <glib/gi18n.h>

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsDynamicPlugin,
                               is_dynamic_plugin,
                               PEAS_TYPE_EXTENSION_BASE,
                               0,
                               G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
                                                             peas_activatable_iface_init));

#define DYNAMIC_RATE_DATA_KEY "dynamic-rate-data"

#define DYNAMIC_SENSOR_PATH "dynamic/virtual"

#define EWMA_ALPHA 0.2

enum
{
  PROP_OBJECT = 1,
};

struct _IsDynamicPluginPrivate
{
  IsApplication *application;
  IsSensor *sensor;
  IsSensor *max;
  gdouble max_rate;
};

typedef struct _RateData
{
  gdouble rate;
  gdouble last_value;
  gint64 last_time;
} RateData;

static void is_dynamic_plugin_finalize(GObject *object);

static void
is_dynamic_plugin_set_property(GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
  IsDynamicPlugin *plugin = IS_DYNAMIC_PLUGIN(object);

  switch (prop_id)
  {
    case PROP_OBJECT:
      plugin->priv->application = IS_APPLICATION(g_value_dup_object(value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
is_dynamic_plugin_get_property(GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
  IsDynamicPlugin *plugin = IS_DYNAMIC_PLUGIN(object);

  switch (prop_id)
  {
    case PROP_OBJECT:
      g_value_set_object(value, plugin->priv->application);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
is_dynamic_plugin_init(IsDynamicPlugin *self)
{
  IsDynamicPluginPrivate *priv =
    G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_DYNAMIC_PLUGIN,
                                IsDynamicPluginPrivate);

  self->priv = priv;
}

static void
is_dynamic_plugin_finalize(GObject *object)
{
  IsDynamicPlugin *self = (IsDynamicPlugin *)object;
  IsDynamicPluginPrivate *priv = self->priv;

  (void)priv;

  G_OBJECT_CLASS(is_dynamic_plugin_parent_class)->finalize(object);
}

static void
update_sensor_from_max(IsDynamicPlugin *self)
{
  IsDynamicPluginPrivate *priv;
  gchar *label;

  priv = self->priv;

  label = g_strdup_printf("Δ%s", is_sensor_get_label(priv->max));
  is_sensor_set_label(priv->sensor, label);
  is_sensor_set_icon(priv->sensor, is_sensor_get_icon(priv->max));
  is_sensor_set_value(priv->sensor, is_sensor_get_value(priv->max));
  is_sensor_set_units(priv->sensor, is_sensor_get_units(priv->max));
  is_sensor_set_digits(priv->sensor, is_sensor_get_digits(priv->max));
  g_free(label);
}

static void
on_sensor_value_notify(IsSensor *sensor,
                       GParamSpec *pspec,
                       gpointer user_data)
{
  IsDynamicPlugin *self;
  IsDynamicPluginPrivate *priv;
  RateData *data;
  gdouble value, dv, dt, rate;
  gint64 now;

  self = IS_DYNAMIC_PLUGIN(user_data);
  priv = self->priv;

  value = is_sensor_get_value(sensor);
  now = g_get_monotonic_time();

  data = g_object_get_data(G_OBJECT(sensor), DYNAMIC_RATE_DATA_KEY);
  if (data == NULL)
  {
    is_debug("dynamic", "Creating new dynamic rate data for sensor: %s",
             is_sensor_get_label(sensor));

    // allocate data
    data = g_malloc0(sizeof(*data));
    data->rate = 0.0f;
    data->last_value = value;
    data->last_time = now;
    g_object_set_data_full(G_OBJECT(sensor), DYNAMIC_RATE_DATA_KEY,
                           data, g_free);
    goto exit;
  }

  is_debug("dynamic", "Got existing rate data for sensor: %s - rate: %f, last_value %f, last_time %lld",
           is_sensor_get_label(sensor),
           data->rate,
           data->last_value,
           data->last_time);
  dv = value - data->last_value;
  dt = ((double)(now - data->last_time) /
        (double)G_USEC_PER_SEC);

  // convert rate to units per second
  rate = fabs(dv / dt);
  is_debug("dynamic", "abs rate of change of sensor %s: %f (t0: %f, t-1: %f, dv: %f, dt: %f)",
           is_sensor_get_label(sensor), rate, value, data->last_value,
           dv, dt);

  // calculate exponentially weighted moving average of rate
  rate = (EWMA_ALPHA * rate) + ((1 - EWMA_ALPHA) * data->rate);
  data->rate = rate;
  data->last_value = value;
  data->last_time = now;
  is_debug("dynamic", "EWMA abs rate of change of sensor %s: %f",
           is_sensor_get_label(sensor), rate);

  if (rate > priv->max_rate && sensor != priv->max)
  {
    // let's see if we can get away without taking a reference on sensor
    priv->max = sensor;

    is_message("dynamic", "New highest EWMA rate sensor: %s (rate %f)",
               is_sensor_get_label(sensor), rate);
  }

  if (sensor == priv->max)
  {
    priv->max_rate = rate;

    update_sensor_from_max(self);
  }

exit:
  return;
}

static void
on_sensor_enabled(IsManager *manager,
                  IsSensor *sensor,
                  gint index,
                  gpointer data)
{
  IsDynamicPlugin *self = (IsDynamicPlugin *)data;

  // don't bother monitoring ourself
  if (g_ascii_strcasecmp(is_sensor_get_path(sensor),
                         DYNAMIC_SENSOR_PATH) != 0)
    g_signal_connect(sensor, "notify::value",
                     G_CALLBACK(on_sensor_value_notify), self);
}

static void
on_sensor_disabled(IsManager *manager,
                   IsSensor *sensor,
                   gpointer data)
{
  IsDynamicPlugin *self = (IsDynamicPlugin *)data;
  IsDynamicPluginPrivate *priv = self->priv;

  // don't bother monitoring ourself
  if (g_ascii_strcasecmp(is_sensor_get_path(sensor),
                         DYNAMIC_SENSOR_PATH) != 0)
  {
    g_signal_handlers_disconnect_by_func(sensor,
                                         G_CALLBACK(on_sensor_value_notify),
                                         self);
    if (priv->max == sensor)
    {
      // get all sensors and find the one with the maximum rate and switch to
      // this
      GSList *sensors, *_list;

      priv->max = NULL;
      priv->max_rate = 0.0;

      is_sensor_set_label(priv->sensor, "Δ");
      is_sensor_set_icon(priv->sensor, IS_STOCK_CHIP);
      is_sensor_set_value(priv->sensor, 0.0);
      is_sensor_set_units(priv->sensor, "");
      is_sensor_set_digits(priv->sensor, 1);

      sensors = is_manager_get_enabled_sensors_list(manager);
      for (_list = sensors;
           _list != NULL;
           _list = _list->next)
      {
        RateData *rate_data = g_object_get_data(G_OBJECT(sensor),
                                                DYNAMIC_RATE_DATA_KEY);
        if (rate_data == NULL)
          continue;
        if (rate_data->rate > priv->max_rate)
        {
          priv->max_rate = rate_data->rate;
          priv->max = sensor;
        }
      }

      if (priv->max)
      {
        update_sensor_from_max(self);
      }
    }

  }
}

static void
is_dynamic_plugin_activate(PeasActivatable *activatable)
{
  IsDynamicPlugin *self = IS_DYNAMIC_PLUGIN(activatable);
  IsDynamicPluginPrivate *priv = self->priv;
  IsManager *manager;
  GSList *sensors, *_list;
  int i = 0;

  manager = is_application_get_manager(priv->application);

  // create our virtual sensor which mimics the current highest rate of change
  // sensor's value and label
  is_debug("dynamic", "creating virtual sensor");
  priv->sensor = is_sensor_new(DYNAMIC_SENSOR_PATH);
  is_sensor_set_label(priv->sensor, "Δ");
  is_sensor_set_icon(priv->sensor, IS_STOCK_CHIP);
  is_sensor_set_value(priv->sensor, 0.0);
  is_sensor_set_units(priv->sensor, "");
  is_sensor_set_digits(priv->sensor, 1);
  is_manager_add_sensor(manager, priv->sensor);

  is_debug("dynamic", "attaching to signals");
  sensors = is_manager_get_enabled_sensors_list(manager);
  for (_list = sensors;
       _list != NULL;
       _list = _list->next)
  {
    IsSensor *sensor = IS_SENSOR(_list->data);
    on_sensor_enabled(manager, sensor, i, self);
    g_object_unref(sensor);
    i++;
  }
  g_slist_free(sensors);
  g_signal_connect(manager, "sensor-enabled",
                   G_CALLBACK(on_sensor_enabled), self);
  g_signal_connect(manager, "sensor-disabled",
                   G_CALLBACK(on_sensor_disabled), self);

}

static void
is_dynamic_plugin_deactivate(PeasActivatable *activatable)
{
  IsDynamicPlugin *self = IS_DYNAMIC_PLUGIN(activatable);
  IsDynamicPluginPrivate *priv = self->priv;
  IsManager *manager;
  GSList *sensors, *_list;

  is_debug("dynamic", "dettaching from signals");

  manager = is_application_get_manager(priv->application);

  is_manager_remove_path(manager, DYNAMIC_SENSOR_PATH);
  sensors = is_manager_get_enabled_sensors_list(manager);
  for (_list = sensors;
       _list != NULL;
       _list = _list->next)
  {
    IsSensor *sensor = IS_SENSOR(_list->data);
    on_sensor_disabled(manager, sensor, self);
    g_object_unref(sensor);
  }
  g_slist_free(sensors);
  g_signal_handlers_disconnect_by_func(manager,
                                       G_CALLBACK(on_sensor_enabled), self);
  g_signal_handlers_disconnect_by_func(manager,
                                       G_CALLBACK(on_sensor_disabled), self);

}

static void
is_dynamic_plugin_class_init(IsDynamicPluginClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass, sizeof(IsDynamicPluginPrivate));

  gobject_class->get_property = is_dynamic_plugin_get_property;
  gobject_class->set_property = is_dynamic_plugin_set_property;
  gobject_class->finalize = is_dynamic_plugin_finalize;

  g_object_class_override_property(gobject_class, PROP_OBJECT, "object");
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
{
  iface->activate = is_dynamic_plugin_activate;
  iface->deactivate = is_dynamic_plugin_deactivate;
}

static void
is_dynamic_plugin_class_finalize(IsDynamicPluginClass *klass)
{
  /* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  is_dynamic_plugin_register_type(G_TYPE_MODULE(module));

  peas_object_module_register_extension_type(module,
                                             PEAS_TYPE_ACTIVATABLE,
                                             IS_TYPE_DYNAMIC_PLUGIN);
}
