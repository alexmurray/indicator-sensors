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
#include "config.h"
#endif

#include "is-indicator.h"
#include "is-application.h"
#include "is-sensor-dialog.h"
#include "is-log.h"
#include <math.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (IsIndicator, is_indicator, IS_INDICATOR_PARENT_TYPE);

static void is_indicator_dispose(GObject *object);
static void is_indicator_finalize(GObject *object);
static void is_indicator_constructed(GObject *object);
static void is_indicator_get_property(GObject *object,
                                      guint property_id, GValue *value, GParamSpec *pspec);
static void is_indicator_set_property(GObject *object,
                                      guint property_id, const GValue *value, GParamSpec *pspec);
#if HAVE_APPINDICATOR
static void is_indicator_connection_changed(AppIndicator *indicator,
                                            gboolean connected,
                                            gpointer data);
#endif
static void sensor_enabled(IsManager *manager,
                           IsSensor *sensor,
                           gint position,
                           IsIndicator *self);
static void _sensor_disabled(IsSensor *sensor,
                             IsIndicator *self);
static void sensor_disabled(IsManager *manager,
                            IsSensor *sensor,
                            IsIndicator *self);
static void sensor_added(IsManager *manager,
                         IsSensor *sensor,
                         IsIndicator *self);

static void
update_sensor_menu_item_label(IsIndicator *self,
                              IsSensor *sensor,
                              GtkMenuItem *menu_item);

/* properties */
enum {
        PROP_APPLICATION = 1,
        PROP_PRIMARY_SENSOR_PATH,
        PROP_DISPLAY_FLAGS,
        LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = {NULL};

struct _IsIndicatorPrivate
{
        IsApplication *application;
        /* the path to the preferred primary sensor */
        gchar *primary_sensor_path;
        IsSensor *primary;
        IsIndicatorDisplayFlags display_flags;
        GSList *menu_items;
};

        static GtkMenu *
is_indicator_get_menu(IsIndicator *self)
{
        GtkMenu *menu;

#if HAVE_APPINDICATOR
        menu = app_indicator_get_menu(APP_INDICATOR(self));
#else
        menu = GTK_MENU(g_object_get_data(G_OBJECT(self), "indicator-menu"));
#endif
        return menu;
}

#if !HAVE_APPINDICATOR
        static void
popup_menu(GtkStatusIcon *status_icon,
           guint button,
           guint activate_time,
           gpointer user_data)
{
        IsIndicator *self = IS_INDICATOR(status_icon);
        gtk_menu_popup(is_indicator_get_menu(self), NULL, NULL,
                       gtk_status_icon_position_menu,
                       self, button, activate_time);
}
#endif

        static void
is_indicator_set_menu(IsIndicator *self,
                      GtkMenu *menu)
{
#if HAVE_APPINDICATOR
        app_indicator_set_menu(APP_INDICATOR(self), menu);
#else
        g_object_set_data_full(G_OBJECT(self), "indicator-menu", menu,
                               (GDestroyNotify)gtk_widget_destroy);
        g_signal_connect(self, "popup-menu", G_CALLBACK(popup_menu), self);
#endif
}

