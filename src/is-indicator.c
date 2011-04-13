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
#include <gtk/gtk.h>
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
	PROP_DUMMY = 1,
	LAST_PROPERTY
};

struct _IsIndicatorPrivate
{
	GTree *sensors;
	GSList *enabled_sensors;
	GSList *menu_items;
	IsSensor *primary_sensor;
	guint enabled_id;
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

	g_object_class_install_property(gobject_class, PROP_DUMMY,
					g_param_spec_uint("dummy", "dummy property",
							  "dummy property blurp.",
							  0, G_MAXUINT,
							  0,
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

	/* sensors is a dual level tree, the first level is each family - for
	   each family there is a tree of sensors */
	priv->sensors = g_tree_new_full((GCompareDataFunc)g_strcmp0,
					NULL,
					g_free,
					(GDestroyNotify)g_tree_unref);
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
	case PROP_DUMMY:
		g_value_set_uint(value, 0);
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

	/* Make compiler happy */
	(void)priv;

	switch (property_id) {
	case PROP_DUMMY:
		g_value_get_uint(value);
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

	if (priv->enabled_id) {
		g_source_remove(priv->enabled_id);
		priv->enabled_id = 0;
	}
	G_OBJECT_CLASS(is_indicator_parent_class)->dispose(object);
}

static void
is_indicator_finalize(GObject *object)
{
	IsIndicator *self = (IsIndicator *)object;
	IsIndicatorPrivate *priv = self->priv;

	g_tree_unref(priv->sensors);
	g_slist_foreach(priv->enabled_sensors, (GFunc)g_object_unref, NULL);
	g_slist_free(priv->enabled_sensors);

	G_OBJECT_CLASS(is_indicator_parent_class)->finalize(object);
}

static void activate_action(GtkAction *action);

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

static void activate_action(GtkAction *action)
{
	GtkWidget *dialog;

	g_debug("activated action %s", gtk_action_get_name(action));

	dialog = gtk_dialog_new_with_buttons(_("Preferences"),
					     NULL,
					     0,
					     GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					     NULL);
	g_signal_connect_swapped(dialog, "response",
				 G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_widget_show_all(dialog);
}

IsIndicator *
is_indicator_new(const gchar *id,
		 const gchar *icon_name)
{
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GError *error = NULL;
	GtkWidget *menu;

	AppIndicator *self = g_object_new(IS_TYPE_INDICATOR,
					  "id", id,
					  "icon-name", icon_name,
					  "category", "Hardware",
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

static gboolean
update_sensors(IsIndicator *self)
{
	g_return_if_fail(IS_IS_INDICATOR(self));

	g_slist_foreach(self->priv->enabled_sensors,
			(GFunc)is_sensor_update_value,
			self);
	return TRUE;
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
disable_sensor(IsIndicator *self,
	       IsSensor *sensor)
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
	priv->enabled_sensors = g_slist_remove(priv->enabled_sensors,
					       sensor);
	if (!priv->enabled_sensors) {
		g_source_remove(priv->enabled_id);
		priv->enabled_id = 0;
	}
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
disable_sensor_from_tree(const gchar *family,
			 IsSensor *sensor,
			 IsIndicator *self)
{
	disable_sensor(self, sensor);
}

static void
enable_sensor(IsIndicator *self,
	      IsSensor *sensor)
{
	IsIndicatorPrivate *priv = self->priv;
	GtkMenu *menu;
	GtkWidget *menu_item;
	GtkWidget *hbox;
	GtkWidget *label;

	/* debug - enable sensor */
	g_debug("enabling sensor [%s]:%s",
		is_sensor_get_family(sensor),
		is_sensor_get_id(sensor));
	if (!priv->enabled_sensors) {
		priv->enabled_id = g_timeout_add_seconds(5,
							 (GSourceFunc)update_sensors,
							 self);
	}
	priv->enabled_sensors = g_slist_append(priv->enabled_sensors,
					       g_object_ref(sensor));
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

	/* finally update the value of this sensor */
	is_sensor_update_value(sensor);
}

gboolean
is_indicator_add_sensor(IsIndicator *self,
			IsSensor *sensor)
{
	IsIndicatorPrivate *priv;
	GTree *sensors;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_INDICATOR(self), FALSE);
	g_return_val_if_fail(IS_IS_SENSOR(sensor), FALSE);

	priv = self->priv;

	sensors = g_tree_lookup(priv->sensors, is_sensor_get_family(sensor));

	if (!sensors) {
		sensors = g_tree_new_full((GCompareDataFunc)g_strcmp0,
					  NULL,
					  NULL,
					  g_object_unref);
		g_tree_insert(priv->sensors,
			      g_strdup(is_sensor_get_family(sensor)),
			      sensors);
		g_debug("inserted family %s", is_sensor_get_family(sensor));
	} else if (g_tree_lookup(sensors, is_sensor_get_id(sensor))) {
		g_warning("sensor with id %s already exists for family %s, not adding duplicate",
			  is_sensor_get_id(sensor), is_sensor_get_family(sensor));
		goto out;
	}
	g_tree_insert(sensors,
		      g_strdup(is_sensor_get_id(sensor)),
		      g_object_ref(sensor));
	g_debug("inserted sensor with id %s for family %s",
		is_sensor_get_id(sensor), is_sensor_get_family(sensor));
	ret = TRUE;

	if (ret) {
		enable_sensor(self, sensor);
	}

out:
	return ret;
}

gboolean
is_indicator_remove_all_sensors(IsIndicator *self,
				const gchar *family)
{
	IsIndicatorPrivate *priv;
	GTree *sensors;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_INDICATOR(self), FALSE);
	g_return_val_if_fail(family != NULL, FALSE);

	priv = self->priv;

	sensors = g_tree_lookup(priv->sensors, family);

	if (sensors) {
		g_tree_foreach(sensors,
			       (GTraverseFunc)disable_sensor_from_tree,
			       self);
		g_tree_remove(priv->sensors, family);
		ret = TRUE;
	}

out:
	return ret;
}
