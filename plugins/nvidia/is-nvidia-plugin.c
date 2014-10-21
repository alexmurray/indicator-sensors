/*
 * Copyright (C) 2011 Alex Murray <alexmurray@fastmail.fm>
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

#include "is-nvidia-plugin.h"
#include <stdlib.h>
#include <indicator-sensors/is-temperature-sensor.h>
#include <indicator-sensors/is-fan-sensor.h>
#include <indicator-sensors/is-application.h>
#include <indicator-sensors/is-log.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#include <glib/gi18n.h>

#define NVIDIA_PATH_PREFIX "nvidia"

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsNvidiaPlugin,
                               is_nvidia_plugin,
                               PEAS_TYPE_EXTENSION_BASE,
                               0,
                               G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
                                   peas_activatable_iface_init));

enum
{
  PROP_OBJECT = 1,
};

struct _IsNvidiaPluginPrivate
{
  IsApplication *application;
  Display *display; /* the connection to the X server */

  gboolean inited;
  GHashTable *sensor_chip_names;
};

static void is_nvidia_plugin_finalize(GObject *object);

static void
is_nvidia_plugin_set_property(GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  IsNvidiaPlugin *plugin = IS_NVIDIA_PLUGIN(object);

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
is_nvidia_plugin_get_property(GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  IsNvidiaPlugin *plugin = IS_NVIDIA_PLUGIN(object);

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
is_nvidia_plugin_init(IsNvidiaPlugin *self)
{
  IsNvidiaPluginPrivate *priv =
    G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_NVIDIA_PLUGIN,
                                IsNvidiaPluginPrivate);

  self->priv = priv;
  priv->display = XOpenDisplay(NULL);
  if (priv->display != NULL)
  {
    priv->inited = TRUE;
  }
}

static void
is_nvidia_plugin_finalize(GObject *object)
{
  IsNvidiaPlugin *self = (IsNvidiaPlugin *)object;
  IsNvidiaPluginPrivate *priv = self->priv;

  /* think about storing this in the class structure so we only init once
     and unload once */
  if (priv->inited)
  {
    XCloseDisplay(priv->display);
    priv->inited = FALSE;
  }
  G_OBJECT_CLASS(is_nvidia_plugin_parent_class)->finalize(object);
}

struct map_entry
{
  gint gpu_attribute;
  gint target;
  gint attribute;
  const gchar *description;
};

static const struct map_entry map[] =
{
  /* use thermal sensors if available, otherwise use core and ambient
   * temp */
#ifdef NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU
  {
    NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU,
    NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,
    NV_CTRL_THERMAL_SENSOR_READING,
    "ThermalSensor"
  },
#else
  {
    -1,
    NV_CTRL_TARGET_TYPE_GPU,
    NV_CTRL_GPU_CORE_TEMPERATURE,
    "CoreTemp"
  },
  {
    -1,
    NV_CTRL_TARGET_TYPE_GPU,
    NV_CTRL_AMBIENT_TEMPERATURE,
    "AmbientTemp"
  },
#endif
  /* use thermal coolers if available */
#ifdef NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU
  {
    NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
    NV_CTRL_TARGET_TYPE_COOLER,
    NV_CTRL_THERMAL_COOLER_LEVEL,
    "Fan"
  },
#endif
};

static void
update_sensor_value(IsSensor *sensor,
                    IsNvidiaPlugin *self)
{
  IsNvidiaPluginPrivate *priv;
  const gchar *path;
  guint i;

  priv = self->priv;

  path = is_sensor_get_path(sensor);

  for (i = 0; i < G_N_ELEMENTS(map); i++)
  {
    Bool ret;
    int value;
    int idx;
    if (g_strrstr(path, map[i].description) == NULL)
    {
      continue;
    }
    idx = g_ascii_strtoll(g_strrstr(path, map[i].description) +
                          strlen(map[i].description), NULL, 10);

    ret = XNVCTRLQueryTargetAttribute(priv->display,
                                      map[i].target,
                                      idx,
                                      0,
                                      map[i].attribute,
                                      &value);
    if (!ret)
    {
      GError *error = g_error_new(g_quark_from_string("nvidia-plugin-error-quark"),
                                  0,
                                  /* first placeholder is
                                   * sensor name */
                                  _("Error getting sensor value for sensor %s"),
                                  is_sensor_get_label(sensor));
      is_sensor_set_error(sensor, error->message);
      g_error_free(error);
      continue;
    }
    if (IS_IS_TEMPERATURE_SENSOR(sensor))
    {
      is_temperature_sensor_set_celsius_value(IS_TEMPERATURE_SENSOR(sensor),
                                              value);
    }
    else
    {
      is_sensor_set_value(sensor, value);
    }
    is_sensor_set_error(sensor, NULL);
  }
}

static void
is_nvidia_plugin_activate(PeasActivatable *activatable)
{
  IsNvidiaPlugin *self = IS_NVIDIA_PLUGIN(activatable);
  IsNvidiaPluginPrivate *priv = self->priv;
  Bool ret;
  int event_base, error_base;
  gint n;
  int i;

  /* search for sensors and add them to manager */
  if (!priv->inited)
  {
    is_warning("nvidia", "not inited, unable to find sensors");
    goto out;
  }

  is_debug("nvidia", "searching for sensors");

  /* check if the NV-CONTROL extension is available on this X
   * server */
  ret = XNVCTRLQueryExtension(priv->display, &event_base, &error_base);
  if (!ret)
  {
    goto out;
  }

  /* get number of GPUs, then for each GPU get any thermal_sensors and
     coolers used by it */
  ret = XNVCTRLQueryTargetCount(priv->display,
                                NV_CTRL_TARGET_TYPE_GPU,
                                &n);
  if (!ret)
  {
    goto out;
  }

  for (i = 0; i < n; i++)
  {
    guint j;
    char *label = NULL;
    ret = XNVCTRLQueryTargetStringAttribute(priv->display,
                                            NV_CTRL_TARGET_TYPE_GPU,
                                            i,
                                            0,
                                            NV_CTRL_STRING_PRODUCT_NAME,
                                            &label);
    if (!ret)
    {
      free(label);
      continue;
    }

    for (j = 0; j < G_N_ELEMENTS(map); j++)
    {
      int32_t *data = NULL;
      int len;
      int k;

      /* old API has no binary targets (in which case
         gpu_attribute is -1) so ignore this and instead query
         for the number of targets */
      if (map[j].gpu_attribute == -1)
      {
        ret = XNVCTRLQueryTargetCount(priv->display,
                                      map[j].target,
                                      &k);
        if (!ret)
        {
          continue;
        }
        /* create fake binary data with first element as
        number of target's and each element the index of
        that element */
        data = malloc(sizeof(int32_t) * (k + 1));
        data[0] = k;
        while (--k > 0)
        {
          data[k] = k;
        }
      }
      else
      {
        ret = XNVCTRLQueryTargetBinaryData(priv->display,
                                           NV_CTRL_TARGET_TYPE_GPU,
                                           i,
                                           0,
                                           map[j].gpu_attribute,
                                           (unsigned char **)&data,
                                           &len);
        if (!ret)
        {
          continue;
        }
      }
      /* data[0] contains number of sensors, and each sensor
         indice follows */
      g_assert(data != NULL);
      for (k = 1; k <= data[0]; k++)
      {
        int32_t idx = data[k];
        gint value;
        IsSensor *sensor;
        gchar *path;

        /* if we are using the old API, requery for GPU
           name but using idx */
        if (map[j].gpu_attribute == -1)
        {
          gchar *old_label;
          ret = XNVCTRLQueryTargetStringAttribute(priv->display,
                                                  NV_CTRL_TARGET_TYPE_GPU,
                                                  idx,
                                                  0,
                                                  NV_CTRL_STRING_PRODUCT_NAME,
                                                  &old_label);
          if (ret)
          {
            free(label);
            label = old_label;
          }
        }
        ret = XNVCTRLQueryTargetAttribute(priv->display,
                                          map[j].target,
                                          idx,
                                          0,
                                          map[j].attribute,
                                          &value);
        if (!ret)
        {
          continue;
        }

        path = g_strdup_printf(NVIDIA_PATH_PREFIX "/%s%d", map[j].description, idx);
#ifdef NV_CTRL_TARGET_TYPE_COOLER
        if (map[j].target == NV_CTRL_TARGET_TYPE_COOLER)
        {
          /* fan sensors are given as a percentage
             from 0 to 100 */
          sensor = is_sensor_new(path);
          is_sensor_set_icon(sensor, IS_STOCK_FAN);
          is_sensor_set_units(sensor, "%");
          is_sensor_set_low_value(sensor, 0.0);
          is_sensor_set_high_value(sensor, 100.0);
        }
        else
        {
#endif
          sensor = is_temperature_sensor_new(path);
          is_sensor_set_icon(sensor, IS_STOCK_GPU);
#ifdef NV_CTRL_TARGET_TYPE_COOLER
        }
#endif
        /* no decimal places to display */
        is_sensor_set_digits(sensor, 0);
        is_sensor_set_label(sensor, label);
        /* connect to update-value signal */
        g_signal_connect(sensor, "update-value",
                         G_CALLBACK(update_sensor_value),
                         self);
        is_manager_add_sensor(is_application_get_manager(priv->application),
                              sensor);
        g_free(path);
      }
      free(data);
    }
    free(label);
  }

out:
  return;
}

static void
is_nvidia_plugin_deactivate(PeasActivatable *activatable)
{
  IsNvidiaPlugin *plugin = IS_NVIDIA_PLUGIN(activatable);
  IsNvidiaPluginPrivate *priv = plugin->priv;
  IsManager *manager;

  manager = is_application_get_manager(priv->application);
  is_manager_remove_paths_with_prefix(manager, NVIDIA_PATH_PREFIX);
}

static void
is_nvidia_plugin_class_init(IsNvidiaPluginClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass, sizeof(IsNvidiaPluginPrivate));

  gobject_class->get_property = is_nvidia_plugin_get_property;
  gobject_class->set_property = is_nvidia_plugin_set_property;
  gobject_class->finalize = is_nvidia_plugin_finalize;

  g_object_class_override_property(gobject_class, PROP_OBJECT, "object");
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
{
  iface->activate = is_nvidia_plugin_activate;
  iface->deactivate = is_nvidia_plugin_deactivate;
}

static void
is_nvidia_plugin_class_finalize(IsNvidiaPluginClass *klass)
{
  /* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  is_nvidia_plugin_register_type(G_TYPE_MODULE(module));

  peas_object_module_register_extension_type(module,
      PEAS_TYPE_ACTIVATABLE,
      IS_TYPE_NVIDIA_PLUGIN);
}
