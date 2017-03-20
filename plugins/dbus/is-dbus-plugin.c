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

#include "is-dbus-plugin.h"
#include "is-org-gnome-shell-search-provider-generated.h"
#include "is-active-sensor-generated.h"
#include <indicator-sensors/is-application.h>
#include <indicator-sensors/is-log.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

static void peas_activatable_iface_init(PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(IsDBusPlugin,
                               is_dbus_plugin,
                               PEAS_TYPE_EXTENSION_BASE,
                               0,
                               G_IMPLEMENT_INTERFACE_DYNAMIC(PEAS_TYPE_ACTIVATABLE,
                                                             peas_activatable_iface_init));

enum
{
  PROP_OBJECT = 1,
};

struct _IsDBusPluginPrivate
{
  IsApplication *application;
  guint id;
  GDBusObjectManagerServer *sensors_object_manager;
  GDBusObjectManagerServer *search_object_manager;
  IsOrgGnomeShellSearchProvider2 *skeleton;
};

static void is_dbus_plugin_finalize(GObject *object);

static void
is_dbus_plugin_set_property(GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
  IsDBusPlugin *plugin = IS_DBUS_PLUGIN(object);

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
is_dbus_plugin_get_property(GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
  IsDBusPlugin *plugin = IS_DBUS_PLUGIN(object);

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
is_dbus_plugin_init(IsDBusPlugin *self)
{
  IsDBusPluginPrivate *priv =
    G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_DBUS_PLUGIN,
                                IsDBusPluginPrivate);
  self->priv = priv;
}

static void
is_dbus_plugin_finalize(GObject *object)
{
  IsDBusPlugin *self = (IsDBusPlugin *)object;
  IsDBusPluginPrivate *priv = self->priv;


  if (priv->application)
  {
    g_object_unref(priv->application);
    priv->application = NULL;
  }
  G_OBJECT_CLASS(is_dbus_plugin_parent_class)->finalize(object);
}

static void
sensor_property_changed(IsSensor *sensor,
                        GParamSpec *pspec,
                        gpointer data)
{
  const gchar *name = g_param_spec_get_name(pspec);
  IsActiveSensor *active_sensor = IS_ACTIVE_SENSOR(data);

  if (!g_strcmp0(name, "value"))
  {
    is_active_sensor_set_value(active_sensor,
                               is_sensor_get_value(sensor));
  }
  else if (!g_strcmp0(name, "label"))
  {
    is_active_sensor_set_label(active_sensor,
                               is_sensor_get_label(sensor));
  }
  else if (!g_strcmp0(name, "units"))
  {
    is_active_sensor_set_units(active_sensor,
                               is_sensor_get_units(sensor));
  }
  else if (!g_strcmp0(name, "icon-path"))
  {
    is_active_sensor_set_icon_path(active_sensor,
                                   is_sensor_get_icon_path(sensor));
  }
  else if (!g_strcmp0(name, "digits"))
  {
    is_active_sensor_set_digits(active_sensor,
                                is_sensor_get_digits(sensor));
  }
  else if (!g_strcmp0(name, "error"))
  {
    is_active_sensor_set_error(active_sensor,
                               is_sensor_get_error(sensor));
  }
}


static gchar *
dbus_sensor_object_path(IsSensor *sensor)
{
  gchar *path;
  /* Create a new D-Bus object at the path
   * /com/github/alexmurray/IndicatorSensors/ActiveSensors/path where path
   * is path of each sensor */
  path = g_strdup_printf("/com/github/alexmurray/IndicatorSensors/ActiveSensors/%s",
                         is_sensor_get_path(sensor));

  /* ensure valid path */
  path = g_strcanon(path,
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz"
                    "1234567890_/",
                    '_');
  return path;
}

static void
sensor_position_changed(IsManager *manager,
                        IsSensor *sensor,
                        gint i,
                        IsDBusPlugin *self)
{
  IsDBusPluginPrivate *priv;
  IsObjectSkeleton *object;
  IsActiveSensor *active_sensor;
  gchar *path;

  priv = self->priv;
  path = dbus_sensor_object_path(sensor);
  object = IS_OBJECT_SKELETON(g_dbus_object_manager_get_object(G_DBUS_OBJECT_MANAGER(priv->sensors_object_manager),
                                                               path));
  g_object_get(object, "active-sensor", &active_sensor, NULL);
  is_active_sensor_set_index(active_sensor, i);
  g_object_unref(object);
  g_free(path);
}

static void
sensor_enabled(IsManager *manager,
               IsSensor *sensor,
               gint i,
               IsDBusPlugin *self)
{
  IsDBusPluginPrivate *priv;
  gchar *path;
  IsActiveSensor *active_sensor;
  IsObjectSkeleton *object;

  priv = self->priv;

  path = dbus_sensor_object_path(sensor);
  object = is_object_skeleton_new(path);
  is_debug("dbus-plugin", "Creating an ActiveSensor at path %s\n", path);
  g_free(path);

  /* Make the newly created object export the interface
   * com.github.alexmurray.indicator-sensors.ActiveSensor
   * (note that @object takes its own reference to
   * @active_sensor).
   */
  active_sensor = is_active_sensor_skeleton_new();
  is_active_sensor_set_path(active_sensor, is_sensor_get_path(sensor));
  is_active_sensor_set_label(active_sensor, is_sensor_get_label(sensor));
  is_active_sensor_set_units(active_sensor, is_sensor_get_units(sensor));
  is_active_sensor_set_value(active_sensor, is_sensor_get_value(sensor));
  is_active_sensor_set_digits(active_sensor, is_sensor_get_digits(sensor));
  is_active_sensor_set_index(active_sensor, i);
  is_active_sensor_set_icon_path(active_sensor, is_sensor_get_icon_path(sensor));

  g_signal_connect(sensor, "notify",
                   G_CALLBACK(sensor_property_changed), active_sensor);
  is_object_skeleton_set_active_sensor(object, active_sensor);
  g_object_unref(active_sensor);

  /* Export the object (@manager takes its own reference to
   * @object) */
  g_dbus_object_manager_server_export(priv->sensors_object_manager,
                                      G_DBUS_OBJECT_SKELETON(object));
  g_object_unref(object);
}

static void
sensor_disabled(IsManager *manager,
                IsSensor *sensor,
                IsDBusPlugin *self)
{
  IsDBusPluginPrivate *priv;
  IsObjectSkeleton *object;
  IsActiveSensor *active_sensor;
  gchar *path;

  priv = self->priv;
  path = dbus_sensor_object_path(sensor);
  object = IS_OBJECT_SKELETON(g_dbus_object_manager_get_object(G_DBUS_OBJECT_MANAGER(priv->sensors_object_manager),
                                                               path));
  g_object_get(object, "active-sensor", &active_sensor, NULL);
  g_object_unref(object);
  g_signal_handlers_disconnect_by_func(sensor, sensor_property_changed, active_sensor);
  g_dbus_object_manager_server_unexport(priv->sensors_object_manager,
                                        path);
  g_free(path);
}

static GDBusNodeInfo *introspection_data = NULL;

/* Introspection data for IndicatorSensors */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='com.github.alexmurray.IndicatorSensors'>"
  "    <method name='ShowPreferences'>"
  "    </method>"
  "    <method name='ShowIndicator'>"
  "    </method>"
  "    <method name='HideIndicator'>"
  "    </method>"
  "  </interface>"
  "</node>";

static void
handle_method_call(GDBusConnection *connection,
                   const gchar *sender,
                   const gchar *object_path,
                   const gchar *interface_name,
                   const gchar *method_name,
                   GVariant *parameters,
                   GDBusMethodInvocation *invocation,
                   gpointer user_data)
{
  IsDBusPlugin *self = IS_DBUS_PLUGIN(user_data);
  IsDBusPluginPrivate *priv = self->priv;

  if (g_strcmp0(method_name, "ShowPreferences") == 0)
  {
    is_application_show_preferences(priv->application);
  }
  else if (g_strcmp0(method_name, "ShowIndicator") == 0)
  {
    is_application_set_show_indicator(priv->application, TRUE);
  }
  else if (g_strcmp0(method_name, "HideIndicator") == 0)
  {
    is_application_set_show_indicator(priv->application, FALSE);
  }
  g_dbus_method_invocation_return_value(invocation, NULL);
}


/* for now */
static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  NULL,
  NULL,
};

static GVariant *
get_result_set(IsDBusPlugin *self,
               gchar **terms)
{
  GVariantBuilder builder;
  GSList *sensors, *_list;
  IsManager *manager;

  g_variant_builder_init(&builder, G_VARIANT_TYPE ("as"));
  manager = is_application_get_manager(self->priv->application);
  sensors = is_manager_get_enabled_sensors_list(manager);

  for (_list = sensors;
       _list != NULL;
       _list = _list->next)
  {
    int i;
    IsSensor *sensor = IS_SENSOR(_list->data);
    const gchar *label = is_sensor_get_label(sensor);

    for (i = 0; terms[i] != NULL; i++)
    {
      if (g_str_match_string(terms[i], label, TRUE) ||
          g_str_match_string(terms[i], "sensors", TRUE))
      {
        is_debug("dbus", "matched term %s against label %s", terms[i], label);
        g_variant_builder_add(&builder, "s", is_sensor_get_path(sensor));
      }
    }
    g_object_unref(sensor);
  }
  g_slist_free(sensors);

  return g_variant_new("(as)", &builder);
}

static GVariant *
get_result_metas (IsDBusPlugin *self,
                  const gchar **results)
{
  IsManager *manager;
  gint idx;
  GVariantBuilder meta, metas;

  g_variant_builder_init(&metas, G_VARIANT_TYPE ("aa{sv}"));
  manager = is_application_get_manager(self->priv->application);

  for (idx = 0; results[idx] != NULL; idx++)
  {
    IsSensor *sensor = is_manager_get_sensor(manager, results[idx]);
    GIcon *gicon = g_themed_icon_new(is_sensor_get_icon(sensor));
    gchar *name;
    gchar *gicon_str;

    g_variant_builder_init(&meta, G_VARIANT_TYPE ("a{sv}"));
    g_variant_builder_add(&meta, "{sv}",
                          "id", g_variant_new_string(results[idx]));

    name = g_strdup_printf("%s %2.*f%s",
                           is_sensor_get_label(sensor),
                           is_sensor_get_digits(sensor),
                           is_sensor_get_value(sensor),
                           is_sensor_get_units(sensor));
    g_variant_builder_add(&meta, "{sv}",
                          "name", g_variant_new_string(name));
    g_free(name);

    gicon_str = g_icon_to_string (gicon);
    g_variant_builder_add (&meta, "{sv}",
                           "gicon", g_variant_new_string (gicon_str));
    g_free(gicon_str);

    g_variant_builder_add_value(&metas, g_variant_builder_end (&meta));
  }

  return g_variant_new("(aa{sv})", &metas);
}

static void
handle_get_initial_result_set (IsOrgGnomeShellSearchProvider2  *skeleton,
                               GDBusMethodInvocation              *invocation,
                               gchar                             **terms,
                               gpointer                            user_data)
{
  IsDBusPlugin *self = IS_DBUS_PLUGIN(user_data);
  gchar *joined_terms = g_strjoinv (" ", terms);

  is_debug("dbus", "GetInitialResultSet() called with %s", joined_terms);
  g_free (joined_terms);

  g_dbus_method_invocation_return_value (invocation, get_result_set(self, terms));
}

static void
handle_get_subsearch_result_set (IsOrgGnomeShellSearchProvider2  *skeleton,
                                 GDBusMethodInvocation              *invocation,
                                 gchar                             **previous_results,
                                 gchar                             **terms,
                                 gpointer                            user_data)
{
  IsDBusPlugin *self = IS_DBUS_PLUGIN(user_data);
  gchar *joined_terms = g_strjoinv (" ", terms);

  is_debug("dbus", "GetSubSearchResultSet() called with %s", joined_terms);
  g_free (joined_terms);

  g_dbus_method_invocation_return_value(invocation, get_result_set(self, terms));
}

static void
handle_get_result_metas (IsOrgGnomeShellSearchProvider2  *skeleton,
                         GDBusMethodInvocation              *invocation,
                         gchar                             **results,
                         gpointer                            user_data)
{
  IsDBusPlugin *self = IS_DBUS_PLUGIN(user_data);
  gint idx;

  is_debug("dbus", "GetResultMetas() called for results");

  for (idx = 0; results[idx] != NULL; idx++)
    is_debug("dbus", "   %s\n", results[idx]);

  is_debug("dbus", "\n");

  g_dbus_method_invocation_return_value (invocation,
                                         get_result_metas (self, (const gchar **)results));
}

static void
handle_launch_search (IsOrgGnomeShellSearchProvider2  *skeleton,
                      GDBusMethodInvocation              *invocation,
                      gchar                             **terms,
                      guint32                             timestamp,
                      gpointer                            user_data)
{
  gchar *joined_terms = g_strjoinv (" ", terms);

  is_debug("dbus", "LaunchSearch() called with %s\n", joined_terms);
  g_free (joined_terms);

  g_dbus_method_invocation_return_value (invocation, NULL);
}

static void
handle_activate_result (IsOrgGnomeShellSearchProvider2  *skeleton,
                        GDBusMethodInvocation              *invocation,
                        gchar                              *result,
                        gchar                             **terms,
                        guint32                             timestamp,
                        gpointer                            user_data)
{
  gchar *joined_terms = g_strjoinv (" ", terms);

  is_debug("dbus", "ActivateResult() called for %s and result %s\n",
           joined_terms, result);
  g_free (joined_terms);

  g_dbus_method_invocation_return_value (invocation, NULL);
}

static void
on_bus_acquired(GDBusConnection *connection,
                const gchar     *name,
                gpointer         user_data)
{
  IsDBusPlugin *self;
  IsDBusPluginPrivate *priv;
  IsManager *manager;
  guint id;
  GError *error = NULL;
  GSList *sensors, *_list;
  gint i = 0;

  self = IS_DBUS_PLUGIN(user_data);
  priv = self->priv;

  is_debug("dbus-plugin", "Acquired a message bus connection\n");

  priv->search_object_manager = g_dbus_object_manager_server_new ("/com/github/alexmurray/IndicatorSensors/SearchProvider");
  priv->skeleton = is_org_gnome_shell_search_provider2_skeleton_new ();

  g_signal_connect (priv->skeleton, "handle-get-initial-result-set",
                    G_CALLBACK (handle_get_initial_result_set), self);
  g_signal_connect (priv->skeleton, "handle-get-subsearch-result-set",
                    G_CALLBACK (handle_get_subsearch_result_set), self);
  g_signal_connect (priv->skeleton, "handle-get-result-metas",
                    G_CALLBACK (handle_get_result_metas), self);
  g_signal_connect (priv->skeleton, "handle-activate-result",
                    G_CALLBACK (handle_activate_result), self);
  g_signal_connect (priv->skeleton, "handle-launch-search",
                    G_CALLBACK (handle_launch_search), self);

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->skeleton),
                                    connection,
                                    "/com/github/alexmurray/IndicatorSensors/SearchProvider",
                                    NULL);
  g_dbus_object_manager_server_set_connection (priv->search_object_manager, connection);

  id = g_dbus_connection_register_object(connection,
                                         "/com/github/alexmurray/IndicatorSensors",
                                         introspection_data->interfaces[0],
                                         &interface_vtable,
                                         self,
                                         NULL,
                                         &error);
  if (!id)
  {
    is_warning("dbus-plugin", "Unabled to register IndicatorSensors object on dbus: %s",
               error->message);
    g_error_free(error);
  }
  /* Create a new org.freedesktop.DBus.ObjectManager rooted at
   * /indicator-sensors/ActiveSensors */
  priv->sensors_object_manager = g_dbus_object_manager_server_new("/com/github/alexmurray/IndicatorSensors/ActiveSensors");

  manager = is_application_get_manager(priv->application);
  /* set up a skeleton object and sensor for each active sensor */
  sensors = is_manager_get_enabled_sensors_list(manager);
  for (_list = sensors; _list != NULL; _list = _list->next)
  {
    IsSensor *sensor;

    sensor = IS_SENSOR(_list->data);

    sensor_enabled(manager, sensor, i++, self);
  }
  g_signal_connect(manager, "sensor-enabled",
                   G_CALLBACK(sensor_enabled), self);
  g_signal_connect(manager, "sensor-disabled",
                   G_CALLBACK(sensor_disabled), self);
  g_signal_connect(manager, "sensor-position-changed",
                   G_CALLBACK(sensor_position_changed), self);
  /* Export all objects */
  g_dbus_object_manager_server_set_connection(priv->sensors_object_manager, connection);
}

