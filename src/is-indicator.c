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

#include "is-indicator.h"
#include "is-manager.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE (IsIndicator, is_indicator, APP_INDICATOR_TYPE);

static void is_indicator_dispose(GObject *object);
static void is_indicator_finalize(GObject *object);
static void is_indicator_get_property(GObject *object,
				      guint property_id, GValue *value, GParamSpec *pspec);
static void is_indicator_set_property(GObject *object,
				      guint property_id, const GValue *value, GParamSpec *pspec);

/* signal enum */
enum {
	SIGNAL_DUMMY,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

/* properties */
enum {
	PROP_MANAGER = 1,
	LAST_PROPERTY
};

struct _IsIndicatorPrivate
{
	IsManager *manager;
	GSList *menu_items;
	IsSensor *primary_sensor;
};

static void
is_indicator_class_init(IsIndicatorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsIndicatorPrivate));

	gobject_class->get_property = is_indicator_get_property;
	gobject_class->set_property = is_indicator_set_property;
	gobject_class->dispose = is_indicator_dispose;
	gobject_class->finalize = is_indicator_finalize;

	g_object_class_install_property(gobject_class, PROP_MANAGER,
					g_param_spec_object("manager", "manager property",
							    "manager property blurp.",
							    IS_TYPE_MANAGER,
							    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	signals[SIGNAL_DUMMY] = g_signal_new("dummy",
					     G_OBJECT_CLASS_TYPE(klass),
					     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
					     0,
					     NULL, NULL,
					     g_cclosure_marshal_VOID__VOID,
					     G_TYPE_NONE, 0);

}

static void
is_indicator_init(IsIndicator *self)
{
	IsIndicatorPrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_INDICATOR,
					    IsIndicatorPrivate);

	self->priv = priv;
}

