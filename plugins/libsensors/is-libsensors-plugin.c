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

#include "is-libsensors-plugin.h"
#include <stdlib.h>
#include <indicator-sensors/is-temperature-sensor.h>
#include <indicator-sensors/is-fan-sensor.h>
#include <indicator-sensors/is-manager.h>
#include <indicator-sensors/is-log.h>
#include <indicator-sensors/is-notify.h>
#include <sensors/sensors.h>
#include <sensors/error.h>
#include <glib/gi18n.h>

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsLibsensorsPlugin,
			       is_libsensors_plugin,
			       PEAS_TYPE_EXTENSION_BASE,
			       0,
			       G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
							     peas_activatable_iface_init));

enum {
	PROP_OBJECT = 1,
};

struct _IsLibsensorsPluginPrivate
{
	IsManager *manager;
	gboolean inited;
	GHashTable *sensor_chip_names;
};

static void is_libsensors_plugin_finalize(GObject *object);

static void
is_libsensors_plugin_set_property(GObject *object,
					 guint prop_id,
					 const GValue *value,
					 GParamSpec *pspec)
{
	IsLibsensorsPlugin *plugin = IS_LIBSENSORS_PLUGIN(object);

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
is_libsensors_plugin_get_property(GObject *object,
					 guint prop_id,
					 GValue *value,
					 GParamSpec *pspec)
{
	IsLibsensorsPlugin *plugin = IS_LIBSENSORS_PLUGIN(object);

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
is_libsensors_plugin_init(IsLibsensorsPlugin *self)
{
	IsLibsensorsPluginPrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_LIBSENSORS_PLUGIN,
					    IsLibsensorsPluginPrivate);

	self->priv = priv;
        if (sensors_init(NULL) == 0) {
		priv->sensor_chip_names = g_hash_table_new_full(g_str_hash,
								g_str_equal,
								g_free,
								NULL);
		priv->inited = TRUE;
	}
}

static void
is_libsensors_plugin_finalize(GObject *object)
{
	IsLibsensorsPlugin *self = (IsLibsensorsPlugin *)object;
	IsLibsensorsPluginPrivate *priv = self->priv;

	if (priv->sensor_chip_names) {
		g_hash_table_destroy(priv->sensor_chip_names);
	}
	/* think about storing this in the class structure so we only init once
	   and unload once */
	if (priv->inited) {
		sensors_cleanup();
		priv->inited = FALSE;
	}
	G_OBJECT_CLASS(is_libsensors_plugin_parent_class)->finalize(object);
}

static gchar *get_chip_name_string(const sensors_chip_name *chip) {
	gchar *name = NULL;

	// adapted from lm-sensors:prog/sensors/main.c:sprintf_chip_name
        // in lm-sensors-3.0
#define BUF_SIZE 200
	name = g_malloc0(BUF_SIZE);
	if (sensors_snprintf_chip_name(name, BUF_SIZE, chip) < 0) {
		g_free(name);
                name = NULL;
        }
#undef BUF_SIZE
	return name;
}

static void
update_sensor_value(IsSensor *sensor,
		    IsLibsensorsPlugin *self)
{
	const gchar *path;
	const sensors_chip_name *found_chip;
	gchar *offset, *end;
	int n, ret;
	gdouble value = is_sensor_get_value(sensor);

	path = is_sensor_get_path(sensor);

	found_chip = g_hash_table_lookup(self->priv->sensor_chip_names,
					 path);
	g_assert(found_chip != NULL);

	/* get last separator */
	offset = g_strrstr(path, "/");
	g_assert(offset != NULL);

	n = (int)g_ascii_strtoll(offset + 1, &end, 10);
	/* conversion should also never fail */
	g_assert(end != NULL);

	/* ignore IO error */
	if ((ret = sensors_get_value(found_chip, n, &value)) < 0 &&
	    ret != -SENSORS_ERR_IO) {
		GError *error = g_error_new(g_quark_from_string("libsensors-plugin-error-quark"),
					    0,
					    /* first placeholder is sensor name,
					     * second is error message */
					    _("Error getting sensor value for sensor %s: %s [%d]"),
					    path, sensors_strerror(ret), ret);
		is_sensor_emit_error(sensor, error);
		g_error_free(error);
		goto out;
	}
	if (IS_IS_TEMPERATURE_SENSOR(sensor)) {
		is_temperature_sensor_set_celsius_value(IS_TEMPERATURE_SENSOR(sensor),
							value);
	} else {
		is_sensor_set_value(sensor, value);
	}

out:
	return;
}

static void
process_sensors_chip_name(IsLibsensorsPlugin *self,
			  const sensors_chip_name *chip_name)
{
	IsLibsensorsPluginPrivate *priv = self->priv;
	gchar *chip_name_string = NULL;
	const sensors_feature *main_feature;
	gint nr1 = 0;

	chip_name_string = get_chip_name_string(chip_name);
	if (chip_name_string == NULL) {
		is_warning("libsensors", "error getting name string for sensor '%s'",
			  chip_name->path);
		goto out;
	}
	while ((main_feature = sensors_get_features(chip_name, &nr1))) {
		gchar *label = NULL;
		const sensors_subfeature *input_feature = NULL;
		const sensors_subfeature *min_feature = NULL;
		const sensors_subfeature *max_feature = NULL;
		gchar *path;
		IsSensor *sensor;
		gdouble value, min, max;
		int ret;

		switch (main_feature->type) {
		case SENSORS_FEATURE_IN:
			input_feature = sensors_get_subfeature(chip_name,
							       main_feature,
							       SENSORS_SUBFEATURE_IN_INPUT);
			min_feature = sensors_get_subfeature(chip_name,
							     main_feature,
							     SENSORS_SUBFEATURE_IN_MIN);
			break;
		case SENSORS_FEATURE_FAN:
			input_feature = sensors_get_subfeature(chip_name,
							       main_feature,
							       SENSORS_SUBFEATURE_FAN_INPUT);
			min_feature = sensors_get_subfeature(chip_name,
							     main_feature,
							     SENSORS_SUBFEATURE_FAN_ALARM);
			break;
		case SENSORS_FEATURE_TEMP:
			input_feature = sensors_get_subfeature(chip_name,
							       main_feature,
							       SENSORS_SUBFEATURE_TEMP_INPUT);
			max_feature = sensors_get_subfeature(chip_name,
							      main_feature,
							      SENSORS_SUBFEATURE_TEMP_MAX);
			min_feature = sensors_get_subfeature(chip_name,
							      main_feature,
							      SENSORS_SUBFEATURE_TEMP_MIN);
			break;

#if SENSORS_API_VERSION > 0x430
		case SENSORS_FEATURE_HUMIDITY:
		case SENSORS_FEATURE_MAX_MAIN:
		case SENSORS_FEATURE_INTRUSION:
		case SENSORS_FEATURE_MAX_OTHER:
#endif
		case SENSORS_FEATURE_POWER:
		case SENSORS_FEATURE_ENERGY:
		case SENSORS_FEATURE_CURR:
		case SENSORS_FEATURE_VID:
		case SENSORS_FEATURE_BEEP_ENABLE:
		case SENSORS_FEATURE_UNKNOWN:
			is_debug("libsensors", "Ignoring unimplemented sensor type %d",
				main_feature->type);
			break;

		default:
			is_warning("libsensors", "error determining type for sensor '%s'",
				  chip_name_string);
			continue;
		}

		if (!input_feature) {
			is_warning("libsensors", "could not get input subfeature for sensor '%s'",
				  chip_name_string);
			continue;
		}
		// if still here we got input feature so get label
		label = sensors_get_label(chip_name, main_feature);
		if (!label) {
			is_warning("libsensors", "could not get label for sensor '%s'",
				  chip_name_string);
			continue;
		}
		// label can sometimes have leading / trailing whitespace so
		// remove any
		g_strstrip(label);

		g_assert(chip_name_string && label);

		/* ignore IO Errors since some chips report these sometimes
		   intermittently */
		if ((ret = sensors_get_value(chip_name, input_feature->number, &value)) < 0 &&
		    ret != -SENSORS_ERR_IO) {
			is_warning("libsensors", "could not get value for input feature of sensor '%s': %s [%d]",
				  chip_name_string, sensors_strerror(ret), ret);
			free(label);
			continue;
		}

		path = g_strdup_printf("libsensors/%s/%d",
				       chip_name_string,
				       input_feature->number);
		if (main_feature->type == SENSORS_FEATURE_TEMP)
		{
			sensor = is_temperature_sensor_new(path);
			is_sensor_set_icon(sensor, IS_STOCK_CPU);
		} else if (main_feature->type == SENSORS_FEATURE_FAN) {
			sensor = is_fan_sensor_new(path);
			/* display fan readings to 0 decimal places like
			   sensors command */
			is_sensor_set_digits(sensor, 0);
		} else {
			/* is a voltage sensor */
			sensor = is_sensor_new(path);
			/* display voltage readings to 2 decimal places like
			   sensors command */
			is_sensor_set_digits(sensor, 2);
			/* translators: V is the unit for Voltage, replace with
			   appropriate unit */
			is_sensor_set_units(sensor, _("V"));
			is_sensor_set_icon(sensor, IS_STOCK_CHIP);
		}
		is_sensor_set_label(sensor, label);
		if (min_feature &&
		    sensors_get_value(chip_name, min_feature->number, &min) == 0) {
			is_sensor_set_alarm_mode(sensor, IS_SENSOR_ALARM_MODE_LOW);
			is_sensor_set_alarm_value(sensor, min);
			if (main_feature->type == SENSORS_FEATURE_TEMP)
			{
				is_sensor_set_low_value(sensor, min);
			}
		}
		if (max_feature &&
		    sensors_get_value(chip_name, max_feature->number, &max) == 0) {
			is_sensor_set_alarm_mode(sensor, IS_SENSOR_ALARM_MODE_HIGH);
			is_sensor_set_alarm_value(sensor, max);
			if (main_feature->type == SENSORS_FEATURE_TEMP)
			{
				is_sensor_set_high_value(sensor, max);
			}
		}

		/* take ownership of path pointer */
		g_hash_table_insert(priv->sensor_chip_names, path, (void *)chip_name);
		/* connect to update-value signal */
		g_signal_connect(sensor, "update-value",
				 G_CALLBACK(update_sensor_value),
				 self);
		is_manager_add_sensor(priv->manager, sensor);
		free(label);
	}
	g_free(chip_name_string);
out:
	return;
}

static void
is_libsensors_plugin_activate(PeasActivatable *activatable)
{
	IsLibsensorsPlugin *self = IS_LIBSENSORS_PLUGIN(activatable);
	IsLibsensorsPluginPrivate *priv = self->priv;
	const sensors_chip_name *chip_name;
        int nr = 0;

	/* search for sensors and add them to manager */
	if (!priv->inited) {
		is_warning("libsensors", "not inited, unable to find sensors");
		goto out;
	}
	is_debug("libsensors", "searching for sensors");
	while ((chip_name = sensors_get_detected_chips(NULL, &nr)))
        {
		process_sensors_chip_name(self, chip_name);
	}
        /* if we couldn't find any sensors then show a notification to tell the
         * user to try and run sensors-detect from the command line */
        if (!g_hash_table_size(priv->sensor_chip_names)) {
                is_notify(IS_NOTIFY_LEVEL_INFO,
                          _("No Sensors Detected"),
                          _("Try running the command 'sensors-detect' from the command-line and restarting %s"), PACKAGE_NAME);
        }
out:
	return;
}

static void
is_libsensors_plugin_deactivate(PeasActivatable *activatable)
{
	IsLibsensorsPlugin *plugin = IS_LIBSENSORS_PLUGIN(activatable);
	IsLibsensorsPluginPrivate *priv = plugin->priv;

	(void)priv;

	/* TODO: remove sensors from manager since we are being unloaded */
}

static void
is_libsensors_plugin_class_init(IsLibsensorsPluginClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsLibsensorsPluginPrivate));

	gobject_class->get_property = is_libsensors_plugin_get_property;
	gobject_class->set_property = is_libsensors_plugin_set_property;
	gobject_class->finalize = is_libsensors_plugin_finalize;

	g_object_class_override_property(gobject_class, PROP_OBJECT, "object");
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
{
  iface->activate = is_libsensors_plugin_activate;
  iface->deactivate = is_libsensors_plugin_deactivate;
}

static void
is_libsensors_plugin_class_finalize(IsLibsensorsPluginClass *klass)
{
	/* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  is_libsensors_plugin_register_type(G_TYPE_MODULE(module));

  peas_object_module_register_extension_type(module,
					     PEAS_TYPE_ACTIVATABLE,
					     IS_TYPE_LIBSENSORS_PLUGIN);
}
