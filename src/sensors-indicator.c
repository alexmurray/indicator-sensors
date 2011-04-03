/*
 * sensors-indicator.c - Source for SensorsIndicator
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "sensors-indicator.h"
#include <gtk/gtk.h>

G_DEFINE_TYPE(SensorsIndicator, sensors_indicator, APP_INDICATOR_TYPE);

static void sensors_indicator_dispose(GObject *object);
static void sensors_indicator_finalize(GObject *object);
static void sensors_indicator_get_property(GObject *object,
					   guint property_id, GValue *value, GParamSpec *pspec);
static void sensors_indicator_set_property(GObject *object,
					   guint property_id, const GValue *value, GParamSpec *pspec);

/* properties */
enum {
	PROP_DUMMY = 1,
	LAST_PROPERTY
};

struct _SensorsIndicatorPrivate
{
	gpointer dummy;
};

static void activate_action(GtkAction *action);

static GtkActionEntry entries[] = {
  { "Preferences", "application-preferences", "_Preferences", NULL,
    "Preferences...", G_CALLBACK(activate_action) },
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
	g_debug("activated action %s", gtk_action_get_name(action));
}

static void
sensors_indicator_class_init(SensorsIndicatorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(SensorsIndicatorPrivate));

	gobject_class->get_property = sensors_indicator_get_property;
	gobject_class->set_property = sensors_indicator_set_property;
	gobject_class->dispose = sensors_indicator_dispose;
	gobject_class->finalize = sensors_indicator_finalize;

	g_object_class_install_property(gobject_class, PROP_DUMMY,
					g_param_spec_uint("dummy", "dummy property",
							  "dummy property blurp.",
							  0, G_MAXUINT,
							  0,
							  G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
sensors_indicator_init(SensorsIndicator *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, SENSORS_TYPE_INDICATOR,
						 SensorsIndicatorPrivate);;
}

static void
sensors_indicator_get_property(GObject *object,
			       guint property_id, GValue *value, GParamSpec *pspec)
{
	SensorsIndicator *self = SENSORS_INDICATOR(object);
	SensorsIndicatorPrivate *priv = self->priv;

	/* Make compiler happy */
	(void)priv;

	switch(property_id) {
	case PROP_DUMMY:
		g_value_set_uint(value, 0);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
sensors_indicator_set_property(GObject *object,
			       guint property_id, const GValue *value, GParamSpec *pspec)
{
	SensorsIndicator *self = SENSORS_INDICATOR(object);
	SensorsIndicatorPrivate *priv = self->priv;

	/* Make compiler happy */
	(void)priv;

	switch(property_id) {
	case PROP_DUMMY:
		g_value_get_uint(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
sensors_indicator_dispose(GObject *object)
{
	G_OBJECT_CLASS(sensors_indicator_parent_class)->dispose(object);
}

static void
sensors_indicator_finalize(GObject *object)
{
	G_OBJECT_CLASS(sensors_indicator_parent_class)->finalize(object);
}

AppIndicator *sensors_indicator_new(const gchar *id,
				    const gchar *icon_name)
{
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GError *error = NULL;
	GtkWidget *menu;

	AppIndicator *self = g_object_new(SENSORS_TYPE_INDICATOR,
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

	app_indicator_set_status(self, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_attention_icon(self, "sensors-indicator");
	app_indicator_set_label(self, "icon", "icon");
	app_indicator_set_menu(self, GTK_MENU(menu));

	return APP_INDICATOR(self);
}