        static gboolean
fake_add_enable_sensors(IsIndicator *self)
{
        IsManager *manager;
        GSList *sensors, *_list;
        gint i = 0;

        manager = is_application_get_manager(self->priv->application);

        /* fake addition of any sensors */
        sensors = is_manager_get_all_sensors_list(manager);
        for (_list = sensors; _list != NULL; _list = _list->next) {
                IsSensor *sensor = IS_SENSOR(_list->data);
                sensor_added(manager, IS_SENSOR(_list->data), self);
                g_object_unref(sensor);
        }
        g_slist_free(sensors);

        /* fake enabling of any sensors */
        sensors = is_manager_get_enabled_sensors_list(manager);
        for (_list = sensors; _list != NULL; _list = _list->next) {
                IsSensor *sensor = IS_SENSOR(_list->data);
                sensor_enabled(manager, IS_SENSOR(_list->data), i++,
                               self);
                g_object_unref(sensor);
        }
        g_slist_free(sensors);

        return FALSE;
}

static void prefs_action(GtkAction *action,
                         IsIndicator *self);
static void about_action(GtkAction *action,
                         IsIndicator *self);
static void quit_action(GtkAction *action,
                        IsIndicator *self);

static GtkActionEntry entries[] = {
        { "Preferences", "application-preferences", N_("Preferences…"), NULL,
                N_("Preferences"), G_CALLBACK(prefs_action) },
        { "About", "about", N_("About…"), NULL,
                N_("About"), G_CALLBACK(about_action) },
        { "Quit", GTK_STOCK_QUIT, N_("Quit"), NULL,
                N_("Quit"), G_CALLBACK(quit_action) },
};
static guint n_entries = G_N_ELEMENTS(entries);

static const gchar *ui_info =
"<ui>"
"  <popup name='Indicator'>"
"    <menuitem action='Preferences' />"
"    <menuitem action='About' />"
"    <menuitem action='Quit' />"
"  </popup>"
"</ui>";

static void prefs_action(GtkAction *action,
                         IsIndicator *self)
{
        is_application_show_preferences(self->priv->application);
}

static void about_action(GtkAction *action,
                         IsIndicator *self)
{
        is_application_show_about(self->priv->application);
}

static void quit_action(GtkAction *action,
                        IsIndicator *self)
{
        is_application_quit(self->priv->application);
}

        static void
is_indicator_set_label(IsIndicator *self,
                       const gchar *label)
{
#if HAVE_APPINDICATOR
        app_indicator_set_label(APP_INDICATOR(self), label, label);
#else
        gtk_status_icon_set_tooltip_text(GTK_STATUS_ICON(self), label);
#endif
}

        static void
is_indicator_constructed(GObject *object)
{
        IsIndicator *self = IS_INDICATOR(object);
        GtkActionGroup *action_group;
        GtkUIManager *ui_manager;
        GError *error = NULL;
        GtkWidget *menu;

        action_group = gtk_action_group_new("AppActions");
        gtk_action_group_add_actions(action_group,
                                     entries, n_entries,
                                     self);

        ui_manager = gtk_ui_manager_new();
        gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
        if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_info, -1, &error)) {
                g_error("Failed to build menus: %s\n", error->message);
        }

        menu = gtk_ui_manager_get_widget(ui_manager, "/ui/Indicator");
        /* manually add separator since specifying it in the ui description
           means it gets optimised out (since there is no menu item above it)
           but if we manually add it and show the whole menu then all is
           good... */
        gtk_menu_shell_prepend(GTK_MENU_SHELL(menu),
                               gtk_separator_menu_item_new());
        gtk_widget_show_all(menu);

        is_indicator_set_label(self, _("No Sensors"));
        is_indicator_set_menu(self, GTK_MENU(menu));
#if HAVE_APPINDICATOR
        app_indicator_set_status(APP_INDICATOR(self), APP_INDICATOR_STATUS_ACTIVE);
#endif

        fake_add_enable_sensors(IS_INDICATOR(object));
}

        static void
