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

#include "is-libnotify-plugin.h"
#include <indicator-sensors/is-manager.h>
#include <libnotify/notify.h>
#include <libnotify/notification.h>
#include <glib/gi18n.h>
#include <math.h>

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsLibnotifyPlugin,
			       is_libnotify_plugin,
			       PEAS_TYPE_EXTENSION_BASE,
			       0,
			       G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
							     peas_activatable_iface_init));

enum {
	PROP_OBJECT = 1,
};

struct _IsLibnotifyPluginPrivate
{
	IsManager *manager;
	gboolean inited;
	gboolean append;
	GSList *sensors;
};

static void is_libnotify_plugin_finalize(GObject *object);

static void
is_libnotify_plugin_set_property(GObject *object,
					 guint prop_id,
					 const GValue *value,
					 GParamSpec *pspec)
{
	IsLibnotifyPlugin *plugin = IS_LIBNOTIFY_PLUGIN(object);

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
is_libnotify_plugin_get_property(GObject *object,
					 guint prop_id,
					 GValue *value,
					 GParamSpec *pspec)
{
	IsLibnotifyPlugin *plugin = IS_LIBNOTIFY_PLUGIN(object);

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
is_libnotify_plugin_init(IsLibnotifyPlugin *self)
{
	IsLibnotifyPluginPrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_LIBNOTIFY_PLUGIN,
					    IsLibnotifyPluginPrivate);
	self->priv = priv;
        priv->inited = notify_init(PACKAGE);
	if (priv->inited) {
		/* look for append capability */
		GList *caps = notify_get_server_caps();
		while (caps != NULL) {
			gchar *cap = (gchar *)caps->data;
			if (g_strcmp0(cap, "append") == 0) {
				priv->append = TRUE;
			}
			g_free(cap);
			caps = g_list_delete_link(caps, caps);
		}
	}
}

static void
is_libnotify_plugin_finalize(GObject *object)
{
	IsLibnotifyPlugin *self = (IsLibnotifyPlugin *)object;
	IsLibnotifyPluginPrivate *priv = self->priv;

	/* think about storing this in the class structure so we only init once
	   and unload once */
	if (priv->inited) {
		notify_uninit();
		priv->inited = FALSE;
	}
	if (priv->manager) {
		g_object_unref(priv->manager);
		priv->manager = NULL;
	}
	G_OBJECT_CLASS(is_libnotify_plugin_parent_class)->finalize(object);
}


static void
sensor_notify_alarmed(IsSensor *sensor,
		      GParamSpec *pspec,
		      IsLibnotifyPlugin *self)
{
	IsLibnotifyPluginPrivate *priv;
	gboolean alarmed;

	priv = self->priv;

	alarmed = is_sensor_get_alarmed(sensor);

	if (alarmed) {
		NotifyNotification *notification;
		gchar *body = g_strdup_printf("%s: %f%s",
					      is_sensor_get_label(sensor),
					      is_sensor_get_value(sensor),
					      is_sensor_get_units(sensor));
		g_debug("Alarm for %s", is_sensor_get_path(sensor));
		notification = notify_notification_new(_("Sensor Alarm"),
						       body, NULL);
		g_free(body);
		if (priv->append) {
			GVariant *cap = g_variant_new("s", "");
			notify_notification_set_hint(notification, "x-canonical-append", cap);
		}
		notify_notification_show(notification, NULL);
		g_object_unref(notification);
	}
}

static void
sensor_enabled(IsManager *manager,
	       IsSensor *sensor,
	       gint position,
	       IsLibnotifyPlugin *self)
{
	IsLibnotifyPluginPrivate *priv;

	priv = self->priv;

	g_assert(!g_slist_find(priv->sensors, sensor));
	g_signal_connect(sensor, "notify::alarmed",
			 G_CALLBACK(sensor_notify_alarmed), self);
	priv->sensors = g_slist_insert(priv->sensors, sensor, position);
}

static void
sensor_disabled(IsManager *manager,
		IsSensor *sensor,
		IsLibnotifyPlugin *self)
{
	IsLibnotifyPluginPrivate *priv;

	priv = self->priv;

	g_signal_handlers_disconnect_by_func(sensor, sensor_notify_alarmed, self);
	priv->sensors = g_slist_remove(priv->sensors, sensor);
}

static void
is_libnotify_plugin_activate(PeasActivatable *activatable)
{
	IsLibnotifyPlugin *self = IS_LIBNOTIFY_PLUGIN(activatable);
	IsLibnotifyPluginPrivate *priv = self->priv;
	GSList *list;
	gint i = 0;

	/* watch for sensors enabled / disabled to monitor their values */
	if (!priv->inited) {
		g_warning("libnotify is not inited, unable to display notifications");
		goto out;
	}
	g_signal_connect(priv->manager, "sensor-enabled",
			 G_CALLBACK(sensor_enabled), self);
	g_signal_connect(priv->manager, "sensor-disabled",
			 G_CALLBACK(sensor_disabled), self);

	list = is_manager_get_enabled_sensors_list(priv->manager);
	i = 0;
	while (list != NULL) {
		IsSensor *sensor = IS_SENSOR(list->data);
		sensor_enabled(priv->manager, sensor, i++, self);
		g_object_unref(sensor);
		list = g_slist_delete_link(list, list);
	}

out:
	return;
}

static void
is_libnotify_plugin_deactivate(PeasActivatable *activatable)
{
	IsLibnotifyPlugin *self = IS_LIBNOTIFY_PLUGIN(activatable);
	IsLibnotifyPluginPrivate *priv = self->priv;

	g_signal_handlers_disconnect_by_func(priv->manager, sensor_enabled,
					     self);
	g_signal_handlers_disconnect_by_func(priv->manager, sensor_disabled,
					     self);
	while (priv->sensors) {
		IsSensor *sensor = IS_SENSOR(priv->sensors->data);
		g_signal_handlers_disconnect_by_func(sensor,
						     sensor_notify_alarmed,
						     self);
		priv->sensors = g_slist_delete_link(priv->sensors,
						    priv->sensors);
	}
}

static void
is_libnotify_plugin_class_init(IsLibnotifyPluginClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsLibnotifyPluginPrivate));

	gobject_class->get_property = is_libnotify_plugin_get_property;
	gobject_class->set_property = is_libnotify_plugin_set_property;
	gobject_class->finalize = is_libnotify_plugin_finalize;

	g_object_class_override_property(gobject_class, PROP_OBJECT, "object");
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
{
  iface->activate = is_libnotify_plugin_activate;
  iface->deactivate = is_libnotify_plugin_deactivate;
}

static void
is_libnotify_plugin_class_finalize(IsLibnotifyPluginClass *klass)
{
	/* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  is_libnotify_plugin_register_type(G_TYPE_MODULE(module));

  peas_object_module_register_extension_type(module,
					     PEAS_TYPE_ACTIVATABLE,
					     IS_TYPE_LIBNOTIFY_PLUGIN);
}
