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

#include "is-udisks-plugin.h"
#include <indicator-sensors/is-log.h>
#include <indicator-sensors/is-manager.h>
#include <indicator-sensors/is-temperature-sensor.h>
#include <atasmart.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#define UDISKS_BUS_NAME              "org.freedesktop.UDisks"
#define UDISKS_INTERFACE_NAME        "org.freedesktop.UDisks"
#define UDISKS_OBJECT_PATH           "/org/freedesktop/UDisks"
#define UDISKS_DEVICE_INTERFACE_NAME "org.freedesktop.UDisks.Device"

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsUdisksPlugin,
			       is_udisks_plugin,
			       PEAS_TYPE_EXTENSION_BASE,
			       0,
			       G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
							     peas_activatable_iface_init));

enum {
	PROP_OBJECT = 1,
};

struct _IsUdisksPluginPrivate
{
	IsManager *manager;
	GHashTable *sensors;
	GDBusConnection *connection;
};

static void is_udisks_plugin_finalize(GObject *object);

static void
is_udisks_plugin_set_property(GObject *object,
				 guint prop_id,
				 const GValue *value,
				 GParamSpec *pspec)
{
	IsUdisksPlugin *plugin = IS_UDISKS_PLUGIN(object);

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
is_udisks_plugin_get_property(GObject *object,
				 guint prop_id,
				 GValue *value,
				 GParamSpec *pspec)
{
	IsUdisksPlugin *plugin = IS_UDISKS_PLUGIN(object);

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
is_udisks_plugin_init(IsUdisksPlugin *self)
{
	IsUdisksPluginPrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_UDISKS_PLUGIN,
					    IsUdisksPluginPrivate);
	self->priv = priv;
}

static void
is_udisks_plugin_finalize(GObject *object)
{
	IsUdisksPlugin *self = (IsUdisksPlugin *)object;
	IsUdisksPluginPrivate *priv = self->priv;

	if (priv->manager) {
		g_object_unref(priv->manager);
		priv->manager = NULL;
	}
	G_OBJECT_CLASS(is_udisks_plugin_parent_class)->finalize(object);
}

static void
update_sensor_value(IsTemperatureSensor *sensor,
		    IsUdisksPlugin *self)

{
	IsUdisksPluginPrivate *priv;
	gchar *device, *path;
	GDBusProxy *proxy;
	GVariant *var;
	GError *error = NULL;
	SkDisk *sk_disk;
	const gchar *blob;
	gsize len;
	guint64 temperature;
	gdouble value;

	priv = self->priv;

	device = g_path_get_basename(is_sensor_get_path(IS_SENSOR(sensor)));
	path = g_strdup_printf("%s/devices/%s", UDISKS_OBJECT_PATH, device);
	proxy = g_dbus_proxy_new_sync(priv->connection,
				      G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
				      NULL,
				      UDISKS_BUS_NAME,
				      path,
				      UDISKS_DEVICE_INTERFACE_NAME,
				      NULL, &error);
	g_free(path);
	g_free(device);

	if (!proxy) {
		g_prefix_error(&error,
			       _("Error reading new SMART data for sensor %s"),
			       is_sensor_get_path(IS_SENSOR(sensor)));
		is_sensor_emit_error(IS_SENSOR(sensor), error);
		g_error_free(error);
		goto out;
	}

	/* update smart data */
	var = g_variant_new_strv(NULL, 0);
	var = g_dbus_proxy_call_sync(proxy, "DriveAtaSmartRefreshData",
				     g_variant_new_tuple(&var,
							 1),
				     G_DBUS_CALL_FLAGS_NONE,
				     -1, NULL, &error);
	if (!var) {
		g_prefix_error(&error,
			       _("Error refreshing SMART data for sensor %s"),
			       is_sensor_get_path(IS_SENSOR(sensor)));
		is_sensor_emit_error(IS_SENSOR(sensor), error);
		g_error_free(error);
		g_object_unref(proxy);
		goto out;
	}
	g_variant_unref(var);

	var = g_dbus_proxy_get_cached_property(proxy,
					       "DriveAtaSmartBlob");
	if (!var) {
		is_debug("udisks", "unable to get atasmartblob for sensor %s",
			is_sensor_get_path(IS_SENSOR(sensor)));
		g_object_unref(proxy);
		goto out;
	}
	g_object_unref(proxy);

	/* can't unref var until done with blob */
	blob = g_variant_get_fixed_array(var, &len, sizeof(gchar));
	if (!blob) {
		/* this can occur if udisks doesn't update immediately,
		 * ignore */
		g_variant_unref(var);
		goto out;
	}
	sk_disk_open(NULL, &sk_disk);
	sk_disk_set_blob(sk_disk, blob, len);
	if (sk_disk_smart_get_temperature(sk_disk, &temperature) < 0)
	{
		is_debug("udisks", "Error getting temperature from AtaSmartBlob for sensor %s",
			is_sensor_get_path(IS_SENSOR(sensor)));
		sk_disk_free(sk_disk);
		g_variant_unref(var);
		/* TODO: emit error */
		goto out;
	}

	sk_disk_free(sk_disk);
	g_variant_unref(var);

	/* Temperature is in mK, so convert it to K first */
	temperature /= 1000;
	value = (gdouble)temperature - 273.15;
	is_temperature_sensor_set_celsius_value(sensor, value);

out:
	return;
}

static void
is_udisks_plugin_activate(PeasActivatable *activatable)
{
	IsUdisksPlugin *self = IS_UDISKS_PLUGIN(activatable);
	IsUdisksPluginPrivate *priv = self->priv;
	GDBusProxy *proxy;
	GError *error = NULL;
	GVariant *container, *paths;
	GVariantIter iter;
	gchar *path;

	priv->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!priv->connection)
	{
		is_warning("udisks", "Failed to open connection to system dbus: %s",
			  error->message);
		g_error_free(error);
		goto out;
	}

	/* This is the proxy which is only used once during the enumeration of
	 * the device object paths
	 */
	proxy = g_dbus_proxy_new_sync(priv->connection,
				      G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
				      G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,

				      NULL,
				      UDISKS_BUS_NAME,
				      UDISKS_OBJECT_PATH,
				      UDISKS_INTERFACE_NAME,
				      NULL, &error);

	if (!proxy) {
		is_warning("udisks", "Error getting proxy to udisks on system bus: %s",
			  error->message);
		g_error_free(error);
		goto out;
	}

	/* The object paths of the disks are enumerated and placed in an array
	 * of object paths
	 */
	container = g_dbus_proxy_call_sync(proxy, "EnumerateDevices", NULL,
				       G_DBUS_CALL_FLAGS_NONE, -1, NULL,
				       &error);
	if (!container) {
		is_warning("udisks", "Failed to enumerate disk devices: %s",
			  error->message);
		g_error_free(error);
		g_object_unref(proxy);
		goto out;
	}

	paths = g_variant_get_child_value(container, 0);
	g_variant_unref(container);

	g_variant_iter_init(&iter, paths);
	while (g_variant_iter_loop(&iter, "o", &path)) {
		/* This proxy is used to get the required data in order to build
		 * up the list of sensors
		 */
		GDBusProxy *sensor_proxy;
		GVariant *model, *smart_available;
		IsSensor *sensor;
		gchar *name, *sensor_path;

		sensor_proxy = g_dbus_proxy_new_sync(priv->connection,
						     G_DBUS_PROXY_FLAGS_NONE,
						     NULL,
						     UDISKS_BUS_NAME,
						     path,
						     UDISKS_DEVICE_INTERFACE_NAME,
						     NULL,
						     &error);

		if (!sensor_proxy) {
			is_debug("udisks", "error getting sensor proxy for disk %s: %s",
				path, error->message);
			g_clear_error(&error);
			g_object_unref(sensor_proxy);
			continue;
		}

		smart_available = g_dbus_proxy_get_cached_property(sensor_proxy,
								   "DriveAtaSmartIsAvailable");
		if (!smart_available) {
			is_debug("udisks", "error getting smart status for disk %s",
				path);
			g_object_unref(sensor_proxy);
			continue;
		}
		if (!g_variant_get_boolean(smart_available)) {
			is_debug("udisks", "drive %s does not support SMART monitoring, ignoring...",
				path);
			g_variant_unref(smart_available);
			g_object_unref(sensor_proxy);
			continue;
		}

		g_variant_unref(smart_available);
		model = g_dbus_proxy_get_cached_property(sensor_proxy,
							 "DriveModel");
		if (!model) {
			is_debug("udisks", "error getting drive model for disk %s",
				path);
			g_clear_error(&error);
			g_object_unref(sensor_proxy);
			continue;
		}
		name = g_path_get_basename(path);
		sensor_path = g_strdup_printf("udisks/%s", name);
		sensor = is_temperature_sensor_new(sensor_path);
		is_sensor_set_label(sensor, g_variant_get_string(model, NULL));
		/* only update every minute to avoid waking disk too much */
		is_sensor_set_update_interval(sensor, 60);
		g_signal_connect(sensor, "update-value",
				 G_CALLBACK(update_sensor_value), self);
		is_manager_add_sensor(priv->manager, sensor);

		g_free(sensor_path);
		g_free(name);
		g_object_unref(sensor);
		g_object_unref(sensor_proxy);
	}
	g_variant_unref(paths);
	g_object_unref(proxy);

out:
	return;
}

static void
is_udisks_plugin_deactivate(PeasActivatable *activatable)
{
	IsUdisksPlugin *self = IS_UDISKS_PLUGIN(activatable);
	IsUdisksPluginPrivate *priv = self->priv;

	if (priv->connection) {
		g_object_unref(priv->connection);
	}
}

static void
is_udisks_plugin_class_init(IsUdisksPluginClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsUdisksPluginPrivate));

	gobject_class->get_property = is_udisks_plugin_get_property;
	gobject_class->set_property = is_udisks_plugin_set_property;
	gobject_class->finalize = is_udisks_plugin_finalize;

	g_object_class_override_property(gobject_class, PROP_OBJECT, "object");
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
{
	iface->activate = is_udisks_plugin_activate;
	iface->deactivate = is_udisks_plugin_deactivate;
}

static void
is_udisks_plugin_class_finalize(IsUdisksPluginClass *klass)
{
	/* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
	is_udisks_plugin_register_type(G_TYPE_MODULE(module));

	peas_object_module_register_extension_type(module,
						   PEAS_TYPE_ACTIVATABLE,
						   IS_TYPE_UDISKS_PLUGIN);
}
