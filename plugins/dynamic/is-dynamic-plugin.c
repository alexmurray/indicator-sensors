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
#include <indicator-sensors/is-log.h>
#include <glib/gi18n.h>

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsDynamicPlugin,
                               is_dynamic_plugin,
                               PEAS_TYPE_EXTENSION_BASE,
                               0,
                               G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
                                   peas_activatable_iface_init));

enum
{
  PROP_OBJECT = 1,
};

struct _IsDynamicPluginPrivate
{
  IsApplication *application;
};

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
sensor_value_notify_cb(GObject *object,
                       GParamSpec *pspec,
                       gpointer data)
{
  // TODO: track rate of change of sensor value - if this is greater than the
  // current one we are tracking then set this as the primary sensor in
  // gsettings - use hysteresis

}

static void
sensor_enabled_cb(IsManger *manager,
                  IsSensor *sensor,
                  gpointer data)
{
  // TODO: connect to notify::value property of sensor
}

static void
sensor_disabled_cb(IsManger *manager,
                  IsSensor *sensor,
                  gpointer data)
{
  // TODO: disconnect to notify::value property of sensor
}

static void
is_dynamic_plugin_activate(PeasActivatable *activatable)
{
  IsDynamicPlugin *self = IS_DYNAMIC_PLUGIN(activatable);
  IsDynamicPluginPrivate *priv = self->priv;
  IsManager *manager;
  GSList *sensors, *_list;

  is_debug("dynamic", "attaching to signals");

  manager = is_application_get_manager(priv->application);
  sensors = is_manager_get_enabled_sensors_list(manager);
  for (_list = sensors;
       _list != NULL;
       _list = _list->next)
  {
    IsSensor *sensor = IS_SENSOR(_list->data);
    g_signal_connect(sensor, "notify::value",
                     G_CALLBACK(sensor_value_notify_cb), self);
    g_object_unref(sensor);
  }
  g_slist_free(sensors);
  g_signal_connect(manager, "sensor-enabled",
                   G_CALLBACK(sensor_enabled_cb), self);
  g_signal_connect(manager, "sensor-disabled",
                   G_CALLBACK(sensor_disabled_cb), self);

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
  sensors = is_manager_get_enabled_sensors_list(manager);
  for (_list = sensors;
       _list != NULL;
       _list = _list->next)
  {
    IsSensor *sensor = IS_SENSOR(_list->data);
    g_signal_handlers_disconnect_by_func(sensor,
                                         G_CALLBACK(sensor_value_changed_cb),
                                         self);
    g_object_unref(sensor);
  }
  g_slist_free(sensors);
  g_signal_handlers_disconnect_by_func(manager,
                                       G_CALLBACK(sensor_enabled_cb), self);
  g_signal_handlers_disconnect_by_func(manager,
                                       G_CALLBACK(sensor_disabled_cb), self);

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