is_indicator_class_init(IsIndicatorClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

        g_type_class_add_private(klass, sizeof(IsIndicatorPrivate));

        gobject_class->get_property = is_indicator_get_property;
        gobject_class->set_property = is_indicator_set_property;
        gobject_class->constructed = is_indicator_constructed;
        gobject_class->dispose = is_indicator_dispose;
        gobject_class->finalize = is_indicator_finalize;
#if HAVE_APPINDICATOR
        APP_INDICATOR_CLASS(klass)->connection_changed = is_indicator_connection_changed;
#endif

        properties[PROP_APPLICATION] = g_param_spec_object("application", "application property",
                                                           "application property blurp.",
                                                           IS_TYPE_APPLICATION,
                                                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
        g_object_class_install_property(gobject_class, PROP_APPLICATION,
                                        properties[PROP_APPLICATION]);

        properties[PROP_PRIMARY_SENSOR_PATH] = g_param_spec_string("primary-sensor-path",
                                                                   "path of preferred primary sensor",
                                                                   "path of preferred primary sensor",
                                                                   NULL,
                                                                   G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
        g_object_class_install_property(gobject_class, PROP_PRIMARY_SENSOR_PATH,
                                        properties[PROP_PRIMARY_SENSOR_PATH]);

        properties[PROP_DISPLAY_FLAGS] = g_param_spec_int("display-flags",
                                                          "display flags property",
                                                          "display flags property blurp.",
                                                          IS_INDICATOR_DISPLAY_VALUE,
                                                          IS_INDICATOR_DISPLAY_ALL,
                                                          IS_INDICATOR_DISPLAY_ALL,
                                                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
        g_object_class_install_property(gobject_class, PROP_DISPLAY_FLAGS,
                                        properties[PROP_DISPLAY_FLAGS]);

}

        static void
is_indicator_init(IsIndicator *self)
{
        self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_INDICATOR,
                                                 IsIndicatorPrivate);
}

        static void
is_indicator_get_property(GObject *object,
                          guint property_id, GValue *value, GParamSpec *pspec)
{
        IsIndicator *self = IS_INDICATOR(object);

        switch (property_id) {
                case PROP_APPLICATION:
                        g_value_set_object(value, is_indicator_get_application(self));
                        break;
                case PROP_PRIMARY_SENSOR_PATH:
                        g_value_set_string(value, is_indicator_get_primary_sensor_path(self));
                        break;
                case PROP_DISPLAY_FLAGS:
                        g_value_set_int(value, is_indicator_get_display_flags(self));
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
                        break;
        }
}

        static void
is_indicator_set_property(GObject *object,
                          guint property_id, const GValue *value, GParamSpec *pspec)
{
        IsIndicator *self = IS_INDICATOR(object);
        IsIndicatorPrivate *priv = self->priv;
        IsManager *manager;

        switch (property_id) {
                case PROP_APPLICATION:
                        g_assert(!priv->application);
                        priv->application = g_object_ref(g_value_get_object(value));
                        manager = is_application_get_manager(priv->application);
                        g_signal_connect(manager, "sensor-enabled",
                                         G_CALLBACK(sensor_enabled), self);
                        g_signal_connect(manager, "sensor-disabled",
                                         G_CALLBACK(sensor_disabled), self);
                        g_signal_connect(manager, "sensor-added",
                                         G_CALLBACK(sensor_added), self);
                        break;
                case PROP_PRIMARY_SENSOR_PATH:
                        is_indicator_set_primary_sensor_path(self, g_value_get_string(value));
                        break;
                case PROP_DISPLAY_FLAGS:
                        is_indicator_set_display_flags(self, g_value_get_int(value));
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
                        break;
        }
}

#if HAVE_APPINDICATOR
static void is_indicator_connection_changed(AppIndicator *indicator,
                                            gboolean connected,
                                            gpointer data)
{
        IsIndicator *self = IS_INDICATOR(indicator);
        IsIndicatorPrivate *priv = self->priv;
        GtkMenuItem *item;

        if (!priv->primary) {
                goto out;
        }
        /* force an update of the primary sensor to reset icon etc */
        if (!priv->primary) {
                goto out;
        }

        item = (GtkMenuItem *)(g_object_get_data(G_OBJECT(priv->primary),
                                                 "menu-item"));
        if (!item) {
                goto out;
        }
        update_sensor_menu_item_label(self, priv->primary, item);

out:
        return;
}
#endif

        static void
is_indicator_dispose(GObject *object)
{
        IsIndicator *self = (IsIndicator *)object;
        IsIndicatorPrivate *priv = self->priv;
        IsManager *manager;
        GSList *sensors, *_list;

        manager = is_application_get_manager(priv->application);
        g_signal_handlers_disconnect_by_func(manager, sensor_enabled, self);
        g_signal_handlers_disconnect_by_func(manager, sensor_disabled, self);
        g_signal_handlers_disconnect_by_func(manager, sensor_added, self);
        /* fake disabling of any sensors */
        sensors = is_manager_get_enabled_sensors_list(manager);
        for (_list = sensors; _list != NULL; _list = _list->next) {
                IsSensor *sensor = IS_SENSOR(_list->data);
                _sensor_disabled(IS_SENSOR(_list->data), self);
                g_object_unref(sensor);
        }
        g_slist_free(sensors);

#if !HAVE_APPINDICATOR
        g_object_set_data(G_OBJECT(self), "indicator-menu", NULL);
#endif
        G_OBJECT_CLASS(is_indicator_parent_class)->dispose(object);
}

        static void
is_indicator_finalize(GObject *object)
{
        IsIndicator *self = (IsIndicator *)object;
        IsIndicatorPrivate *priv = self->priv;

        g_object_unref(priv->application);

        G_OBJECT_CLASS(is_indicator_parent_class)->finalize(object);
}

#if HAVE_APPINDICATOR
typedef enum {
        VERY_LOW_SENSOR_VALUE = 0,
        LOW_SENSOR_VALUE,
        NORMAL_SENSOR_VALUE,
        HIGH_SENSOR_VALUE,
        VERY_HIGH_SENSOR_VALUE
} SensorValueRange;

/* Cast a given value to a valid SensorValueRange */
#define SENSOR_VALUE_RANGE(x) ((SensorValueRange)(CLAMP(x, VERY_LOW_SENSOR_VALUE, VERY_HIGH_SENSOR_VALUE)))

static gdouble sensor_value_range_normalised(gdouble value,
                                             gdouble low_value,
                                             gdouble high_value) {
        return ((value - low_value)/(high_value - low_value));
}

static SensorValueRange sensor_value_range(gdouble sensor_value,
                                           gdouble low_value,
                                           gdouble high_value) {
        gdouble range;
        range = sensor_value_range_normalised(sensor_value, low_value, high_value)*(gdouble)(VERY_HIGH_SENSOR_VALUE);

        /* check if need to round up, otherwise let int conversion
         * round down for us and make sure it is a valid range
         * value */
        return SENSOR_VALUE_RANGE(((gint)range + ((range - ((gint)range)) >= 0.5)));
}

#define DEFAULT_ICON_SIZE 22

#define NUM_OVERLAY_ICONS 5

static const gchar * const value_overlay_icons[NUM_OVERLAY_ICONS] = {
        "very-low-value-icon",
        "low-value-icon",
        "normal-value-icon",
        "high-value-icon",
        "very-high-value-icon"
};

        static gchar *
icon_cache_dir()
{
        return g_build_path(G_DIR_SEPARATOR_S, g_get_user_cache_dir(),
                            "indicator-sensors", "icons", NULL);
}

        static gboolean
prepare_cache_icon(const gchar * const base_icon_name,
                   const gchar * const overlay_name,
                   const gchar * const icon_path,
                   GError **error)
{
        GdkPixbuf *base_icon, *overlay_icon, *new_icon;
        gchar *icon_dir;
        GtkIconTheme *icon_theme;
        gboolean ret;

        ret = g_file_test(icon_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);

        if (ret) {
                goto out;
        }

        icon_theme = gtk_icon_theme_get_default();
        base_icon = gtk_icon_theme_load_icon(icon_theme,
                                             base_icon_name,
                                             DEFAULT_ICON_SIZE,
                                             DEFAULT_ICON_SIZE,
                                             error);
        if (!base_icon)
        {
                goto out;
        }
        overlay_icon = gtk_icon_theme_load_icon(icon_theme,
                                                overlay_name,
                                                DEFAULT_ICON_SIZE,
                                                DEFAULT_ICON_SIZE,
                                                error);

        if (!overlay_icon) {
                g_object_unref(base_icon);
                goto out;
        }

        new_icon = gdk_pixbuf_copy(base_icon);
        gdk_pixbuf_composite(overlay_icon, new_icon,
                             0, 0,
                             DEFAULT_ICON_SIZE, DEFAULT_ICON_SIZE,
                             0, 0,
                             1.0, 1.0,
                             GDK_INTERP_BILINEAR,
                             255);
        g_object_unref(overlay_icon);

        /* ensure path to icon exists */
        icon_dir = g_path_get_dirname(icon_path);
        g_mkdir_with_parents(icon_dir, 0755);
        g_free(icon_dir);

        /* write out icon */
        ret = gdk_pixbuf_save(new_icon, icon_path, "png", error, NULL);
        g_object_unref(new_icon);
        g_object_unref(base_icon);

out:
        return ret;
}

        static gchar *
sensor_icon_path(IsSensor *sensor)
{
        gdouble value, low, high;
        SensorValueRange value_range;
        const gchar *base_name, *overlay_name;
        gchar *icon_name, *icon_path = NULL, *cache_dir;
        gboolean ret;
        GError *error = NULL;

        base_name = is_sensor_get_icon(sensor);
        value = is_sensor_get_value(sensor);
        low = is_sensor_get_low_value(sensor);
        high = is_sensor_get_high_value(sensor);

        /* no range - return base icon */
        if (fabs(low - high) <= DBL_EPSILON) {
                /* use base_name */
                icon_path = g_strdup(base_name);
                goto out;
        }

        value_range = sensor_value_range(value, low, high);
        overlay_name = value_overlay_icons[value_range];
        cache_dir = icon_cache_dir();
        icon_name = g_strdup_printf("%s-%s", base_name, overlay_name);
        icon_path = g_build_filename(cache_dir, icon_name, NULL);
        g_free(icon_name);
        g_free(cache_dir);

        /* prepare cache icon */
        ret = prepare_cache_icon(base_name, overlay_name, icon_path, &error);
        if (!ret) {
                is_warning("indicator", "Couldn't create cache icon %s from base %s and overlay %s: %s",
                           icon_path, base_name, overlay_name, error->message);
                g_error_free(error);
                /* return base_name instead */
                g_free(icon_path);
                icon_path = g_strdup(base_name);
                goto out;
        }

out:
        return icon_path;
}
#endif

        static void
update_sensor_menu_item_label(IsIndicator *self,
                              IsSensor *sensor,
                              GtkMenuItem *menu_item)
{
        gchar *text;

        text = g_strdup_printf("%s %2.*f%s",
                               is_sensor_get_label(sensor),
                               is_sensor_get_digits(sensor),
                               is_sensor_get_value(sensor),
                               is_sensor_get_units(sensor));
        gtk_menu_item_set_label(menu_item, text);
        g_free(text);
        text = NULL;

#if HAVE_APPINDICATOR
        if (sensor == self->priv->primary) {
                IsIndicatorPrivate *priv = self->priv;
                gboolean connected;

                g_object_get(self, "connected", &connected, NULL);
                /* using fallback so just set icon */
                if (!connected) {
                        app_indicator_set_icon_full(APP_INDICATOR(self), PACKAGE,
                                                    is_sensor_get_label(sensor));
                        return;
                }

                if (priv->display_flags & IS_INDICATOR_DISPLAY_VALUE) {
                        text = g_strdup_printf("%2.*f%s",
                                               is_sensor_get_digits(sensor),
                                               is_sensor_get_value(sensor),
                                               is_sensor_get_units(sensor));
                }
                if (priv->display_flags & IS_INDICATOR_DISPLAY_LABEL) {
                        /* join label to existing text - if text is NULL this
                           will just show label */
                        text = g_strjoin(" ",
                                         is_sensor_get_label(sensor),
                                         text, NULL);
                }
                if (priv->display_flags & IS_INDICATOR_DISPLAY_ICON) {
                        gchar *icon_path = sensor_icon_path(sensor);
                        app_indicator_set_icon_full(APP_INDICATOR(self), icon_path,
                                                    is_sensor_get_label(sensor));
                        g_free(icon_path);
                } else {
                        /* set the empty string to show no icon */
                        app_indicator_set_icon_full(APP_INDICATOR(self), "",
                                                    is_sensor_get_label(sensor));

                }
                app_indicator_set_label(APP_INDICATOR(self), text, text);
                g_free(text);
                app_indicator_set_status(APP_INDICATOR(self),
                                         is_sensor_get_alarmed(sensor) ?
                                         APP_INDICATOR_STATUS_ATTENTION :
                                         APP_INDICATOR_STATUS_ACTIVE);
        }
#else
        gtk_status_icon_set_from_icon_name(GTK_STATUS_ICON(self), PACKAGE);
#endif

}

        static void
sensor_notify(IsSensor *sensor,
              GParamSpec *psec,
              IsIndicator *self)
{
        GtkMenuItem *menu_item;

        g_return_if_fail(IS_IS_SENSOR(sensor));
        g_return_if_fail(IS_IS_INDICATOR(self));

        menu_item = GTK_MENU_ITEM(g_object_get_data(G_OBJECT(sensor),
                                                    "menu-item"));
        if (menu_item) {
                update_sensor_menu_item_label(self, sensor, menu_item);
        }
}

        static void
sensor_error(IsSensor *sensor, GError *error, IsIndicator *self)
{
        is_warning("indicator", "sensor %s error: %s",
                   is_sensor_get_path(sensor),
                   error->message);
}

        static void
_sensor_disabled(IsSensor *sensor,
                 IsIndicator *self)
{
        IsIndicatorPrivate *priv = self->priv;
        GtkWidget *menu_item;

        is_debug("indicator", "disabling sensor %s",
                 is_sensor_get_path(sensor));

        /* destroy menu item */
        menu_item = GTK_WIDGET(g_object_get_data(G_OBJECT(sensor),
                                                 "menu-item"));
        priv->menu_items = g_slist_remove(priv->menu_items, menu_item);
        gtk_container_remove(GTK_CONTAINER(is_indicator_get_menu(self)),
                             menu_item);
        g_object_set_data(G_OBJECT(sensor), "menu-item", NULL);

        g_signal_handlers_disconnect_by_func(sensor,
                                             sensor_notify,
                                             self);
        g_signal_handlers_disconnect_by_func(sensor,
                                             sensor_error,
                                             self);
        /* ignore if was not primary sensor, otherwise, get a new one */
        if (sensor != priv->primary) {
                goto out;
        }
        if (!priv->menu_items) {
                is_indicator_set_label(self, _("No active sensors"));
#if !HAVE_APPINDICATOR
                gtk_status_icon_set_from_stock(GTK_STATUS_ICON(self),
                                               GTK_STOCK_DIALOG_WARNING);
#endif
                g_object_unref(priv->primary);
                priv->primary = NULL;
        }
out:
        return;
}

        static void
sensor_disabled(IsManager *manager,
                IsSensor *sensor,
                IsIndicator *self)
{
        IsIndicatorPrivate *priv;
        GtkWidget *menu_item;

        priv = self->priv;

        _sensor_disabled(sensor, self);

        /* choose top-most menu item and set it as primary one */
        menu_item = GTK_WIDGET(priv->menu_items->data);
        /* activate it to make this the new primary sensor */
        gtk_menu_item_activate(GTK_MENU_ITEM(menu_item));
}

        static void
sensor_menu_item_toggled(GtkMenuItem *menu_item,
                         IsIndicator *self)
{
        IsSensor *sensor;
        gboolean active = FALSE;

        g_return_if_fail(IS_IS_INDICATOR(self));

        /* only set as primary if was toggled to active state since we may have
         * untoggled it instead */
        active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
        if (active) {
                sensor = IS_SENSOR(g_object_get_data(G_OBJECT(menu_item),
                                                     "sensor"));
                is_debug("indicator", "Sensor %s menu-item toggled to active - setting as primary sensor",
                         is_sensor_get_path(sensor));
                is_indicator_set_primary_sensor_path(self, is_sensor_get_path(sensor));
        }
}

        static void
sensor_enabled(IsManager *manager,
               IsSensor *sensor,
               gint position,
               IsIndicator *self)
{
        IsIndicatorPrivate *priv = self->priv;

        /* make sure we haven't seen this sensor before - if sensor has a
         * menu-item then ignore it */
        if (!g_object_get_data(G_OBJECT(sensor), "menu-item")) {
                GtkMenu *menu;
                GtkWidget *menu_item;

                is_debug("indicator", "Creating menu item for newly enabled sensor %s",
                         is_sensor_get_path(sensor));

                g_signal_connect(sensor, "notify::value",
                                 G_CALLBACK(sensor_notify),
                                 self);
                g_signal_connect(sensor, "notify::label",
                                 G_CALLBACK(sensor_notify),
                                 self);
                g_signal_connect(sensor, "notify::alarmed",
                                 G_CALLBACK(sensor_notify),
                                 self);
                g_signal_connect(sensor, "notify::low-value",
                                 G_CALLBACK(sensor_notify),
                                 self);
                g_signal_connect(sensor, "notify::high-value",
                                 G_CALLBACK(sensor_notify),
                                 self);
                g_signal_connect(sensor, "error",
                                 G_CALLBACK(sensor_error), self);
                /* add a menu entry for this sensor */
                menu = is_indicator_get_menu(self);
                menu_item = gtk_check_menu_item_new();
                gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(menu_item),
                                                      TRUE);
                g_object_set_data(G_OBJECT(sensor), "menu-item", menu_item);
                g_object_set_data(G_OBJECT(menu_item), "sensor", sensor);

                priv->menu_items = g_slist_insert(priv->menu_items, menu_item,
                                                  position);
                /* if we haven't seen our primary sensor yet or if this is the
                 * primary sensor, display this as primary anyway */
                if (!priv->primary ||
                    g_strcmp0(is_sensor_get_path(sensor),
                              priv->primary_sensor_path) == 0) {
                        is_debug("indicator", "Using sensor with path %s as primary",
                                 is_sensor_get_path(sensor));
                        if (priv->primary) {
                                GtkCheckMenuItem *item;
                                /* uncheck menu item if exists for this
                                 * existing primary sensor */
                                item = (GtkCheckMenuItem *)(g_object_get_data(G_OBJECT(priv->primary),
                                                                              "menu-item"));
                                if (item) {
                                        is_debug("indicator", "Unchecking current primary sensor item");
                                        gtk_check_menu_item_set_active(item, FALSE);
                                }
                                g_object_unref(priv->primary);
                        }
                        priv->primary = g_object_ref(sensor);
                        is_debug("indicator", "Checking new primary sensor item");
                        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
                                                       TRUE);
                        update_sensor_menu_item_label(self, sensor,
                                                      GTK_MENU_ITEM(menu_item));
                }
                /* connect to toggled signal now - if we connect to it earlier
                 * we may interpret the above menu_item_set_active as a user
                 * initiated setting of the primary sensor rather than us just
                 * picking the first available sensor */
                g_signal_connect(menu_item, "toggled",
                                 G_CALLBACK(sensor_menu_item_toggled),
                                 self);
                gtk_widget_show_all(menu_item);

                gtk_menu_shell_insert(GTK_MENU_SHELL(menu), menu_item, position);
                update_sensor_menu_item_label(self, sensor, GTK_MENU_ITEM(menu_item));
        } else {
                is_debug("indicator", "Newly enabled sensor %s already has a menu-item, ignoring...",
                         is_sensor_get_path(sensor));
        }
}

        static void