static void
is_indicator_get_property(GObject *object,
			  guint property_id, GValue *value, GParamSpec *pspec)
{
	IsIndicator *self = IS_INDICATOR(object);
	IsIndicatorPrivate *priv = self->priv;

	/* Make compiler happy */
	(void)priv;

	switch (property_id) {
	case PROP_MANAGER:
		g_value_set_object(value, priv->manager);
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
		priv->manager = g_object_ref(g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}


static void
is_indicator_dispose(GObject *object)
{
	IsIndicator *self = (IsIndicator *)object;
	IsIndicatorPrivate *priv = self->priv;

	(void)priv;

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

static void activate_action(GtkAction *action,
			    IsIndicator *self);

static GtkActionEntry entries[] = {
	{ "Preferences", "application-preferences", N_("_Preferences"), NULL,
	  N_("Preferences"), G_CALLBACK(activate_action) },
};
static guint n_entries = G_N_ELEMENTS(entries);

static const gchar *ui_info =
"<ui>"
"  <popup name='Indicator'>"
"    <menuitem action='Preferences' />"
"  </popup>"
"</ui>";

static void activate_action(GtkAction *action,
			    IsIndicator *self)
{
	IsIndicatorPrivate *priv;
	GtkWidget *dialog;
	GtkWidget *scrolled_window;

	priv = self->priv;
	g_debug("activated action %s", gtk_action_get_name(action));

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window),
			  GTK_WIDGET(priv->manager));
	dialog = gtk_dialog_new_with_buttons(_("Preferences"),
					     NULL,
					     0,
					     GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					     NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 500);

	g_signal_connect_swapped(dialog, "response",
				 G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
			  scrolled_window);
	gtk_widget_show_all(dialog);
}

IsIndicator *
is_indicator_new(const gchar *id,
		 const gchar *icon_name,
		 IsManager *manager)
{
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GError *error = NULL;
	GtkWidget *menu;

	AppIndicator *self = g_object_new(IS_TYPE_INDICATOR,
					  "id", id,
					  "icon-name", icon_name,
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
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      gtk_separator_menu_item_new());
	app_indicator_set_status(self, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_attention_icon(self, "sensors-indicator");
	/* TODO: translate me */
	app_indicator_set_label(self, _("No sensors"), _("No Sensors"));
	app_indicator_set_menu(self, GTK_MENU(menu));

	return IS_INDICATOR(self);
}

static void
update_sensor_menu_item_label(IsIndicator *self,
			      IsSensor *sensor,
			      GtkMenuItem *menu_item)
{
	gchar *text = g_strdup_printf("%s: %2.1f%s",
				      is_sensor_get_label(sensor),
				      is_sensor_get_value(sensor),
				      is_sensor_get_units(sensor));
	gtk_menu_item_set_label(menu_item, text);
	if (self->priv->primary_sensor == sensor) {
		app_indicator_set_label(APP_INDICATOR(self),
					text, text);
	}
	g_free(text);
}

static void
sensor_label_or_value_notify(IsSensor *sensor,
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
	g_warning("sensor [%s]:%s error: %s",
		  is_sensor_get_family(sensor),
		  is_sensor_get_id(sensor),
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
	g_debug("disabling sensor [%s]:%s",
		is_sensor_get_family(sensor),
		is_sensor_get_id(sensor));

	/* destroy menu item */
	menu_item = GTK_WIDGET(g_object_get_data(G_OBJECT(sensor),
						 "menu-item"));
	gtk_container_remove(GTK_CONTAINER(app_indicator_get_menu(APP_INDICATOR(self))),
			     menu_item);
	g_object_set_data(G_OBJECT(sensor), "menu-item", NULL);
	g_object_set_data(G_OBJECT(sensor), "value-item", NULL);

	g_signal_handlers_disconnect_by_func(sensor,
					     sensor_label_or_value_notify,
					     self);
	g_signal_handlers_disconnect_by_func(sensor,
					     sensor_error,
					     self);
}

static void
sensor_menu_item_activated(GtkMenuItem *menu_item,
			   IsIndicator *self)
{
	IsIndicatorPrivate *priv;
	IsSensor *sensor;

	g_return_if_fail(IS_IS_INDICATOR(self));

	priv = self->priv;
	sensor = IS_SENSOR(g_object_get_data(G_OBJECT(menu_item),
					     "sensor"));
	/* enable display of this sensor if not displaying it */
	if (priv->primary_sensor != sensor) {
		g_debug("displaying sensor [%s]:%s",
			is_sensor_get_family(sensor),
			is_sensor_get_id(sensor));
		priv->primary_sensor = sensor;
		update_sensor_menu_item_label(self, sensor, menu_item);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
					       TRUE);
	}
}

static void
sensor_enabled(IsManager *manager,
	       IsSensor *sensor,
	       IsIndicator *self)
{
	IsIndicatorPrivate *priv = self->priv;
	GtkMenu *menu;
	GtkWidget *menu_item;
	GtkWidget *hbox;
	GtkWidget *label;

	g_signal_connect(sensor, "notify::value",
			 G_CALLBACK(sensor_label_or_value_notify),
			 self);
	g_signal_connect(sensor, "notify::label",
			 G_CALLBACK(sensor_label_or_value_notify),
			 self);
	g_signal_connect(sensor, "error",
			 G_CALLBACK(sensor_error), self);
	/* add a menu entry for this sensor */
	menu = app_indicator_get_menu(APP_INDICATOR(self));
	/* if this is the first sensor we are displaying, then show it by
	   default as the main sensor */
	if (!priv->menu_items) {
		priv->primary_sensor = sensor;
	}
	menu_item = gtk_radio_menu_item_new(priv->menu_items);
	priv->menu_items = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
	if (priv->primary_sensor == sensor) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
					       TRUE);
	}
	g_signal_connect(menu_item, "activate",
			 G_CALLBACK(sensor_menu_item_activated),
			 self);
	gtk_widget_show_all(menu_item);

	g_object_set_data(G_OBJECT(sensor), "menu-item", menu_item);
	g_object_set_data(G_OBJECT(menu_item), "sensor", sensor);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
}

