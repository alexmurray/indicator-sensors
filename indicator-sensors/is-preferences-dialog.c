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

#include "is-preferences-dialog.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE(IsPreferencesDialog, is_preferences_dialog, GTK_TYPE_DIALOG);

static void is_preferences_dialog_dispose(GObject *object);
static void is_preferences_dialog_finalize(GObject *object);
static void is_preferences_dialog_get_property(GObject *object,
					       guint property_id, GValue *value, GParamSpec *pspec);
static void is_preferences_dialog_set_property(GObject *object,
					       guint property_id, const GValue *value, GParamSpec *pspec);

/* properties */
enum {
	PROP_MANAGER = 1,
	LAST_PROPERTY
};

struct _IsPreferencesDialogPrivate
{
	IsManager *manager;
	GtkWidget *table;
	GtkWidget *autostart_check_button;
	GtkWidget *celsius_radio_button;
	GtkWidget *fahrenheit_radio_button;
};

static void
is_preferences_dialog_class_init(IsPreferencesDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsPreferencesDialogPrivate));

	gobject_class->get_property = is_preferences_dialog_get_property;
	gobject_class->set_property = is_preferences_dialog_set_property;
	gobject_class->dispose = is_preferences_dialog_dispose;
	gobject_class->finalize = is_preferences_dialog_finalize;

	g_object_class_install_property(gobject_class, PROP_MANAGER,
					g_param_spec_object("manager", "manager property",
							    "manager property blurp.",
							    IS_TYPE_MANAGER,
							    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
temperature_scale_toggled(GtkToggleButton *toggle_button,
			  IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv;

	priv = self->priv;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->celsius_radio_button))) {
		is_manager_set_temperature_scale(priv->manager,
						 IS_TEMPERATURE_SENSOR_SCALE_CELSIUS);
	} else {
		is_manager_set_temperature_scale(priv->manager,
						 IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT);
	}
}

static void
is_preferences_dialog_init(IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv;
	GtkWidget *label;
	gchar *markup;

	self->priv = priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_PREFERENCES_DIALOG,
					    IsPreferencesDialogPrivate);

	gtk_window_set_title(GTK_WINDOW(self), _("Hardware Sensors Indicator Preferences"));
	gtk_window_set_default_size(GTK_WINDOW(self), 400, 500);

	gtk_dialog_add_button(GTK_DIALOG(self),
			      GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT);

	/* pack content into box */
	priv->table = gtk_table_new(5, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(priv->table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(priv->table), 1);
	gtk_container_set_border_width(GTK_CONTAINER(priv->table), 12);

	label = gtk_label_new(NULL);
	markup = g_strdup_printf("<span weight='bold'>%s</span>",
				 _("General"));
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	gtk_table_attach(GTK_TABLE(priv->table), label,
			 0, 1,
			 0, 1,
			 GTK_FILL, GTK_FILL,
			 0, 0);
	priv->autostart_check_button = gtk_check_button_new_with_label
		(_("Start automatically on login"));
	gtk_widget_set_sensitive(priv->autostart_check_button, FALSE);
	gtk_table_attach(GTK_TABLE(priv->table), priv->autostart_check_button,
			 0, 2,
			 1, 2,
			 GTK_FILL, GTK_FILL,
			 6, 0);
	gtk_table_set_row_spacing(GTK_TABLE(priv->table), 1, 6);
	label = gtk_label_new(NULL);
	markup = g_strdup_printf("<span weight='bold'>%s</span>",
				 _("Temperature Scale"));
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(priv->table), label,
			 0, 1,
			 2, 3,
			 GTK_FILL, GTK_FILL,
			 0, 0);

	priv->celsius_radio_button = gtk_radio_button_new_with_label
		(NULL, _("Celsius (\302\260C)"));
	gtk_widget_set_sensitive(priv->celsius_radio_button, FALSE);
	gtk_table_attach(GTK_TABLE(priv->table), priv->celsius_radio_button,
			 0, 1,
			 3, 4,
			 GTK_EXPAND | GTK_FILL, GTK_FILL,
			 6, 0);
	g_signal_connect(priv->celsius_radio_button, "toggled",
			 G_CALLBACK(temperature_scale_toggled), self);
	priv->fahrenheit_radio_button =
		gtk_radio_button_new_with_label_from_widget
		(GTK_RADIO_BUTTON(priv->celsius_radio_button),
		 _("Fahrenheit (\302\260F)"));
	gtk_widget_set_sensitive(priv->fahrenheit_radio_button, FALSE);
	gtk_table_attach(GTK_TABLE(priv->table), priv->fahrenheit_radio_button,
			 1, 2,
			 3, 4,
			 GTK_EXPAND | GTK_FILL, GTK_FILL,
			 6, 0);
	g_signal_connect(priv->fahrenheit_radio_button, "toggled",
			 G_CALLBACK(temperature_scale_toggled), self);

	gtk_table_set_row_spacing(GTK_TABLE(priv->table), 3, 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))),
			  priv->table);
}