sensor_added(IsManager *manager,
             IsSensor *sensor,
             IsIndicator *self)
{
        /* if a sensor has been added and we haven't yet got any enabled sensors
           to display (and hence no primary sensor), change our text to show
           this */
        if (!self->priv->menu_items) {
                is_indicator_set_label(self, _("No active sensors"));
#if !HAVE_APPINDICATOR
                gtk_status_icon_set_from_stock(GTK_STATUS_ICON(self),
                                               GTK_STOCK_DIALOG_WARNING);
#endif
        }
}

        IsIndicator *
is_indicator_new(IsApplication *application)
{
        IsIndicator *self = g_object_new(IS_TYPE_INDICATOR,
#if HAVE_APPINDICATOR
                                         "id", PACKAGE,
                                         "category", "Hardware",
#endif
                                         "application", application,
                                         "icon-name", PACKAGE,
                                         "title", PACKAGE_NAME,
                                         NULL);
        return IS_INDICATOR(self);
}

void is_indicator_set_primary_sensor_path(IsIndicator *self,
                                          const gchar *path)
{
        IsIndicatorPrivate *priv;

        g_return_if_fail(IS_IS_INDICATOR(self));

        priv = self->priv;

        if (g_strcmp0(priv->primary_sensor_path, path) != 0 &&
            g_strcmp0(path, "") != 0) {
                IsSensor *sensor;

                is_debug("indicator", "new primary sensor path %s (previously %s)",
                         path, priv->primary_sensor_path);

                /* uncheck current primary sensor label - may be NULL as is
                 * already disabled */
                if (priv->primary) {
                        GtkCheckMenuItem *item;
                        item = (GtkCheckMenuItem *)(g_object_get_data(G_OBJECT(priv->primary),
                                                                      "menu-item"));
                        if (item) {
                                gtk_check_menu_item_set_active(item, FALSE);
                        }
                        g_object_unref(priv->primary);
                }

                g_free(priv->primary_sensor_path);
                priv->primary_sensor_path = g_strdup(path);

                is_debug("indicator", "Setting primary sensor path to: %s", path);

                /* try and activate this sensor if it exists */
                sensor = is_manager_get_sensor(is_application_get_manager(priv->application),
                                               priv->primary_sensor_path);
                if (sensor) {
                        GtkCheckMenuItem *item = (GtkCheckMenuItem *)(g_object_get_data(G_OBJECT(sensor),
                                                                                        "menu-item"));
                        /* take reference from manager */
                        priv->primary = sensor;
                        if (item) {
                                gtk_check_menu_item_set_active(item, TRUE);
                                update_sensor_menu_item_label(self, sensor,
                                                              GTK_MENU_ITEM(item));
                        }
                }

                g_object_notify_by_pspec(G_OBJECT(self),
                                         properties[PROP_PRIMARY_SENSOR_PATH]);
        }
}

