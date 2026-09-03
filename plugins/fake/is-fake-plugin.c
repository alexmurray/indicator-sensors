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

#include "is-fake-plugin.h"
#include <glib/gi18n.h>
#include <indicator-sensors/is-activatable.h>
#include <indicator-sensors/is-application.h>
#include <indicator-sensors/is-fan-sensor.h>
#include <indicator-sensors/is-temperature-sensor.h>

#define FAKE_PATH_PREFIX "fake"

typedef struct _IsFakePluginPrivate
{
  IsApplication *application;
  GRand *rand;
} IsFakePluginPrivate;

static void is_activatable_iface_init(IsActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(
    IsFakePlugin, is_fake_plugin, G_TYPE_OBJECT, 0,
    G_ADD_PRIVATE_DYNAMIC(IsFakePlugin)
        G_IMPLEMENT_INTERFACE_DYNAMIC(IS_TYPE_ACTIVATABLE,
                                      is_activatable_iface_init));

enum
{
  PROP_APPLICATION = 1,
};

static void is_fake_plugin_finalize(GObject *object);

static void
is_fake_plugin_set_property(GObject *object, guint prop_id, const GValue *value,
                            GParamSpec *pspec)
{
  IsFakePlugin *plugin = IS_FAKE_PLUGIN(object);
  IsFakePluginPrivate *priv = is_fake_plugin_get_instance_private(plugin);

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
is_fake_plugin_get_property(GObject *object, guint prop_id, GValue *value,
                            GParamSpec *pspec)
{
  IsFakePlugin *plugin = IS_FAKE_PLUGIN(object);
  IsFakePluginPrivate *priv = is_fake_plugin_get_instance_private(plugin);

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
is_fake_plugin_init(IsFakePlugin *self)
{
  IsFakePluginPrivate *priv = is_fake_plugin_get_instance_private(self);

  priv->rand = g_rand_new();
}

static void
is_fake_plugin_finalize(GObject *object)
{
  IsFakePlugin *self = IS_FAKE_PLUGIN(object);
  IsFakePluginPrivate *priv = is_fake_plugin_get_instance_private(self);

  g_rand_free(priv->rand);

  if (priv->application)
  {
    g_object_unref(priv->application);
    priv->application = NULL;
  }

  G_OBJECT_CLASS(is_fake_plugin_parent_class)->finalize(object);
}

static void
update_sensor_value(IsSensor *sensor, IsFakePlugin *self)
{
  IsFakePluginPrivate *priv = is_fake_plugin_get_instance_private(self);
  /* fake value */
  is_sensor_set_value(
      sensor, g_rand_double_range(priv->rand, is_sensor_get_low_value(sensor),
                                  is_sensor_get_high_value(sensor)));
}

static void
is_fake_plugin_activate(IsActivatable *activatable)
{
  IsFakePlugin *self = IS_FAKE_PLUGIN(activatable);
  IsFakePluginPrivate *priv = is_fake_plugin_get_instance_private(self);
  int i;
  int n_fans = 0;

  /* generate some fake sensors */
  for (i = 0; i < 5; i++)
  {
    gchar *path;
    gchar *label;
    IsSensor *sensor;

    path = g_strdup_printf(FAKE_PATH_PREFIX "/sensor%d", i);
    if (g_rand_boolean(priv->rand))
    {
      n_fans++;
      sensor = is_fan_sensor_new(path);
      is_sensor_set_low_value(sensor, 100.0);
      is_sensor_set_high_value(sensor, 5000.0);
      label = g_strdup_printf(_("Fake Fan %d"), n_fans);
    }
    else
    {
      sensor = is_temperature_sensor_new(path);
      is_sensor_set_icon(sensor, IS_STOCK_CPU);
      label = g_strdup_printf(_("Fake CPU %d"), i - n_fans + 1);
    }
    /* no decimal places to display */
    is_sensor_set_digits(sensor, 0);
    is_sensor_set_label(sensor, label);
    /* connect to update-value signal */
    g_signal_connect(sensor, "update-value", G_CALLBACK(update_sensor_value),
                     self);
    is_manager_add_sensor(is_application_get_manager(priv->application),
                          sensor);
    g_free(label);
    g_free(path);
  }
}

static void
is_fake_plugin_deactivate(IsActivatable *activatable)
{
  IsFakePlugin *plugin = IS_FAKE_PLUGIN(activatable);
  IsFakePluginPrivate *priv = is_fake_plugin_get_instance_private(plugin);
  IsManager *manager;

  manager = is_application_get_manager(priv->application);
  is_manager_remove_paths_with_prefix(manager, FAKE_PATH_PREFIX);
}

static void
is_fake_plugin_class_init(IsFakePluginClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->get_property = is_fake_plugin_get_property;
  gobject_class->set_property = is_fake_plugin_set_property;
  gobject_class->finalize = is_fake_plugin_finalize;

  g_object_class_override_property(gobject_class, PROP_APPLICATION,
                                   "application");
}

static void
is_activatable_iface_init(IsActivatableInterface *iface)
{
  iface->activate = is_fake_plugin_activate;
  iface->deactivate = is_fake_plugin_deactivate;
}

static void
is_fake_plugin_class_finalize(IsFakePluginClass *klass)
{
  /* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  is_fake_plugin_register_type(G_TYPE_MODULE(module));

  peas_object_module_register_extension_type(module, IS_TYPE_ACTIVATABLE,
                                             IS_TYPE_FAKE_PLUGIN);
}
