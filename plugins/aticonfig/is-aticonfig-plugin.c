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

#include "is-aticonfig-plugin.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <indicator-sensors/is-temperature-sensor.h>
#include <indicator-sensors/is-fan-sensor.h>
#include <indicator-sensors/is-manager.h>
#include <indicator-sensors/is-log.h>
#include <glib/gi18n.h>

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsATIConfigPlugin,
			       is_aticonfig_plugin,
			       PEAS_TYPE_EXTENSION_BASE,
			       0,
			       G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
							     peas_activatable_iface_init));

enum {
	PROP_OBJECT = 1,
};

struct _IsATIConfigPluginPrivate
{
	IsManager *manager;
	GRegex *temperature_regex;
	GRegex *fanspeed_regex;
};

static void is_aticonfig_plugin_finalize(GObject *object);

static void
is_aticonfig_plugin_set_property(GObject *object,
					 guint prop_id,
					 const GValue *value,
					 GParamSpec *pspec)
{
	IsATIConfigPlugin *plugin = IS_ATICONFIG_PLUGIN(object);

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
is_aticonfig_plugin_get_property(GObject *object,
					 guint prop_id,
					 GValue *value,
					 GParamSpec *pspec)
{
	IsATIConfigPlugin *plugin = IS_ATICONFIG_PLUGIN(object);

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
is_aticonfig_plugin_init(IsATIConfigPlugin *self)
{
	GError *error = NULL;
	IsATIConfigPluginPrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_ATICONFIG_PLUGIN,
					    IsATIConfigPluginPrivate);

	self->priv = priv;
	priv->temperature_regex = g_regex_new(".*Sensor 0: Temperature - ([0-9|\\.]+) C",
			    0, 0, &error);
	if (!priv->temperature_regex) {
		is_warning("aticonfig", "Error compiling regex to read sensor temperatures: %s",
			   error->message);
		g_clear_error(&error);
	}

	priv->fanspeed_regex = g_regex_new(".*Fan Speed: ([0-9]+)%", 0, 0, &error);
	if (!priv->fanspeed_regex) {
		is_warning("aticonfig", "Error compiling regex to read sensor fanspeeds: %s",
			   error->message);
		g_clear_error(&error);
	}
}

static void
is_aticonfig_plugin_finalize(GObject *object)
{
	IsATIConfigPlugin *plugin = IS_ATICONFIG_PLUGIN(object);
	IsATIConfigPluginPrivate *priv = plugin->priv;

	if (priv->temperature_regex) {
		g_regex_unref(priv->temperature_regex);
	}
	if (priv->fanspeed_regex) {
		g_regex_unref(priv->fanspeed_regex);
	}
	G_OBJECT_CLASS(is_aticonfig_plugin_parent_class)->finalize(object);
}

#define ATICONFIG_PATH_PREFIX "aticonfig/GPU"

static gboolean aticonfig_get_temperature(IsATIConfigPlugin *self,
					  int gpu,
					  gdouble *result,
					  GError **error)
{
	gchar *command = NULL;
	gchar *output = NULL;
	GMatchInfo *match = NULL;
	gchar *temperature = NULL;
	gboolean ret;

	command = g_strdup_printf("aticonfig --od-gettemperature --adapter=%d", gpu);
	ret = g_spawn_command_line_sync(command, &output, NULL, NULL, error);
	if (!ret) {
		is_warning("aticonfig", "Error calling aticonfig to get temperature for GPU %d: %s",
			   gpu, (*error)->message);
		goto out;
	}

	ret = g_regex_match(self->priv->temperature_regex, output, 0, &match);
	if (!ret) {
		*error = g_error_new(g_quark_from_string("aticonfig-plugin-error-quark"),
				     0,
				     /* first placeholder is sensor name */
				     _("Error reading temperature value for GPU %d"),
				     gpu);
		goto out;
	}
	temperature = g_match_info_fetch(match, 1);
	*result = g_strtod(temperature, NULL);
	g_free(temperature);

out:
	g_match_info_free(match);
	g_free(output);
	g_free(command);
	return ret;
}

static gboolean aticonfig_get_fanspeed(IsATIConfigPlugin *self,
				       int gpu,
				       gdouble *result,
				       GError **error)
{
	gchar *command = NULL;
	gchar *output = NULL;
	GMatchInfo *match = NULL;
	gchar *fanspeed = NULL;
	gboolean ret;

	command = g_strdup_printf("aticonfig --pplib-cmd \"get fanspeed %d\"", gpu);
	ret = g_spawn_command_line_sync(command, &output, NULL, NULL, error);
	if (!ret) {
		is_warning("aticonfig", "Error calling aticonfig to get fanspeed for GPU %d: %s",
			   gpu, (*error)->message);
		goto out;
	}

	ret = g_regex_match(self->priv->fanspeed_regex, output, 0, &match);
	if (!ret) {
		*error = g_error_new(g_quark_from_string("aticonfig-plugin-error-quark"),
				     0,
				     /* first placeholder is sensor name */
				     _("Error reading fanspeed value for GPU %d"),
				     gpu);
		goto out;
	}
	fanspeed = g_match_info_fetch(match, 1);
	*result = g_strtod(fanspeed, NULL);
	g_free(fanspeed);

out:
	g_match_info_free(match);
	g_free(output);
	g_free(command);
	return ret;
}

static void
update_sensor_value(IsSensor *sensor,
		    IsATIConfigPlugin *self)
{
	const gchar *path;
	gint gpu;
	gdouble value = 0.0;
	gboolean ret = FALSE;
	GError *error = NULL;

	path = is_sensor_get_path(sensor);
	gpu = g_ascii_strtoll(path + strlen(ATICONFIG_PATH_PREFIX), NULL, 10);

	if (IS_IS_TEMPERATURE_SENSOR(sensor)) {
		ret = aticonfig_get_temperature(self, gpu, &value, &error);
		if (ret) {
			is_temperature_sensor_set_celsius_value(IS_TEMPERATURE_SENSOR(sensor),
								value);
		}
	} else {
		/* is a fan sensor */
		ret = aticonfig_get_fanspeed(self, gpu, &value, &error);
		if (ret) {
			is_sensor_set_value(sensor, value);
		}
	}
	/* emit any error which may have occurred */
	if (error) {
		is_sensor_emit_error(sensor, error);
		g_error_free(error);
	}
}

static void
is_aticonfig_plugin_activate(PeasActivatable *activatable)
{
	IsATIConfigPlugin *self = IS_ATICONFIG_PLUGIN(activatable);
	gchar *output = NULL;
	GError *error = NULL;
	GRegex *regex = NULL;
	GMatchInfo *match = NULL;
	gboolean ret;

	/* search for sensors and add them to manager */
	is_debug("aticonfig", "searching for sensors");

	/* call aticonfig with --list-adapters to get available adapters,
	 * then test if each can do temperature and fan speed - if so add
	 * appropriate sensors */
	ret = g_spawn_command_line_sync("aticonfig --list-adapters",
					&output, NULL, NULL, &error);
	if (!ret) {
		is_warning("aticonfig", "Error calling aticonfig to detect available sensors: %s",
			   error->message);
		g_error_free(error);
		goto out;
	}

	regex = g_regex_new("^.*([0-9]+)\\. ([0-9][0-9]:[0-9][0-9]\\.[0-9]) (.*)$",
			    G_REGEX_MULTILINE, 0, &error);
	if (!regex) {
		is_warning("aticonfig", "Error compiling regex to detect listed sensors: %s",
			   error->message);
		g_error_free(error);
		goto out;
	}

	ret = g_regex_match(regex, output, 0, &match);
	if (!ret) {
		is_warning("aticonfig", "No sensors found in aticonfig output: %s", output);
		goto out;
	}
	while (g_match_info_matches(match)) {
		gint i;
		gchar *idx, *pci, *name;
		gdouble value;
		gchar *path;
		IsSensor *sensor;

		idx = g_match_info_fetch(match, 1);
		pci = g_match_info_fetch(match, 2);
		name = g_match_info_fetch(match, 3);

		i = g_ascii_strtoull(idx, NULL, 10);
		/* we have an adapter - see if we can get its temperature and
		   fanspeed */
		ret = aticonfig_get_temperature(self, i, &value, &error);
		if (!ret) {
			is_warning("aticonfig", "Error getting temperature for adapter %d: %s",
				   i, error->message);
			g_clear_error(&error);
		} else {
			path = g_strdup_printf("%s%d%s", ATICONFIG_PATH_PREFIX, i, _("Temperature"));
			sensor = is_temperature_sensor_new(path);
			is_sensor_set_label(sensor, name);
			is_sensor_set_icon(sensor, IS_STOCK_GPU);
			g_signal_connect(sensor, "update-value",
					 G_CALLBACK(update_sensor_value),
					 self);
			is_manager_add_sensor(self->priv->manager, sensor);
			g_object_unref(sensor);
			g_free(path);
		}

		ret = aticonfig_get_fanspeed(self, i, &value, &error);
		if (!ret) {
			is_warning("aticonfig", "Error getting fanpeed for adapter %d: %s",
				   i, error->message);
			g_clear_error(&error);
		} else {
			path = g_strdup_printf("%s%d%s", ATICONFIG_PATH_PREFIX, i, _("Fan"));
			sensor = is_sensor_new(path);
			is_sensor_set_label(sensor, name);
			/* fan sensors are given as a percentage from 0 to 100 */
			is_sensor_set_units(sensor, "%");
			is_sensor_set_low_value(sensor, 0.0);
			is_sensor_set_high_value(sensor, 100.0);
			is_sensor_set_digits(sensor, 0);
			is_sensor_set_icon(sensor, IS_STOCK_FAN);
			g_signal_connect(sensor, "update-value",
					 G_CALLBACK(update_sensor_value),
					 self);
			is_manager_add_sensor(self->priv->manager, sensor);
			g_object_unref(sensor);
			g_free(path);
		}

		g_free(idx);
		g_free(pci);
		g_free(name);

		g_match_info_next(match, &error);
	}

out:
	g_match_info_free(match);
	if (regex) {
		g_regex_unref(regex);
	}
	g_free(output);
	return;
}

static void
is_aticonfig_plugin_deactivate(PeasActivatable *activatable)
{
	IsATIConfigPlugin *plugin = IS_ATICONFIG_PLUGIN(activatable);
	IsATIConfigPluginPrivate *priv = plugin->priv;

	(void)priv;

	/* TODO: remove sensors from manager since we are being unloaded */
}

static void
is_aticonfig_plugin_class_init(IsATIConfigPluginClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsATIConfigPluginPrivate));

	gobject_class->get_property = is_aticonfig_plugin_get_property;
	gobject_class->set_property = is_aticonfig_plugin_set_property;
	gobject_class->finalize = is_aticonfig_plugin_finalize;

	g_object_class_override_property(gobject_class, PROP_OBJECT, "object");
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
{
  iface->activate = is_aticonfig_plugin_activate;
  iface->deactivate = is_aticonfig_plugin_deactivate;
}

static void
is_aticonfig_plugin_class_finalize(IsATIConfigPluginClass *klass)
{
	/* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  is_aticonfig_plugin_register_type(G_TYPE_MODULE(module));

  peas_object_module_register_extension_type(module,
					     PEAS_TYPE_ACTIVATABLE,
					     IS_TYPE_ATICONFIG_PLUGIN);
}
