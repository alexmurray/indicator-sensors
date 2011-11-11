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
#include "is-manager.h"
#include "is-preferences-dialog.h"
#include "is-sensor-dialog.h"
#include "is-log.h"
#include <math.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (IsIndicator, is_indicator, APP_INDICATOR_TYPE);

static void is_indicator_dispose(GObject *object);
static void is_indicator_finalize(GObject *object);
static void is_indicator_get_property(GObject *object,
				      guint property_id, GValue *value, GParamSpec *pspec);
static void is_indicator_set_property(GObject *object,
				      guint property_id, const GValue *value, GParamSpec *pspec);
static void is_indicator_connection_changed(AppIndicator *indicator,
					    gboolean connected,
					    gpointer data);
static void sensor_enabled(IsManager *manager,
			   IsSensor *sensor,
			   gint position,
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
	PROP_MANAGER = 1,
	PROP_PRIMARY_SENSOR,
	PROP_DISPLAY_MODE,
	LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = {NULL};

struct _IsIndicatorPrivate
{
	IsManager *manager;
	gchar *primary_sensor;
	IsIndicatorDisplayMode display_mode;
	GSList *menu_items;
	GtkWidget *prefs_dialog;
};

static IsIndicator *default_indicator = NULL;

static void
is_indicator_class_init(IsIndicatorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	AppIndicatorClass *indicator_class = APP_INDICATOR_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsIndicatorPrivate));

	gobject_class->get_property = is_indicator_get_property;
	gobject_class->set_property = is_indicator_set_property;
	gobject_class->dispose = is_indicator_dispose;
	gobject_class->finalize = is_indicator_finalize;
	indicator_class->connection_changed = is_indicator_connection_changed;

	properties[PROP_MANAGER] = g_param_spec_object("manager", "manager property",
						       "manager property blurp.",
						       IS_TYPE_MANAGER,
						       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_MANAGER,
					properties[PROP_MANAGER]);

	properties[PROP_PRIMARY_SENSOR] = g_param_spec_string("primary-sensor", "primary sensor property",
							      "primary sensor property blurp.",
							      NULL,
							      G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_PRIMARY_SENSOR,
					properties[PROP_PRIMARY_SENSOR]);

	properties[PROP_DISPLAY_MODE] = g_param_spec_int("display-mode",
							 "display mode property",
							 "display mode property blurp.",
							 IS_INDICATOR_DISPLAY_MODE_VALUE_ONLY,
							 NUM_IS_INDICATOR_DISPLAY_MODES,
							 IS_INDICATOR_DISPLAY_MODE_LABEL_AND_VALUE,
							 G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_DISPLAY_MODE,
					properties[PROP_DISPLAY_MODE]);

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
	case PROP_MANAGER:
		g_value_set_object(value, is_indicator_get_manager(self));
		break;
	case PROP_PRIMARY_SENSOR:
		g_value_set_string(value, is_indicator_get_primary_sensor(self));
		break;
	case PROP_DISPLAY_MODE:
		g_value_set_int(value, is_indicator_get_display_mode(self));
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

	switch (property_id) {
	case PROP_MANAGER:
		g_assert(!priv->manager);
		priv->manager = g_object_ref(g_value_get_object(value));
		g_signal_connect(priv->manager, "sensor-enabled",
				 G_CALLBACK(sensor_enabled), self);
		g_signal_connect(priv->manager, "sensor-disabled",
				 G_CALLBACK(sensor_disabled), self);
		g_signal_connect(priv->manager, "sensor-added",
				 G_CALLBACK(sensor_added), self);
		break;
	case PROP_PRIMARY_SENSOR:
		is_indicator_set_primary_sensor(self, g_value_get_string(value));
		break;
	case PROP_DISPLAY_MODE:
		is_indicator_set_display_mode(self, g_value_get_int(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void is_indicator_connection_changed(AppIndicator *indicator,
					    gboolean connected,
					    gpointer data)
{
	IsIndicator *self = IS_INDICATOR(indicator);
	IsIndicatorPrivate *priv = self->priv;
	IsSensor *sensor;
	GtkMenuItem *item;

	if (!priv->primary_sensor) {
		goto out;
	}
	/* force an update of the primary sensor to reset icon etc */
	sensor = is_manager_get_sensor(priv->manager,
				       priv->primary_sensor);
	if (!sensor) {
		goto out;
	}

	item = (GtkMenuItem *)(g_object_get_data(G_OBJECT(sensor),
						 "menu-item"));
	if (!item) {
		g_object_unref(sensor);
		goto out;
	}
	update_sensor_menu_item_label(self, sensor, item);
	g_object_unref(sensor);

out:
	return;
}

static void
is_indicator_dispose(GObject *object)
{
	IsIndicator *self = (IsIndicator *)object;
	IsIndicatorPrivate *priv = self->priv;

	if (priv->prefs_dialog) {
		gtk_widget_destroy(priv->prefs_dialog);
		priv->prefs_dialog = NULL;
	}
	G_OBJECT_CLASS(is_indicator_parent_class)->dispose(object);
}

static void
is_indicator_finalize(GObject *object)
{
	IsIndicator *self = (IsIndicator *)object;
	IsIndicatorPrivate *priv = self->priv;

	g_object_unref(priv->manager);

	G_OBJECT_CLASS(is_indicator_parent_class)->finalize(object);
}

static void prefs_action(GtkAction *action,
			 IsIndicator *self);
static void about_action(GtkAction *action,
			 IsIndicator *self);
static void quit_action(GtkAction *action,
			IsIndicator *self);

static GtkActionEntry entries[] = {
	{ "Preferences", "application-preferences", N_("Preferences…"), NULL,
	  N_("Preferences…"), G_CALLBACK(prefs_action) },
	{ "About", "about", N_("About…"), NULL,
	  N_("About…"), G_CALLBACK(about_action) },
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

static void
prefs_dialog_response(IsPreferencesDialog *dialog,
		      gint response_id,
		      IsIndicator *self)
{
	IsIndicatorPrivate *priv;

	priv = self->priv;

	if (response_id == IS_PREFERENCES_DIALOG_RESPONSE_SENSOR_PROPERTIES) {
		IsSensor *sensor = is_manager_get_selected_sensor(priv->manager);
		if (sensor) {
			GtkWidget *sensor_dialog = is_sensor_dialog_new(sensor);
			g_signal_connect(sensor_dialog, "response",
					 G_CALLBACK(gtk_widget_destroy), NULL);
			g_signal_connect(sensor_dialog, "delete-event",
					 G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_widget_show_all(sensor_dialog);
			g_object_unref(sensor);
		}
	} else {
		gtk_widget_hide(GTK_WIDGET(dialog));
	}
}

static void prefs_action(GtkAction *action,
			 IsIndicator *self)
{
	IsIndicatorPrivate *priv;

	priv = self->priv;

	if (!priv->prefs_dialog) {
		priv->prefs_dialog = is_preferences_dialog_new(self);
		g_signal_connect(priv->prefs_dialog, "response",
				 G_CALLBACK(prefs_dialog_response), self);
		g_signal_connect(priv->prefs_dialog, "delete-event",
				 G_CALLBACK(gtk_widget_hide_on_delete),
				 NULL);
		gtk_widget_show_all(priv->prefs_dialog);
	}
	gtk_window_present(GTK_WINDOW(priv->prefs_dialog));
}

static void about_action(GtkAction *action,
			 IsIndicator *self)
{
	const gchar * const authors[] =
	{
		"Alex Murray <murray.alex@gmail.com>",
		NULL
	};

	gtk_show_about_dialog(NULL,
			      "program-name", _("Hardware Sensors Indicator"),
			      "authors", authors,
			      "comments", _("Application Indicator for Unity showing hardware sensors."),
			      "copyright", "Copyright © 2011 Alex Murray",
			      "logo-icon-name", "indicator-sensors",
			      "version", VERSION,
			      "website", "https://launchpad.net/indicator-sensors",
			      NULL);
}

static void quit_action(GtkAction *action,
			IsIndicator *self)
{
	gtk_main_quit();
}


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

static void
update_sensor_menu_item_label(IsIndicator *self,
			      IsSensor *sensor,
			      GtkMenuItem *menu_item)
{
	IsIndicatorPrivate *priv = self->priv;
	gchar *text;

	text = g_strdup_printf("%s %2.*f%s",
				      is_sensor_get_label(sensor),
				      is_sensor_get_digits(sensor),
				      is_sensor_get_value(sensor),
				      is_sensor_get_units(sensor));
	gtk_menu_item_set_label(menu_item, text);

	if (g_strcmp0(priv->primary_sensor, is_sensor_get_path(sensor)) == 0) {
		gboolean connected;
		gchar *icon_path;

		g_object_get(self, "connected", &connected, NULL);

		if (!connected) {
			app_indicator_set_icon_full(APP_INDICATOR(self), PACKAGE,
						    is_sensor_get_label(sensor));
			goto out;
		}

		icon_path = sensor_icon_path(sensor);
		app_indicator_set_icon_full(APP_INDICATOR(self), icon_path,
					    is_sensor_get_label(sensor));
		g_free(icon_path);

		/* set display based on current display_mode */
		switch (priv->display_mode) {
		case IS_INDICATOR_DISPLAY_MODE_VALUE_ONLY:
			app_indicator_set_label(APP_INDICATOR(self),
						text + strlen(is_sensor_get_label(sensor)) + 1,
						text + strlen(is_sensor_get_label(sensor)) + 1);
			break;
		case IS_INDICATOR_DISPLAY_MODE_LABEL_AND_VALUE:
			app_indicator_set_label(APP_INDICATOR(self),
						text, text);
			break;
		case IS_INDICATOR_DISPLAY_MODE_INVALID:
		case NUM_IS_INDICATOR_DISPLAY_MODES:
		default:
			g_assert_not_reached();
		}
		app_indicator_set_status(APP_INDICATOR(self),
					 is_sensor_get_alarmed(sensor) ?
					 APP_INDICATOR_STATUS_ATTENTION :
					 APP_INDICATOR_STATUS_ACTIVE);
	}
out:
	g_free(text);
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
	update_sensor_menu_item_label(self, sensor, menu_item);
}

static void
sensor_error(IsSensor *sensor, GError *error, IsIndicator *self)
{
	is_warning("indicator", "sensor %s error: %s",
		  is_sensor_get_path(sensor),
		  error->message);
}

static void
sensor_disabled(IsManager *manager,
		IsSensor *sensor,
		IsIndicator *self)
{
	IsIndicatorPrivate *priv = self->priv;
	GtkWidget *menu_item;

	/* debug - enable sensor */
	is_debug("main", "disabling sensor %s",
		is_sensor_get_path(sensor));

	/* destroy menu item */
	menu_item = GTK_WIDGET(g_object_get_data(G_OBJECT(sensor),
						 "menu-item"));
	priv->menu_items = g_slist_remove(priv->menu_items, menu_item);
	gtk_container_remove(GTK_CONTAINER(app_indicator_get_menu(APP_INDICATOR(self))),
			     menu_item);
	g_object_set_data(G_OBJECT(sensor), "menu-item", NULL);
	g_object_set_data(G_OBJECT(sensor), "value-item", NULL);

	g_signal_handlers_disconnect_by_func(sensor,
					     sensor_notify,
					     self);
	g_signal_handlers_disconnect_by_func(sensor,
					     sensor_error,
					     self);
	/* ignore if was not primary sensor, otherwise, get a new one */
	if (g_strcmp0(priv->primary_sensor, is_sensor_get_path(sensor)) != 0) {
		goto out;
	}
	if (!priv->menu_items) {
		app_indicator_set_label(APP_INDICATOR(self),
					_("No active sensors"),
					_("No active sensors"));
		g_free(priv->primary_sensor);
		priv->primary_sensor = NULL;
		goto out;
	}
	/* choose top-most menu item */
	menu_item = GTK_WIDGET(priv->menu_items->data);
	/* activate it to make this the new primary sensor */
	gtk_menu_item_activate(GTK_MENU_ITEM(menu_item));

out:
	return;
}

static void
sensor_menu_item_activated(GtkMenuItem *menu_item,
			   IsIndicator *self)
{
	IsSensor *sensor;

	g_return_if_fail(IS_IS_INDICATOR(self));

	sensor = IS_SENSOR(g_object_get_data(G_OBJECT(menu_item),
					     "sensor"));
	is_indicator_set_primary_sensor(self, is_sensor_get_path(sensor));
}

static void
sensor_enabled(IsManager *manager,
	       IsSensor *sensor,
	       gint position,
	       IsIndicator *self)
{
	IsIndicatorPrivate *priv = self->priv;
	GtkMenu *menu;
	GtkWidget *menu_item;

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
	menu = app_indicator_get_menu(APP_INDICATOR(self));
	menu_item = gtk_check_menu_item_new();
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(menu_item),
					      TRUE);
	g_signal_connect(menu_item, "activate",
			 G_CALLBACK(sensor_menu_item_activated),
			 self);
	g_object_set_data(G_OBJECT(sensor), "menu-item", menu_item);
	g_object_set_data(G_OBJECT(menu_item), "sensor", sensor);

	priv->menu_items = g_slist_insert(priv->menu_items, menu_item,
					  position);
	/* if we don't have a primary sensor, display this one by default,
	 * otherwise wait till we see our preferred primary sensor */
	if (!priv->primary_sensor) {
		is_indicator_set_primary_sensor(self, is_sensor_get_path(sensor));
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
					       TRUE);
	}
	gtk_widget_show_all(menu_item);

	gtk_menu_shell_insert(GTK_MENU_SHELL(menu), menu_item, position);
	update_sensor_menu_item_label(self, sensor, GTK_MENU_ITEM(menu_item));
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
		app_indicator_set_label(APP_INDICATOR(self),
					_("No active sensors"),
					_("No active sensors"));
	}
}

static IsIndicator *
is_indicator_new(IsManager *manager)
{
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GError *error = NULL;
	GtkWidget *menu;

	AppIndicator *self = g_object_new(IS_TYPE_INDICATOR,
					  "id", PACKAGE,
					  "icon-name", PACKAGE,
					  "category", "Hardware",
					  "manager", manager,
					  NULL);

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

	app_indicator_set_label(self, _("No Sensors"), _("No Sensors"));
	app_indicator_set_menu(self, GTK_MENU(menu));
	app_indicator_set_status(self, APP_INDICATOR_STATUS_ACTIVE);

	return IS_INDICATOR(self);
}

IsIndicator *
is_indicator_get_default(void)
{
	if (!default_indicator) {
		default_indicator = is_indicator_new(is_manager_new());
		g_object_add_weak_pointer(G_OBJECT(default_indicator),
					  (gpointer *)&default_indicator);
	}
	g_assert(default_indicator);
	return g_object_ref(default_indicator);
}

void is_indicator_set_primary_sensor(IsIndicator *self,
				     const gchar *primary_sensor)
{
	IsIndicatorPrivate *priv;

	g_return_if_fail(IS_IS_INDICATOR(self));

	priv = self->priv;

	if (g_strcmp0(priv->primary_sensor, primary_sensor) != 0 &&
	    g_strcmp0(primary_sensor, "") != 0) {
		IsSensor *sensor;
		/* uncheck current primary sensor label - may be NULL as is
		* already disabled */
		if (priv->primary_sensor) {
			sensor = is_manager_get_sensor(priv->manager,
								 priv->primary_sensor);
			if (sensor) {
				GtkCheckMenuItem *item;
				item = (GtkCheckMenuItem *)(g_object_get_data(G_OBJECT(sensor),
									      "menu-item"));
				if (item) {
					gtk_check_menu_item_set_active(item, FALSE);
				}
				g_object_unref(sensor);
			}
		}


		g_free(priv->primary_sensor);
		priv->primary_sensor = g_strdup(primary_sensor);

		is_debug("indicator", "displaying primary sensor %s", priv->primary_sensor);

		/* try and activate this sensor if it exists */
		sensor = is_manager_get_sensor(priv->manager,
					       priv->primary_sensor);
		if (sensor) {
			GtkCheckMenuItem *item = (GtkCheckMenuItem *)(g_object_get_data(G_OBJECT(sensor),
											"menu-item"));
			if (item) {
				gtk_check_menu_item_set_active(item, TRUE);
				update_sensor_menu_item_label(self, sensor,
							      GTK_MENU_ITEM(item));
			}
			g_object_unref(sensor);
		}

		g_object_notify_by_pspec(G_OBJECT(self),
					 properties[PROP_PRIMARY_SENSOR]);
	}
}

const gchar *is_indicator_get_primary_sensor(IsIndicator *self)
{
	g_return_val_if_fail(IS_IS_INDICATOR(self), NULL);

	return self->priv->primary_sensor;
}

IsManager *is_indicator_get_manager(IsIndicator *self)
{
	g_return_val_if_fail(IS_IS_INDICATOR(self), NULL);

	return self->priv->manager;
}

void is_indicator_set_display_mode(IsIndicator *self,
				   IsIndicatorDisplayMode display_mode)
{
	IsIndicatorPrivate *priv;

	g_return_if_fail(IS_IS_INDICATOR(self));
	g_return_if_fail(display_mode > IS_INDICATOR_DISPLAY_MODE_INVALID &&
			 display_mode < NUM_IS_INDICATOR_DISPLAY_MODES);

	priv = self->priv;

	if (display_mode != priv->display_mode) {
		IsSensor *sensor;
		GtkMenuItem *item;

		priv->display_mode = display_mode;

		g_object_notify_by_pspec(G_OBJECT(self),
					 properties[PROP_DISPLAY_MODE]);

		if (priv->primary_sensor) {
			/* redisplay primary sensor */
			sensor = is_manager_get_sensor(priv->manager,
						       priv->primary_sensor);
			if(sensor) {
				item = GTK_MENU_ITEM(g_object_get_data(G_OBJECT(sensor),
								       "menu-item"));
				update_sensor_menu_item_label(self, sensor, item);
				g_object_unref(sensor);
			}
		}
	}
}

IsIndicatorDisplayMode
is_indicator_get_display_mode(IsIndicator *self)
{
	g_return_val_if_fail(IS_IS_INDICATOR(self),
			     IS_INDICATOR_DISPLAY_MODE_INVALID);

	return self->priv->display_mode;
}
