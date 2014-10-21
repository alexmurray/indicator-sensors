/*
 * Copyright (C) 2011 Alex Murray <murray.alex@gmail.com>
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

#include "is-fake-plugin.h"
#include <stdlib.h>
#include <indicator-sensors/is-temperature-sensor.h>
#include <indicator-sensors/is-fan-sensor.h>
#include <indicator-sensors/is-application.h>
#include <glib/gi18n.h>

#define FAKE_PATH_PREFIX "fake"

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsFakePlugin,
                               is_fake_plugin,
                               PEAS_TYPE_EXTENSION_BASE,
                               0,
                               G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
                                   peas_activatable_iface_init));

enum
{
  PROP_OBJECT = 1,
};

struct _IsFakePluginPrivate
{
  IsApplication *application;
  GRand *rand;
};

static void is_fake_plugin_finalize(GObject *object);

static void
is_fake_plugin_set_property(GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
  IsFakePlugin *plugin = IS_FAKE_PLUGIN(object);

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
is_fake_plugin_get_property(GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
  IsFakePlugin *plugin = IS_FAKE_PLUGIN(object);

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
is_fake_plugin_init(IsFakePlugin *self)
{
  IsFakePluginPrivate *priv =
    G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_FAKE_PLUGIN,
                                IsFakePluginPrivate);

  self->priv = priv;
  priv->rand = g_rand_new();
}

static void
is_fake_plugin_finalize(GObject *object)
{
  IsFakePlugin *self = IS_FAKE_PLUGIN(object);
  IsFakePluginPrivate *priv = self->priv;

  g_rand_free(priv->rand);

  G_OBJECT_CLASS(is_fake_plugin_parent_class)->finalize(object);
}

static void
update_sensor_value(IsSensor *sensor,
                    IsFakePlugin *self)
{
  /* fake value */
  is_sensor_set_value(sensor,
                      g_rand_double_range(self->priv->rand,
                                          is_sensor_get_low_value(sensor),
                                          is_sensor_get_high_value(sensor)));
}

static void
is_fake_plugin_activate(PeasActivatable *activatable)
{
  IsFakePlugin *self = IS_FAKE_PLUGIN(activatable);
  IsFakePluginPrivate *priv = self->priv;
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
      label = g_strdup_printf(_("Fake Fan %d"),
                              n_fans);

    }
    else
    {
      sensor = is_temperature_sensor_new(path);
      is_sensor_set_icon(sensor, IS_STOCK_CPU);
      label = g_strdup_printf(_("Fake CPU %d"),
                              i - n_fans + 1);
    }
    /* no decimal places to display */
    is_sensor_set_digits(sensor, 0);
    is_sensor_set_label(sensor, label);
    /* connect to update-value signal */
    g_signal_connect(sensor, "update-value",
                     G_CALLBACK(update_sensor_value),
                     self);
    is_manager_add_sensor(is_application_get_manager(priv->application),
                          sensor);
    g_free(label);
    g_free(path);
  }
}

static void
is_fake_plugin_deactivate(PeasActivatable *activatable)
{
  IsFakePlugin *plugin = IS_FAKE_PLUGIN(activatable);
  IsFakePluginPrivate *priv = plugin->priv;
  IsManager *manager;

  manager = is_application_get_manager(priv->application);
  is_manager_remove_paths_with_prefix(manager, FAKE_PATH_PREFIX);
}

static void
is_fake_plugin_class_init(IsFakePluginClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass, sizeof(IsFakePluginPrivate));

  gobject_class->get_property = is_fake_plugin_get_property;
  gobject_class->set_property = is_fake_plugin_set_property;
  gobject_class->finalize = is_fake_plugin_finalize;

  g_object_class_override_property(gobject_class, PROP_OBJECT, "object");
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
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

  peas_object_module_register_extension_type(module,
      PEAS_TYPE_ACTIVATABLE,
      IS_TYPE_FAKE_PLUGIN);
}
