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

#include "is-udisks2-plugin.h"
#include <indicator-sensors/is-log.h>
#include <indicator-sensors/is-application.h>
#include <indicator-sensors/is-temperature-sensor.h>
#include <udisks/udisks.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#define UDISKS2_PATH_PREFIX "udisks2"

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsUDisks2Plugin,
                               is_udisks2_plugin,
                               PEAS_TYPE_EXTENSION_BASE,
                               0,
                               G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
                                   peas_activatable_iface_init));

enum
{
  PROP_OBJECT = 1,
};

struct _IsUDisks2PluginPrivate
{
  IsApplication *application;
  UDisksClient *client;
  GHashTable *sensors;
};

static void is_udisks2_plugin_finalize(GObject *object);

static void
is_udisks2_plugin_set_property(GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
  IsUDisks2Plugin *plugin = IS_UDISKS2_PLUGIN(object);

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
is_udisks2_plugin_get_property(GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
  IsUDisks2Plugin *plugin = IS_UDISKS2_PLUGIN(object);

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
is_udisks2_plugin_init(IsUDisks2Plugin *self)
{
  IsUDisks2PluginPrivate *priv =
    G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_UDISKS2_PLUGIN,
                                IsUDisks2PluginPrivate);
  self->priv = priv;
}

static void
is_udisks2_plugin_finalize(GObject *object)
{
  IsUDisks2Plugin *self = (IsUDisks2Plugin *)object;
  IsUDisks2PluginPrivate *priv = self->priv;

  if (priv->application)
  {
    g_object_unref(priv->application);
    priv->application = NULL;
  }
  G_OBJECT_CLASS(is_udisks2_plugin_parent_class)->finalize(object);
}

static void
smart_update_ready_cb(GObject *source,
                      GAsyncResult *res,
                      gpointer data)
{
  UDisksDriveAta *drive = UDISKS_DRIVE_ATA(source);
  IsTemperatureSensor *sensor = IS_TEMPERATURE_SENSOR(data);
  GError *error = NULL;
  gboolean ret;
  gdouble temp_k;

  ret = udisks_drive_ata_call_smart_update_finish(drive, res, &error);
  if (!ret)
  {
    g_prefix_error(&error,
                   _("Error reading new SMART data for sensor %s"),
                   is_sensor_get_path(IS_SENSOR(sensor)));
    is_sensor_set_error(IS_SENSOR(sensor), error->message);
    g_error_free(error);
    goto out;
  }

  /* temperature is in kelvin */
  temp_k = udisks_drive_ata_get_smart_temperature(drive);
  /* convert to celcius if is non-zero otherwise report as simply 0C */
  is_temperature_sensor_set_celsius_value(sensor, temp_k ? temp_k - 273.15 : temp_k);
  is_sensor_set_error(IS_SENSOR(sensor), NULL);

out:
  return;
}

static void
pm_get_state_ready_cb(GObject *source,
                      GAsyncResult *res,
                      gpointer data)
{
  UDisksDriveAta *drive = UDISKS_DRIVE_ATA(source);
  IsTemperatureSensor *sensor = IS_TEMPERATURE_SENSOR(data);
  GError *error = NULL;
  gboolean ret;
  guchar state;

  ret = udisks_drive_ata_call_pm_get_state_finish(drive, &state, res, &error);
  if (!ret)
  {
    g_prefix_error(&error,
                   _("Error reading power management state for sensor %s"),
                   is_sensor_get_path(IS_SENSOR(sensor)));
    is_sensor_set_error(IS_SENSOR(sensor), error->message);
    g_error_free(error);
    goto out;
  }

  is_debug("udisks2", "pm_get_state result: 0x%x", state);

  // drive is spun down if state is 0 - otherwise assume is spun up and
  // go ahead and query for new SMART properties
  if (!state)
  {
    // standby - disk is idle so don't bother querying
    g_set_error(&error,
                g_quark_from_string("udisks2-plugin-error-quark"),
                0,
                /* first placeholder is sensor name */
                _("Error getting sensor value for disk sensor %s: disk is idle"),
                is_sensor_get_label(IS_SENSOR(sensor)));
    is_sensor_set_error(IS_SENSOR(sensor), error->message);
    g_error_free(error);
    goto out;
  }

  is_debug("udisks2", "pm_get_state doing smart update");
  udisks_drive_ata_call_smart_update(drive,
                                     /* try not to wakeup disk if is spun
                                        down */
                                     g_variant_new_parsed("{'nowakeup': %v}",
                                         g_variant_new_boolean(TRUE)),
                                     NULL,
                                     smart_update_ready_cb,
                                     sensor);
  is_sensor_set_error(IS_SENSOR(sensor), NULL);

out:
  return;
}

static void
update_sensor_value(IsTemperatureSensor *sensor,
                    IsUDisks2Plugin *self)
{
  GDBusObjectManager *manager;
  gchar *path = NULL;
  GDBusObject *object;
  UDisksDriveAta *drive = NULL;

  manager = udisks_client_get_object_manager(self->priv->client);
  path = g_strdup_printf("/org/freedesktop/UDisks2/drives/%s",
                         /* get id to find on bus as character past / */
                         g_strrstr(is_sensor_get_path(IS_SENSOR(sensor)),
                                   "/") + 1);
  object = g_dbus_object_manager_get_object(manager, path);
  if (!object)
  {
    is_error("udisks2", "Error looking up drive with path %s", path);
    goto out;
  }

  g_object_get(object, "drive-ata", &drive, NULL);
  udisks_drive_ata_call_pm_get_state(drive,
                                     g_variant_new("a{sv}", NULL),
                                     NULL,
                                     pm_get_state_ready_cb,
                                     sensor);
out:
  g_clear_object(&drive);
  g_clear_object(&object);
  g_free(path);
  return;
}

static void
object_removed_cb(GDBusObjectManager *obj_manager,
                  GDBusObject *object,
                  gpointer data)
{
  IsUDisks2Plugin *self;
  IsManager *manager;
  const gchar *id;
  gchar *path = NULL;

  self = IS_UDISKS2_PLUGIN(data);

  id = g_dbus_object_get_object_path(object);
  /* ignore if is not a drive */
  if (!g_str_has_prefix(id, "/org/freedesktop/UDisks2/drives/"))
  {
    goto out;
  }

  id = g_strrstr(id, "/") + 1;
  path = g_strdup_printf(UDISKS2_PATH_PREFIX "/%s", id);
  manager = is_application_get_manager(self->priv->application);
  is_debug("udisks2", "Removing sensor %s as drive removed", id);
  is_manager_remove_path(manager, path);

out:
  g_free(path);
}

static void
object_added_cb(GDBusObjectManager *manager,
                GDBusObject *object,
                gpointer data)
{
  IsUDisks2Plugin *self;
  UDisksDrive *drive = NULL;
  UDisksDriveAta *ata_drive = NULL;
  const gchar *id;
  gchar *path = NULL;
  IsSensor *sensor;

  self = IS_UDISKS2_PLUGIN(data);

  id = g_dbus_object_get_object_path(object);
  /* ignore if is not a drive */
  if (!g_str_has_prefix(id, "/org/freedesktop/UDisks2/drives/"))
  {
    goto out;
  }
  g_object_get(object,
               "drive", &drive,
               "drive-ata", &ata_drive,
               NULL);
  if (!drive || !ata_drive ||
      !udisks_drive_ata_get_smart_enabled(ata_drive))
  {
    goto out;
  }

  id = g_strrstr(id, "/") + 1;
  path = g_strdup_printf("udisks2/%s", id);
  sensor = is_temperature_sensor_new(path);
  is_sensor_set_label(sensor, udisks_drive_get_model(drive));
  is_sensor_set_digits(sensor, 0);
  is_sensor_set_icon(sensor, IS_STOCK_DISK);
  /* only update every minute to avoid waking disk too much */
  is_sensor_set_update_interval(sensor, 60);
  g_signal_connect(sensor, "update-value",
                   G_CALLBACK(update_sensor_value), self);
  is_debug("udisks2", "Adding sensor %s as drive added", id);
  is_manager_add_sensor(is_application_get_manager(self->priv->application),
                        sensor);

out:
  g_free(path);
  g_clear_object(&drive);
  g_clear_object(&ata_drive);
}

static void
is_udisks2_plugin_client_ready_cb(GObject *source,
                                  GAsyncResult *res,
                                  gpointer data)
{
  IsUDisks2Plugin *self = IS_UDISKS2_PLUGIN(data);
  IsUDisks2PluginPrivate *priv = self->priv;
  GError *error = NULL;
  GDBusObjectManager *manager;
  GList *objects, *_list;

  priv->client = udisks_client_new_finish(res, &error);
  if (!priv->client)
  {
    is_debug("udisks2", "Error getting udisks2 client: %s", error->message);
    g_clear_error(&error);
    goto out;
  }

  manager = udisks_client_get_object_manager(priv->client);
  if (!manager)
  {
    is_debug("udisks2", "Unable to get manager from udisks client");
    g_clear_object(&priv->client);
    goto out;
  }

  objects = g_dbus_object_manager_get_objects(manager);
  for (_list = objects; _list != NULL; _list = _list->next)
  {
    object_added_cb(manager, G_DBUS_OBJECT(_list->data), self);
    g_object_unref(G_OBJECT(_list->data));
  }
  g_list_free(objects);

  g_signal_connect(manager, "object-added",
                   G_CALLBACK(object_added_cb), self);
  g_signal_connect(manager, "object-removed",
                   G_CALLBACK(object_removed_cb), self);

out:
  return;
}

static void
is_udisks2_plugin_activate(PeasActivatable *activatable)
{
  IsUDisks2Plugin *self = IS_UDISKS2_PLUGIN(activatable);

  is_debug("udisks2", "Trying to get udisks client");
  udisks_client_new(NULL,
                    is_udisks2_plugin_client_ready_cb,
                    self);
}

static void
is_udisks2_plugin_deactivate(PeasActivatable *activatable)
{
  IsUDisks2Plugin *self = IS_UDISKS2_PLUGIN(activatable);
  IsUDisks2PluginPrivate *priv = self->priv;
  IsManager *manager;

  if (priv->client)
  {
    g_object_unref(priv->client);
  }
  manager = is_application_get_manager(priv->application);
  is_manager_remove_paths_with_prefix(manager, UDISKS2_PATH_PREFIX);
}

static void
is_udisks2_plugin_class_init(IsUDisks2PluginClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass, sizeof(IsUDisks2PluginPrivate));

  gobject_class->get_property = is_udisks2_plugin_get_property;
  gobject_class->set_property = is_udisks2_plugin_set_property;
  gobject_class->finalize = is_udisks2_plugin_finalize;

  g_object_class_override_property(gobject_class, PROP_OBJECT, "object");
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
{
  iface->activate = is_udisks2_plugin_activate;
  iface->deactivate = is_udisks2_plugin_deactivate;
}

static void
is_udisks2_plugin_class_finalize(IsUDisks2PluginClass *klass)
{
  /* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  is_udisks2_plugin_register_type(G_TYPE_MODULE(module));

  peas_object_module_register_extension_type(module,
      PEAS_TYPE_ACTIVATABLE,
      IS_TYPE_UDISKS2_PLUGIN);
}
