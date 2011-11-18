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
#include <indicator-sensors/is-indicator.h>
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
is_gsettings_plugin_activate(PeasActivatable *activatable)
{
	IsGSettingsPlugin *self = IS_GSETTINGS_PLUGIN(activatable);
	IsGSettingsPluginPrivate *priv = self->priv;
	GSettings *settings;
	IsIndicator *indicator;

	/* bind the enabled-sensors property of the manager as well so we save /
	 * restore the list of enabled sensors too */
	settings = g_settings_new("indicator-sensors.manager");
	g_settings_bind(settings, "enabled-sensors",
			priv->manager, "enabled-sensors",
			G_SETTINGS_BIND_DEFAULT);
	g_object_set_data_full(G_OBJECT(priv->manager), "gsettings", settings,
			       (GDestroyNotify)g_object_unref);
	g_settings_bind(settings, "temperature-scale",
			priv->manager, "temperature-scale",
			G_SETTINGS_BIND_DEFAULT);
	/* get default indicator and bind its primary_sensor property */
	indicator = is_indicator_get_default();
	settings = g_settings_new("indicator-sensors.indicator");
	g_settings_bind(settings, "primary-sensor",
			indicator, "primary-sensor-path",
			G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(settings, "display-flags",
			indicator, "display-flags",
			G_SETTINGS_BIND_DEFAULT);
	g_object_set_data_full(G_OBJECT(indicator), "gsettings", settings,
			       (GDestroyNotify)g_object_unref);
	g_object_unref(indicator);
}

static void
is_gsettings_plugin_deactivate(PeasActivatable *activatable)
{
	IsGSettingsPlugin *self = IS_GSETTINGS_PLUGIN(activatable);
	IsGSettingsPluginPrivate *priv = self->priv;

	g_object_set_data(G_OBJECT(priv->manager), "gsettings", NULL);
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