static void
on_name_acquired(GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  is_debug("dbus-plugin", "Acquired the name %s\n", name);
}

static void
on_name_lost(GDBusConnection *connection,
             const gchar     *name,
             gpointer         user_data)
{
  is_debug("dbus-plugin", "Lost the name %s\n", name);
}

static void
is_dbus_plugin_activate(PeasActivatable *activatable)
{
  IsDBusPlugin *self = IS_DBUS_PLUGIN(activatable);
  IsDBusPluginPrivate *priv = self->priv;

  /* get our dbus name */
  priv->id = g_bus_own_name(G_BUS_TYPE_SESSION,
                            "com.github.alexmurray.IndicatorSensors",
                            G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                            G_BUS_NAME_OWNER_FLAGS_REPLACE,
                            on_bus_acquired,
                            on_name_acquired,
                            on_name_lost,
                            self,
                            NULL);
  /* connect to sensors enabled / disabled signals and export them over
   * dbus - also iterate through any existing enabled sensors and export
   * those as well */

  /* setup dbus object manager */
}

static void
is_dbus_plugin_deactivate(PeasActivatable *activatable)
{
  IsDBusPlugin *self = IS_DBUS_PLUGIN(activatable);
  IsDBusPluginPrivate *priv = self->priv;

  /* teardown dbus object manager */

  /* disconnect from signals */
  g_bus_unown_name(priv->id);
}

static void
is_dbus_plugin_class_init(IsDBusPluginClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass, sizeof(IsDBusPluginPrivate));

  gobject_class->get_property = is_dbus_plugin_get_property;
  gobject_class->set_property = is_dbus_plugin_set_property;
  gobject_class->finalize = is_dbus_plugin_finalize;

  g_object_class_override_property(gobject_class, PROP_OBJECT, "object");

  /* do interface introspection data */
  introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
  g_assert(introspection_data != NULL);
}

static void
peas_activatable_iface_init(PeasActivatableInterface *iface)
{
  iface->activate = is_dbus_plugin_activate;
  iface->deactivate = is_dbus_plugin_deactivate;
}

static void
is_dbus_plugin_class_finalize(IsDBusPluginClass *klass)
{
  /* nothing to do */
}

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  is_dbus_plugin_register_type(G_TYPE_MODULE(module));

  peas_object_module_register_extension_type(module,
                                             PEAS_TYPE_ACTIVATABLE,
                                             IS_TYPE_DBUS_PLUGIN);
}
