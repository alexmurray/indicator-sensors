/*
 * Copyright (C) 2011-2025 Alex Murray <murray.alex@gmail.com>
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

#include "config.h"

#include "glib-object.h"
#include "is-application.h"
#include "is-indicator.h"
#include "is-log.h"
#include "is-notify.h"
#include <glib/gi18n.h>

/* how long to wait after the AppIndicator reports it is disconnected before
 * assuming there is no indicator/tray support on this desktop at all and
 * warning the user, rather than reacting to what may just be the desktop
 * shell's StatusNotifierWatcher not having registered yet at startup */
#define INDICATOR_NOT_SHOWN_TIMEOUT 10

typedef struct _IsIndicatorPrivate
{
  IsApplication *application;
  /* the path to the preferred primary sensor */
  gchar *primary_sensor_path;
  IsSensor *primary;
  IsIndicatorDisplayFlags display_flags;
  GSList *menu_items;
  guint indicator_not_shown_timeout_id;
} IsIndicatorPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IsIndicator, is_indicator, APP_INDICATOR_TYPE);

static void is_indicator_dispose(GObject *object);
static void is_indicator_finalize(GObject *object);
static void is_indicator_constructed(GObject *object);
static void is_indicator_get_property(GObject *object, guint property_id,
                                      GValue *value, GParamSpec *pspec);
static void is_indicator_set_property(GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec);
static void is_indicator_connection_changed(AppIndicator *indicator,
                                            gboolean connected, gpointer data);
static gboolean indicator_not_shown_timeout_cb(gpointer data);
static void sensor_enabled(IsManager *manager, IsSensor *sensor, gint position,
                           IsIndicator *self);
static void _sensor_disabled(IsSensor *sensor, IsIndicator *self);
static void sensor_disabled(IsManager *manager, IsSensor *sensor,
                            IsIndicator *self);
static void sensor_added(IsManager *manager, IsSensor *sensor,
                         IsIndicator *self);

static void update_sensor_menu_item_label(IsIndicator *self, IsSensor *sensor,
                                          GtkMenuItem *menu_item);