static void
is_preferences_dialog_get_property(GObject *object,
				   guint property_id, GValue *value, GParamSpec *pspec)
{
	IsPreferencesDialog *self = IS_PREFERENCES_DIALOG(object);
	IsPreferencesDialogPrivate *priv = self->priv;

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
autostart_toggled(GtkToggleButton *toggle_button,
		  IsManager *manager)
{
	is_manager_set_autostart(manager,
				 gtk_toggle_button_get_active(toggle_button));
}

static void
manager_notify_autostart(IsManager *manager,
			 GParamSpec *pspec,
			 GtkToggleButton *check_button)
{
	gtk_toggle_button_set_active(check_button,
				     is_manager_get_autostart(manager));
}

static void
is_preferences_dialog_set_property(GObject *object,
				   guint property_id, const GValue *value, GParamSpec *pspec)
{
	IsPreferencesDialog *self = IS_PREFERENCES_DIALOG(object);
	IsPreferencesDialogPrivate *priv = self->priv;
	GtkWidget *scrolled_window;

	switch (property_id) {
	case PROP_MANAGER:
		priv->manager = g_object_ref(g_value_get_object(value));
		/* set state of autostart checkbutton */
		gtk_widget_set_sensitive(priv->autostart_check_button, TRUE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->autostart_check_button),
					     is_manager_get_autostart(priv->manager));
		gtk_widget_set_sensitive(priv->celsius_radio_button, TRUE);
		gtk_widget_set_sensitive(priv->fahrenheit_radio_button, TRUE);

		switch (is_manager_get_temperature_scale(priv->manager)) {
		case IS_TEMPERATURE_SENSOR_SCALE_CELSIUS:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->celsius_radio_button),
						     TRUE);
			break;

		case IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->fahrenheit_radio_button),
						     TRUE);
			break;

		case IS_TEMPERATURE_SENSOR_SCALE_INVALID:
		case NUM_IS_TEMPERATURE_SENSOR_SCALE:
		default:
			g_assert_not_reached();
		}
		g_signal_connect(priv->autostart_check_button, "toggled",
				 G_CALLBACK(autostart_toggled), priv->manager);
		g_signal_connect(priv->manager, "notify::autostart",
				 G_CALLBACK(manager_notify_autostart),
				 priv->autostart_check_button);
		scrolled_window = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
					       GTK_POLICY_AUTOMATIC,
					       GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolled_window),
				  GTK_WIDGET(priv->manager));
		gtk_table_attach(GTK_TABLE(priv->table), scrolled_window,
				 0, 2,
				 4, 5,
				 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				 0, 0);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}


static void
is_preferences_dialog_dispose(GObject *object)
{
	IsPreferencesDialog *self = (IsPreferencesDialog *)object;
	IsPreferencesDialogPrivate *priv = self->priv;

	if (priv->manager) {
		g_object_unref(priv->manager);
		priv->manager = NULL;
	}
	G_OBJECT_CLASS(is_preferences_dialog_parent_class)->dispose(object);
}

static void
is_preferences_dialog_finalize(GObject *object)
{
	IsPreferencesDialog *self = (IsPreferencesDialog *)object;

	/* Make compiler happy */
	(void)self;

	G_OBJECT_CLASS(is_preferences_dialog_parent_class)->finalize(object);
}

GtkWidget *is_preferences_dialog_new(IsManager *manager)
{
	return GTK_WIDGET(g_object_new(IS_TYPE_PREFERENCES_DIALOG,
				       "manager", manager,
				       NULL));
}
