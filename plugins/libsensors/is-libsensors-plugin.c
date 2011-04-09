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
#include <is-indicator.h>

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
}

static void
is_libsensors_plugin_finalize(GObject *object)
{
	IsLibsensorsPlugin *self = (IsLibsensorsPlugin *)object;

	/* Make compiler happy */
	(void)self;

	G_OBJECT_CLASS(is_libsensors_plugin_parent_class)->finalize(object);
}

static void
is_libsensors_plugin_activate(PeasActivatable *activatable)
{
	IsLibsensorsPlugin *plugin = IS_LIBSENSORS_PLUGIN(activatable);

	g_debug(G_STRFUNC);

	/* search for sensors and add them to indicator */
}

static void
is_libsensors_plugin_deactivate(PeasActivatable *activatable)
{
	IsLibsensorsPlugin *plugin = IS_LIBSENSORS_PLUGIN(activatable);

	g_debug(G_STRFUNC);

	/* remove sensors from indicator */

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
