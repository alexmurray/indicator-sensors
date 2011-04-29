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

#include "is-nvidia-plugin.h"
#include "is-manager.h"
#include <stdlib.h>
#include <is-temperature-sensor.h>
#include <is-fan-sensor.h>
#include <is-manager.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#include <glib/gi18n.h>

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsNvidiaPlugin,
			       is_nvidia_plugin,
			       PEAS_TYPE_EXTENSION_BASE,
			       0,
			       G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
							     peas_activatable_iface_init));

enum {
	PROP_OBJECT = 1,
};

struct _IsNvidiaPluginPrivate
{
	IsManager *manager;
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

	switch (prop_id) {
	case PROP_OBJECT:
		plugin->priv->manager = IS_MANAGER(g_value_dup_object(value));
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

	switch (prop_id) {
	case PROP_OBJECT:
		g_value_set_object(value, plugin->priv->manager);
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
        if (priv->display != NULL) {
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
	if (priv->inited) {
		XCloseDisplay(priv->display);
		priv->inited = FALSE;
	}
	G_OBJECT_CLASS(is_nvidia_plugin_parent_class)->finalize(object);
}

struct map_entry
{
	gint target;
	gint attribute;
	const gchar *description;
};

static const struct map_entry map[] = {
	{ NV_CTRL_TARGET_TYPE_GPU, NV_CTRL_GPU_CORE_TEMPERATURE, "CoreTemp" },
	{ NV_CTRL_TARGET_TYPE_GPU, NV_CTRL_AMBIENT_TEMPERATURE, "AmbientTemp" },
	{ NV_CTRL_TARGET_TYPE_THERMAL_SENSOR, NV_CTRL_THERMAL_SENSOR_READING, "ThermalSensor" },
	{ NV_CTRL_TARGET_TYPE_COOLER, NV_CTRL_THERMAL_COOLER_LEVEL, "Fan" },
};

#define NVIDIA_PATH_PREFIX "nvidia/GPU"

static void
update_sensor_value(IsSensor *sensor,
		    IsNvidiaPlugin *self)
{
	IsNvidiaPluginPrivate *priv;
	const gchar *path;
	guint i;
	gint n;

	priv = self->priv;

	path = is_sensor_get_path(sensor);
	n = g_ascii_strtoll(path + strlen(NVIDIA_PATH_PREFIX), NULL, 10);

	for (i = 0; i < G_N_ELEMENTS(map); i++) {
		Bool ret;
		int value;
		if (g_strrstr(path, map[i].description) == NULL) {
			continue;
		}
		ret = XNVCTRLQueryTargetAttribute(priv->display,
						  map[i].target,
						  n,
						  0,
						  map[i].attribute,
						  &value);
		if (!ret) {
			GError *error = g_error_new(g_quark_from_string("nvidia-plugin-error-quark"),
						    0,
						    /* first placeholder is
						     * sensor name */
						    _("Error getting sensor value for sensor %s"),
						    path);
			is_sensor_emit_error(sensor, error);
			g_error_free(error);
			continue;
		}
		is_sensor_set_value(sensor, (gdouble)value);
	}
}

static void
is_nvidia_plugin_activate(PeasActivatable *activatable)
{
	IsNvidiaPlugin *self = IS_NVIDIA_PLUGIN(activatable);
	IsNvidiaPluginPrivate *priv = self->priv;
	Bool ret;
	int event_base, error_base;
	guint i;
	gint n;

	/* search for sensors and add them to manager */
	if (!priv->inited) {
		g_warning("nvidia is not inited, unable to find sensors");
		goto out;
	}

	g_debug("searching for sensors");

	/* check if the NV-CONTROL extension is available on this X
         * server */
	ret = XNVCTRLQueryExtension(priv->display, &event_base, &error_base);
	if (!ret) {
		goto out;
	}

	for (i = 0; i < G_N_ELEMENTS(map); i++) {
		ret = XNVCTRLQueryTargetCount(priv->display,
					      map[i].target,
					      &n);
		if (ret) {
			int j;
			for (j = 0; j < n; j++) {
				gint value;
				IsSensor *sensor;
				gchar *path, *label;

				ret = XNVCTRLQueryTargetAttribute(priv->display,
								  map[i].target,
								  j,
								  0,
								  map[i].attribute,
								  &value);
				if (!ret) {
					continue;
				}

				path = g_strdup_printf(NVIDIA_PATH_PREFIX "%d%s", j, map[i].description);
				label = g_strdup_printf("GPU%d %s", j, map[i].description);
				if (map[i].target == NV_CTRL_TARGET_TYPE_COOLER) {
					sensor = is_fan_sensor_new(path, label);
				} else {
					sensor = is_temperature_sensor_new(path, label);
				}
				/* connect to update-value signal */
				g_signal_connect(sensor, "update-value",
						 G_CALLBACK(update_sensor_value),
						 self);
				is_manager_add_sensor(priv->manager, sensor);
				g_free(label);
				g_free(path);
			}
		}
        }

out:
	return;
}

static void
is_nvidia_plugin_deactivate(PeasActivatable *activatable)
{
	IsNvidiaPlugin *plugin = IS_NVIDIA_PLUGIN(activatable);
	IsNvidiaPluginPrivate *priv = plugin->priv;

	(void)priv;

	/* TODO: remove sensors from manager since we are being unloaded */
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