/* properties */
enum
{
  PROP_APPLICATION = 1,
  PROP_PRIMARY_SENSOR_PATH,
  PROP_DISPLAY_FLAGS,
  PROP_MENU, // ideally menu would be a property but AppIndicator
             // doesn't expose it as such
  LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = {NULL};

static gboolean
process_existing_sensors(IsIndicator *self)
{
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  IsManager *manager;
  GSList *sensors, *_list;
  gint i = 0;

  manager = is_application_get_manager(priv->application);

  /* fake addition of any sensors */
  sensors = is_manager_get_all_sensors_list(manager);
  for (_list = sensors; _list != NULL; _list = _list->next)
  {
    IsSensor *sensor = IS_SENSOR(_list->data);
    sensor_added(manager, IS_SENSOR(_list->data), self);
    g_object_unref(sensor);
  }
  g_slist_free(sensors);

  /* fake enabling of any sensors */
  sensors = is_manager_get_enabled_sensors_list(manager);
  for (_list = sensors; _list != NULL; _list = _list->next)
  {
    IsSensor *sensor = IS_SENSOR(_list->data);
    sensor_enabled(manager, IS_SENSOR(_list->data), i++, self);
    g_object_unref(sensor);
  }
  g_slist_free(sensors);

  return FALSE;
}

static void
is_indicator_constructed(GObject *object)
{
  IsIndicator *self = IS_INDICATOR(object);
  const gchar *snap;

  G_OBJECT_CLASS(is_indicator_parent_class)->constructed(object);

  const gchar *label = _("No Sensors");
  app_indicator_set_label(APP_INDICATOR(self), label, label);
  app_indicator_set_status(APP_INDICATOR(self), APP_INDICATOR_STATUS_ACTIVE);

  /* when running as a snap, our icons are installed under $SNAP rather
   * than a system icon theme directory, so the (unconfined) process which
   * renders the indicator icon can't find them by name unless we tell it
   * where to look */
  snap = g_getenv("SNAP");
  if (snap != NULL)
  {
    gchar *icon_theme_path =
        g_build_filename(snap, "usr", "share", "icons", NULL);
    app_indicator_set_icon_theme_path(APP_INDICATOR(self), icon_theme_path);
    g_free(icon_theme_path);
  }

  process_existing_sensors(IS_INDICATOR(object));
}

static void
is_indicator_class_init(IsIndicatorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->get_property = is_indicator_get_property;
  gobject_class->set_property = is_indicator_set_property;
  gobject_class->constructed = is_indicator_constructed;
  gobject_class->dispose = is_indicator_dispose;
  gobject_class->finalize = is_indicator_finalize;
  APP_INDICATOR_CLASS(klass)->connection_changed =
      is_indicator_connection_changed;

  properties[PROP_APPLICATION] = g_param_spec_object(
      "application", "application property", "application property blurp.",
      IS_TYPE_APPLICATION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_APPLICATION,
                                  properties[PROP_APPLICATION]);

  properties[PROP_PRIMARY_SENSOR_PATH] = g_param_spec_string(
      "primary-sensor-path", "path of preferred primary sensor",
      "path of preferred primary sensor", NULL,
      G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_PRIMARY_SENSOR_PATH,
                                  properties[PROP_PRIMARY_SENSOR_PATH]);

  properties[PROP_DISPLAY_FLAGS] = g_param_spec_int(
      "display-flags", "display flags property",
      "display flags property blurp.", IS_INDICATOR_DISPLAY_VALUE,
      IS_INDICATOR_DISPLAY_ALL, IS_INDICATOR_DISPLAY_ALL,
      G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_DISPLAY_FLAGS,
                                  properties[PROP_DISPLAY_FLAGS]);
  properties[PROP_MENU] = g_param_spec_object(
      "menu", "menu property", "menu property blurp.", GTK_TYPE_MENU,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_MENU,
                                  properties[PROP_MENU]);
}

static void
is_indicator_init(IsIndicator *self)
{
}

static void
is_indicator_get_property(GObject *object, guint property_id, GValue *value,
                          GParamSpec *pspec)
{
  IsIndicator *self = IS_INDICATOR(object);

  switch (property_id)
  {
  case PROP_APPLICATION:
    g_value_set_object(value, is_indicator_get_application(self));
    break;
  case PROP_PRIMARY_SENSOR_PATH:
    g_value_set_string(value, is_indicator_get_primary_sensor_path(self));
    break;
  case PROP_DISPLAY_FLAGS:
    g_value_set_int(value, is_indicator_get_display_flags(self));
    break;
  case PROP_MENU:
    g_value_set_object(value, app_indicator_get_menu(APP_INDICATOR(self)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void
is_indicator_set_property(GObject *object, guint property_id,
                          const GValue *value, GParamSpec *pspec)
{
  IsIndicator *self = IS_INDICATOR(object);
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  IsManager *manager;

  switch (property_id)
  {
  case PROP_APPLICATION:
    g_assert(!priv->application);
    priv->application = g_object_ref(g_value_get_object(value));
    manager = is_application_get_manager(priv->application);
    g_signal_connect(manager, "sensor-enabled", G_CALLBACK(sensor_enabled),
                     self);
    g_signal_connect(manager, "sensor-disabled", G_CALLBACK(sensor_disabled),
                     self);
    g_signal_connect(manager, "sensor-added", G_CALLBACK(sensor_added), self);
    break;
  case PROP_PRIMARY_SENSOR_PATH:
    is_indicator_set_primary_sensor_path(self, g_value_get_string(value));
    break;
  case PROP_DISPLAY_FLAGS:
    is_indicator_set_display_flags(self, g_value_get_int(value));
    break;
  case PROP_MENU:
    app_indicator_set_menu(APP_INDICATOR(self),
                           GTK_MENU(g_value_get_object(value)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static gboolean
indicator_not_shown_timeout_cb(gpointer data)
{
  IsIndicator *self = IS_INDICATOR(data);
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  GNotification *notification;

  is_warning("indicator", "No indicator/tray support was found on this "
                          "desktop - unable to display sensors");
  notification = is_notify(
      "indicator-not-shown", IS_NOTIFY_LEVEL_WARNING,
      _("Unable To Display Sensors"), "app.preferences",
      _("%s could not find a compatible indicator area on this desktop to "
        "display sensor readings in - you may need to install an "
        "AppIndicator / Ayatana indicator support extension. Sensors can "
        "still be configured via the Preferences window."),
      PACKAGE_NAME);
  g_object_unref(notification);

  priv->indicator_not_shown_timeout_id = 0;
  return G_SOURCE_REMOVE;
}

static void
is_indicator_connection_changed(AppIndicator *indicator, gboolean connected,
                                gpointer data)
{
  IsIndicator *self = IS_INDICATOR(indicator);
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  GtkMenuItem *item;

  is_debug("indicator", "connection changed: %s",
          connected ? "connected" : "disconnected");

  if (connected)
  {
    if (priv->indicator_not_shown_timeout_id)
    {
      g_source_remove(priv->indicator_not_shown_timeout_id);
      priv->indicator_not_shown_timeout_id = 0;
    }
    is_notify_withdraw("indicator-not-shown");
  }
  else if (!priv->indicator_not_shown_timeout_id)
  {
    /* don't alert immediately - give the desktop shell a chance to
     * register a StatusNotifierWatcher first, which may not have
     * happened yet this early during startup */
    priv->indicator_not_shown_timeout_id = g_timeout_add_seconds(
        INDICATOR_NOT_SHOWN_TIMEOUT, indicator_not_shown_timeout_cb, self);
  }

  if (!priv->primary)
  {
    goto out;
  }
  /* force an update of the primary sensor to reset icon etc */
  if (!priv->primary)
  {
    goto out;
  }

  item =
      (GtkMenuItem *)(g_object_get_data(G_OBJECT(priv->primary), "menu-item"));
  if (!item)
  {
    goto out;
  }
  update_sensor_menu_item_label(self, priv->primary, item);

out:
  return;
}

static void
is_indicator_dispose(GObject *object)
{
  IsIndicator *self = (IsIndicator *)object;
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  IsManager *manager;
  GSList *sensors, *_list;

  if (priv->indicator_not_shown_timeout_id)
  {
    g_source_remove(priv->indicator_not_shown_timeout_id);
    priv->indicator_not_shown_timeout_id = 0;
  }

  manager = is_application_get_manager(priv->application);
  g_signal_handlers_disconnect_by_func(manager, sensor_enabled, self);
  g_signal_handlers_disconnect_by_func(manager, sensor_disabled, self);
  g_signal_handlers_disconnect_by_func(manager, sensor_added, self);
  /* fake disabling of any sensors */
  sensors = is_manager_get_enabled_sensors_list(manager);
  for (_list = sensors; _list != NULL; _list = _list->next)
  {
    IsSensor *sensor = IS_SENSOR(_list->data);
    _sensor_disabled(IS_SENSOR(_list->data), self);
    g_object_unref(sensor);
  }
  g_slist_free(sensors);

  G_OBJECT_CLASS(is_indicator_parent_class)->dispose(object);
}

static void
is_indicator_finalize(GObject *object)
{
  IsIndicator *self = (IsIndicator *)object;
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);

  g_object_unref(priv->application);

  G_OBJECT_CLASS(is_indicator_parent_class)->finalize(object);
}

static void
update_sensor_menu_item_label(IsIndicator *self, IsSensor *sensor,
                              GtkMenuItem *menu_item)
{
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  gchar *text;

  text = g_strdup_printf(
      "%s %2.*f%s", is_sensor_get_label(sensor), is_sensor_get_digits(sensor),
      is_sensor_get_value(sensor), is_sensor_get_units(sensor));
  gtk_menu_item_set_label(menu_item, text);
  g_free(text);
  text = NULL;

  if (sensor == priv->primary)
  {
    gboolean connected;

    g_object_get(self, "connected", &connected, NULL);
    /* using fallback so just set icon */
    if (!connected)
    {
      app_indicator_set_icon_full(APP_INDICATOR(self), PACKAGE,
                                  is_sensor_get_label(sensor));
      return;
    }

    if (priv->display_flags & IS_INDICATOR_DISPLAY_VALUE)
    {
      text = g_strdup_printf("%2.*f%s", is_sensor_get_digits(sensor),
                             is_sensor_get_value(sensor),
                             is_sensor_get_units(sensor));
    }
    if (priv->display_flags & IS_INDICATOR_DISPLAY_LABEL)
    {
      /* join label to existing text - if text is NULL this
         will just show label */
      gchar *old = text;
      text = g_strjoin(" ", is_sensor_get_label(sensor), text, NULL);
      g_free(old);
    }
    if (priv->display_flags & IS_INDICATOR_DISPLAY_ICON)
    {
      app_indicator_set_icon_full(APP_INDICATOR(self),
                                  is_sensor_get_icon_path(sensor),
                                  is_sensor_get_label(sensor));
    }
    else
    {
      /* set to a 1x1 transparent icon for no icon */
      app_indicator_set_icon_full(APP_INDICATOR(self),
                                  "indicator-sensors-no-icon",
                                  is_sensor_get_label(sensor));
    }
    /* prefix with a space */
    gchar *old = text;
    text = g_strjoin(" ", "", text, NULL);
    g_free(old);
    app_indicator_set_label(APP_INDICATOR(self), text, text);
    g_free(text);
    app_indicator_set_status(APP_INDICATOR(self),
                             is_sensor_get_alarmed(sensor)
                                 ? APP_INDICATOR_STATUS_ATTENTION
                                 : APP_INDICATOR_STATUS_ACTIVE);
  }
}

static void
sensor_notify(IsSensor *sensor, GParamSpec *psec, IsIndicator *self)
{
  GtkMenuItem *menu_item;

  g_return_if_fail(IS_IS_SENSOR(sensor));
  g_return_if_fail(IS_IS_INDICATOR(self));

  menu_item = GTK_MENU_ITEM(g_object_get_data(G_OBJECT(sensor), "menu-item"));
  if (menu_item)
  {
    update_sensor_menu_item_label(self, sensor, menu_item);
  }
}

static void
_sensor_disabled(IsSensor *sensor, IsIndicator *self)
{
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  GtkWidget *menu_item;

  is_debug("indicator", "disabling sensor %s", is_sensor_get_path(sensor));

  /* destroy menu item */
  menu_item = GTK_WIDGET(g_object_get_data(G_OBJECT(sensor), "menu-item"));
  priv->menu_items = g_slist_remove(priv->menu_items, menu_item);
  gtk_container_remove(
      GTK_CONTAINER(app_indicator_get_menu(APP_INDICATOR(self))), menu_item);
  g_object_set_data(G_OBJECT(sensor), "menu-item", NULL);

  g_signal_handlers_disconnect_by_func(sensor, sensor_notify, self);
}

static void
sensor_disabled(IsManager *manager, IsSensor *sensor, IsIndicator *self)
{
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);

  _sensor_disabled(sensor, self);

  if (priv->menu_items)
  {
    /* ignore if was not primary sensor, otherwise, get a new one */
    if (sensor != priv->primary)
    {
      goto out;
    }
    /* choose top-most menu item and set it as primary one */
    GtkWidget *menu_item = GTK_WIDGET(priv->menu_items->data);
    /* activate it to make this the new primary sensor */
    gtk_menu_item_activate(GTK_MENU_ITEM(menu_item));
  }
  else
  {
    const gchar *label = _("No active sensors");
    app_indicator_set_label(APP_INDICATOR(self), label, label);
    g_clear_object(&priv->primary);
  }
out:
  return;
}

static void
sensor_menu_item_toggled(GtkMenuItem *menu_item, IsIndicator *self)
{
  IsSensor *sensor;
  gboolean active = FALSE;

  g_return_if_fail(IS_IS_INDICATOR(self));

  /* only set as primary if was toggled to active state since we may have
   * untoggled it instead */
  active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
  if (active)
  {
    sensor = IS_SENSOR(g_object_get_data(G_OBJECT(menu_item), "sensor"));
    is_debug(
        "indicator",
        "Sensor %s menu-item toggled to active - setting as primary sensor",
        is_sensor_get_path(sensor));
    is_indicator_set_primary_sensor_path(self, is_sensor_get_path(sensor));
  }
}

static void
sensor_enabled(IsManager *manager, IsSensor *sensor, gint position,
               IsIndicator *self)
{
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);

  /* make sure we haven't seen this sensor before - if sensor has a
   * menu-item then ignore it */
  if (!g_object_get_data(G_OBJECT(sensor), "menu-item"))
  {
    GtkMenu *menu;
    GtkWidget *menu_item;

    is_debug("indicator", "Creating menu item for newly enabled sensor %s",
             is_sensor_get_path(sensor));

    g_signal_connect(sensor, "notify::value", G_CALLBACK(sensor_notify), self);
    g_signal_connect(sensor, "notify::label", G_CALLBACK(sensor_notify), self);
    g_signal_connect(sensor, "notify::alarmed", G_CALLBACK(sensor_notify),
                     self);
    g_signal_connect(sensor, "notify::low-value", G_CALLBACK(sensor_notify),
                     self);
    g_signal_connect(sensor, "notify::high-value", G_CALLBACK(sensor_notify),
                     self);
    /* add a menu entry for this sensor */
    menu = app_indicator_get_menu(APP_INDICATOR(self));
    menu_item = gtk_check_menu_item_new();
    gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
    g_object_set_data(G_OBJECT(sensor), "menu-item", menu_item);
    g_object_set_data(G_OBJECT(menu_item), "sensor", sensor);

    priv->menu_items = g_slist_insert(priv->menu_items, menu_item, position);
    /* if we haven't seen our primary sensor yet or if this is the
     * primary sensor, display this as primary anyway */
    if (!priv->primary ||
        g_strcmp0(is_sensor_get_path(sensor), priv->primary_sensor_path) == 0)
    {
      is_debug("indicator", "Using sensor with path %s as primary",
               is_sensor_get_path(sensor));
      if (priv->primary)
      {
        GtkCheckMenuItem *item;
        /* uncheck menu item if exists for this
         * existing primary sensor */
        item = (GtkCheckMenuItem *)(g_object_get_data(G_OBJECT(priv->primary),
                                                      "menu-item"));
        if (item)
        {
          is_debug("indicator", "Unchecking current primary sensor item");
          gtk_check_menu_item_set_active(item, FALSE);
        }
        g_object_unref(priv->primary);
      }
      priv->primary = g_object_ref(sensor);
      is_debug("indicator", "Checking new primary sensor item");
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
      update_sensor_menu_item_label(self, sensor, GTK_MENU_ITEM(menu_item));
    }
    /* connect to toggled signal now - if we connect to it earlier
     * we may interpret the above menu_item_set_active as a user
     * initiated setting of the primary sensor rather than us just
     * picking the first available sensor */
    g_signal_connect(menu_item, "toggled", G_CALLBACK(sensor_menu_item_toggled),
                     self);
    gtk_widget_show_all(menu_item);

    update_sensor_menu_item_label(self, sensor, GTK_MENU_ITEM(menu_item));
    gtk_menu_shell_insert(GTK_MENU_SHELL(menu), menu_item, position);
  }
  else
  {
    is_debug("indicator",
             "Newly enabled sensor %s already has a menu-item, ignoring...",
             is_sensor_get_path(sensor));
  }
}

static void
sensor_added(IsManager *manager, IsSensor *sensor, IsIndicator *self)
{
  /* if a sensor has been added and we haven't yet got any enabled sensors
     to display (and hence no primary sensor), change our text to show
     this */
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  if (!priv->menu_items)
  {
    const gchar *label = _("No active sensors");
    app_indicator_set_label(APP_INDICATOR(self), label, label);
  }
}

IsIndicator *
is_indicator_new(IsApplication *application, GtkMenu *menu)
{
  IsIndicator *self =
      g_object_new(IS_TYPE_INDICATOR, "id", PACKAGE, "category", "Hardware",
                   "application", application, "menu", menu, "icon-name",
                   PACKAGE, "title", PACKAGE_NAME, NULL);
  return IS_INDICATOR(self);
}

void
is_indicator_set_primary_sensor_path(IsIndicator *self, const gchar *path)
{
  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);

  g_return_if_fail(IS_IS_INDICATOR(self));

  if (g_strcmp0(priv->primary_sensor_path, path) != 0 &&
      g_strcmp0(path, "") != 0)
  {
    IsSensor *sensor;

    is_debug("indicator", "new primary sensor path %s (previously %s)", path,
             priv->primary_sensor_path);

    /* uncheck current primary sensor label - may be NULL as is
     * already disabled */
    if (priv->primary)
    {
      GtkCheckMenuItem *item;
      item = (GtkCheckMenuItem *)(g_object_get_data(G_OBJECT(priv->primary),
                                                    "menu-item"));
      if (item)
      {
        gtk_check_menu_item_set_active(item, FALSE);
      }
      g_object_unref(priv->primary);
    }

    g_free(priv->primary_sensor_path);
    priv->primary_sensor_path = g_strdup(path);

    is_debug("indicator", "Setting primary sensor path to: %s", path);

    /* try and activate this sensor if it exists */
    sensor =
        is_manager_get_sensor(is_application_get_manager(priv->application),
                              priv->primary_sensor_path);
    if (sensor)
    {
      GtkCheckMenuItem *item = (GtkCheckMenuItem *)(g_object_get_data(
          G_OBJECT(sensor), "menu-item"));
      /* take reference from manager */
      priv->primary = sensor;
      if (item)
      {
        gtk_check_menu_item_set_active(item, TRUE);
        update_sensor_menu_item_label(self, sensor, GTK_MENU_ITEM(item));
      }
    }

    g_object_notify_by_pspec(G_OBJECT(self),
                             properties[PROP_PRIMARY_SENSOR_PATH]);
  }
}

const gchar *
is_indicator_get_primary_sensor_path(IsIndicator *self)
{
  g_return_val_if_fail(IS_IS_INDICATOR(self), NULL);

  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  return priv->primary_sensor_path;
}

IsApplication *
is_indicator_get_application(IsIndicator *self)
{
  g_return_val_if_fail(IS_IS_INDICATOR(self), NULL);

  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  return priv->application;
}

void
is_indicator_set_display_flags(IsIndicator *self,
                               IsIndicatorDisplayFlags display_flags)
{
  g_return_if_fail(IS_IS_INDICATOR(self));

  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);

  if (display_flags != priv->display_flags)
  {
    priv->display_flags = display_flags;
    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_DISPLAY_FLAGS]);

    if (priv->primary)
    {
      GtkMenuItem *item = GTK_MENU_ITEM(
          g_object_get_data(G_OBJECT(priv->primary), "menu-item"));
      if (item)
      {
        update_sensor_menu_item_label(self, priv->primary, item);
      }
    }
  }
}

IsIndicatorDisplayFlags
is_indicator_get_display_flags(IsIndicator *self)
{
  g_return_val_if_fail(IS_IS_INDICATOR(self), 0);

  IsIndicatorPrivate *priv = is_indicator_get_instance_private(self);
  return priv->display_flags;
}
