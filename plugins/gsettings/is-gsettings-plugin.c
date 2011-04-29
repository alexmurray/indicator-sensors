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

#include "is-gsettings-plugin.h"
#include <indicator-sensors/is-manager.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsGSettingsPlugin,
			       is_gsettings_plugin,
			       PEAS_TYPE_EXTENSION_BASE,
			       0,
			       G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
							     peas_activatable_iface_init));

enum {
	PROP_OBJECT = 1,
};

struct _IsGSettingsPluginPrivate
{
	IsManager *manager;
	GSList *sensors;
};

static void is_gsettings_plugin_finalize(GObject *object);

static void
is_gsettings_plugin_set_property(GObject *object,
				 guint prop_id,
				 const GValue *value,
				 GParamSpec *pspec)
{
	IsGSettingsPlugin *plugin = IS_GSETTINGS_PLUGIN(object);

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
is_gsettings_plugin_get_property(GObject *object,
				 guint prop_id,
				 GValue *value,
				 GParamSpec *pspec)
{
	IsGSettingsPlugin *plugin = IS_GSETTINGS_PLUGIN(object);

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
is_gsettings_plugin_init(IsGSettingsPlugin *self)
{
	IsGSettingsPluginPrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_GSETTINGS_PLUGIN,
					    IsGSettingsPluginPrivate);
	self->priv = priv;
}

static void
is_gsettings_plugin_finalize(GObject *object)
{
	IsGSettingsPlugin *self = (IsGSettingsPlugin *)object;
	IsGSettingsPluginPrivate *priv = self->priv;

	if (priv->manager) {
		g_object_unref(priv->manager);
		priv->manager = NULL;
	}
	G_OBJECT_CLASS(is_gsettings_plugin_parent_class)->finalize(object);
}

static void
sensor_added(IsManager *manager,
	     IsSensor *sensor,
	     IsGSettingsPlugin *self)
{
	IsGSettingsPluginPrivate *priv;
	gchar *path;
	GSettings *settings;

	priv = self->priv;

	g_assert(!g_slist_find(priv->sensors, sensor));
	priv->sensors = g_slist_append(priv->sensors, sensor);

	path = g_strdup_printf("/apps/indicator-sensors/sensors/%s/",
			       is_sensor_get_path(sensor));
	settings = g_settings_new_with_path("indicator-sensors.sensor",
					    path);
	g_free(path);

	/* bind to properties so we automatically save and restore most recent
	 * value */
	g_settings_bind(settings, "label", sensor, "label",
			G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(settings, "min", sensor, "min",
			G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(settings, "max", sensor, "max",
			G_SETTINGS_BIND_DEFAULT);
	g_object_set_data_full(G_OBJECT(sensor),
			       "gsettings", settings, g_object_unref);
}

static void
sensor_removed(IsManager *manager,
	       IsSensor *sensor,
	       IsGSettingsPlugin *self)
{
	IsGSettingsPluginPrivate *priv;

	priv = self->priv;

	g_object_set_data(G_OBJECT(sensor), "gsettings", NULL);
	priv->sensors = g_slist_remove(priv->sensors, sensor);
}

static void
is_gsettings_plugin_activate(PeasActivatable *activatable)
{
	IsGSettingsPlugin *self = IS_GSETTINGS_PLUGIN(activatable);
	IsGSettingsPluginPrivate *priv = self->priv;
	GSList *sensors;
	GSettings *settings;

	sensors = is_manager_get_all_sensors_list(priv->manager);
	while (sensors != NULL) {
		IsSensor *sensor = IS_SENSOR(sensors->data);
		sensor_added(priv->manager, sensor, self);
		g_object_unref(sensor);
		sensors = g_slist_delete_link(sensors, sensors);
	}

	/* watch for sensors added / removed to save / restore their
	 * settings  */
	g_signal_connect(priv->manager, "sensor-added",
			 G_CALLBACK(sensor_added), self);
	g_signal_connect(priv->manager, "sensor-removed",
			 G_CALLBACK(sensor_removed), self);

	/* bind the enabled-sensors property of the manager as well so we save /
	 * restore the list of enabled sensors too */
	settings = g_settings_new("indicator-sensors.manager");
	g_settings_bind(settings, "enabled-sensors",
			priv->manager, "enabled-sensors",
			G_SETTINGS_BIND_DEFAULT);
	g_object_set_data_full(G_OBJECT(priv->manager), "gsettings", settings,
			       (GDestroyNotify)g_object_unref);
}

static void
is_gsettings_plugin_deactivate(PeasActivatable *activatable)
{
	IsGSettingsPlugin *self = IS_GSETTINGS_PLUGIN(activatable);
	IsGSettingsPluginPrivate *priv = self->priv;

	g_object_set_data(G_OBJECT(priv->manager), "gsettings", NULL);
	g_signal_handlers_disconnect_by_func(priv->manager, sensor_added, self);
	g_signal_handlers_disconnect_by_func(priv->manager, sensor_removed, self);
	while (priv->sensors) {
		IsSensor *sensor = IS_SENSOR(priv->sensors->data);
		sensor_removed(priv->manager, sensor, self);
	}
}

static void
is_gsettings_plugin_class_init(IsGSettingsPluginClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsGSettingsPluginPrivate));

	gobject_class->get_property = is_gsettings_plugin_get_property;
	gobject_class->set_property = is_gsettings_plugin_set_property;
	gobject_class->finalize = is_gsettings_plugin_finalize;

	g_object_class_override_property(gobject_class, PROP_OBJECT, "object");
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
{
	iface->activate = is_gsettings_plugin_activate;
	iface->deactivate = is_gsettings_plugin_deactivate;
}

static void
is_gsettings_plugin_class_finalize(IsGSettingsPluginClass *klass)
{
	/* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
	is_gsettings_plugin_register_type(G_TYPE_MODULE(module));

	peas_object_module_register_extension_type(module,
						   PEAS_TYPE_ACTIVATABLE,
						   IS_TYPE_GSETTINGS_PLUGIN);
}
