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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "is-libsensors-plugin.h"
#include <is-indicator.h>
#include <sensors/sensors.h>

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
	IsIndicator *indicator;
	gboolean inited;
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
		plugin->priv->indicator = IS_INDICATOR(g_value_dup_object(value));
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
		g_value_set_object(value, plugin->priv->indicator);
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
		priv->inited = TRUE;
	}
}

static void
is_libsensors_plugin_finalize(GObject *object)
{
	IsLibsensorsPlugin *self = (IsLibsensorsPlugin *)object;

	/* Make compiler happy */
	(void)self;

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
is_libsensors_plugin_activate(PeasActivatable *activatable)
{
	IsLibsensorsPlugin *plugin = IS_LIBSENSORS_PLUGIN(activatable);
	IsLibsensorsPluginPrivate *priv = plugin->priv;
	const sensors_chip_name *chip_name;
	int i = 0;
        int nr = 0;

	/* search for sensors and add them to indicator */
	if (!priv->inited) {
		g_warning("libsensors is not inited, unable to find sensors");
		goto out;
	}
	g_debug("searching for sensors");
	while ((chip_name = sensors_get_detected_chips(NULL, &nr)))
        {
                gchar *chip_name_string = NULL;
		gchar *label = NULL;
                const sensors_subfeature *input_feature;
                const sensors_subfeature *low_feature;
                const sensors_subfeature *high_feature;
		const sensors_feature *main_feature;
		gint nr1 = 0;
                gdouble value, low, high;
                gchar *id;
		IsSensor *sensor;

		chip_name_string = get_chip_name_string(chip_name);
                if (chip_name_string == NULL) {
                        g_debug("%s: %d: error getting name string for sensor: %s\n",
                                        __FILE__, __LINE__, chip_name->path);
                        continue;
                }
		while ((main_feature = sensors_get_features(chip_name, &nr1)))
                {
			switch (main_feature->type)
                        {
                        case SENSORS_FEATURE_IN:
				input_feature = sensors_get_subfeature(chip_name,
								       main_feature,
								       SENSORS_SUBFEATURE_IN_INPUT);
				low_feature = sensors_get_subfeature(chip_name,
                                                                     main_feature,
                                                                     SENSORS_SUBFEATURE_IN_MIN);
				high_feature = sensors_get_subfeature(chip_name,
                                                                      main_feature,
                                                                      SENSORS_SUBFEATURE_IN_MAX);
			  	break;
                        case SENSORS_FEATURE_FAN:
				input_feature = sensors_get_subfeature(chip_name,
								       main_feature,
								       SENSORS_SUBFEATURE_FAN_INPUT);
				low_feature = sensors_get_subfeature(chip_name,
                                                                     main_feature,
                                                                     SENSORS_SUBFEATURE_FAN_MIN);
                                // no fan max feature
				high_feature = NULL;
				break;
                        case SENSORS_FEATURE_TEMP:
				input_feature = sensors_get_subfeature(chip_name,
								       main_feature,
								       SENSORS_SUBFEATURE_TEMP_INPUT);
				low_feature = sensors_get_subfeature(chip_name,
                                                                     main_feature,
                                                                     SENSORS_SUBFEATURE_TEMP_MIN);
				high_feature = sensors_get_subfeature(chip_name,
                                                                      main_feature,
                                                                      SENSORS_SUBFEATURE_TEMP_MAX);
				if (!high_feature)
					high_feature = sensors_get_subfeature(chip_name,
									      main_feature,
									      SENSORS_SUBFEATURE_TEMP_CRIT);
				break;
                        default:
                                g_debug("libsensors plugin: error determining type for: %s\n",
                                        chip_name_string);
				continue;
                        }

			if (!input_feature)
                        {
                                g_debug("%s: %d: could not get input subfeature for: %s\n",
                                        __FILE__, __LINE__, chip_name_string);
				continue;
                        }
                        // if still here we got input feature so get label
                        label = sensors_get_label(chip_name, main_feature);
                        if (!label)
                        {
                                g_debug("%s: %d: error: could not get label for: %s\n",
                                        __FILE__, __LINE__, chip_name_string);
                                continue;
                        }

                        g_assert(chip_name_string && label);

                        if (low_feature) {
                                sensors_get_value(chip_name, low_feature->number, &low);
                        }

                        if (high_feature) {
                                sensors_get_value(chip_name, high_feature->number, &high);
                        }
                        if (sensors_get_value(chip_name, input_feature->number, &value) < 0) {
                                g_debug("%s: %d: error: could not get value for input feature of sensor: %s\n",
                                        __FILE__, __LINE__, chip_name_string);
                                free(label);
                                continue;
                        }

                        id = g_strdup_printf("%s/%d", chip_name_string,
					     input_feature->number);
			// TODO: create our own subclass of IsSensor
			sensor = is_sensor_new("libsensors", id, label,
					       low, high);
			is_indicator_add_sensor(priv->indicator, sensor);
			g_free(id);
			free(label);
		}
		g_free(chip_name_string);
	}
out:
	return;
}

static void
is_libsensors_plugin_deactivate(PeasActivatable *activatable)
{
	IsLibsensorsPlugin *plugin = IS_LIBSENSORS_PLUGIN(activatable);
	IsLibsensorsPluginPrivate *priv = plugin->priv;

	/* remove sensors from indicator */
	is_indicator_remove_all_sensors(priv->indicator, "libsensors");
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