const gchar *is_indicator_get_primary_sensor_path(IsIndicator *self)
{
        g_return_val_if_fail(IS_IS_INDICATOR(self), NULL);

        return self->priv->primary_sensor_path;
}

IsApplication *is_indicator_get_application(IsIndicator *self)
{
        g_return_val_if_fail(IS_IS_INDICATOR(self), NULL);

        return self->priv->application;
}

void is_indicator_set_display_flags(IsIndicator *self,
                                    IsIndicatorDisplayFlags display_flags)
{
        IsIndicatorPrivate *priv;

        g_return_if_fail(IS_IS_INDICATOR(self));

        priv = self->priv;

        if (display_flags != priv->display_flags) {
                priv->display_flags = display_flags;
                g_object_notify_by_pspec(G_OBJECT(self),
                                         properties[PROP_DISPLAY_FLAGS]);

                if (priv->primary) {
                        GtkMenuItem *item = GTK_MENU_ITEM(g_object_get_data(G_OBJECT(priv->primary),
                                                                            "menu-item"));
                        if (item) {
                                update_sensor_menu_item_label(self, priv->primary, item);
                        }
                }
        }
}

        IsIndicatorDisplayFlags
is_indicator_get_display_flags(IsIndicator *self)
{
        g_return_val_if_fail(IS_IS_INDICATOR(self), 0);

        return self->priv->display_flags;
}
