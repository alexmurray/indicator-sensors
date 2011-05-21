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
	PROP_INDICATOR = 1,
	LAST_PROPERTY
};

struct _IsPreferencesDialogPrivate
{
	IsIndicator *indicator;
	GtkWidget *table;
	GtkWidget *autostart_check_button;
	GtkWidget *celsius_radio_button;
	GtkWidget *fahrenheit_radio_button;
	GtkWidget *display_mode_combo_box;
	GtkWidget *sensor_properties_button;
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

	g_object_class_install_property(gobject_class, PROP_INDICATOR,
					g_param_spec_object("indicator", "indicator property",
							    "indicator property blurp.",
							    IS_TYPE_INDICATOR,
							    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
temperature_scale_toggled(GtkToggleButton *toggle_button,
			  IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv;
	IsManager *manager;

	priv = self->priv;

	manager = is_indicator_get_manager(priv->indicator);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->celsius_radio_button))) {
		is_manager_set_temperature_scale(manager,
						 IS_TEMPERATURE_SENSOR_SCALE_CELSIUS);
	} else {
		is_manager_set_temperature_scale(manager,
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

	gtk_window_set_title(GTK_WINDOW(self), _(PACKAGE_NAME " Preferences"));
	gtk_window_set_default_size(GTK_WINDOW(self), 350, 500);

	priv->sensor_properties_button =
		gtk_dialog_add_button(GTK_DIALOG(self),
				      GTK_STOCK_PROPERTIES,
				      IS_PREFERENCES_DIALOG_RESPONSE_SENSOR_PROPERTIES);
	gtk_widget_set_sensitive(priv->sensor_properties_button, FALSE);
	gtk_dialog_add_button(GTK_DIALOG(self),
			      GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT);

	/* pack content into box */
	priv->table = gtk_table_new(5, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(priv->table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(priv->table), 6);
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
			 0, 3,
			 1, 2,
			 GTK_FILL, GTK_FILL,
			 6, 0);

	label = gtk_label_new(_("Primary sensor display"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 0);
	gtk_table_attach(GTK_TABLE(priv->table), label,
			 0, 1,
			 2, 3,
			 GTK_FILL, GTK_FILL,
			 6, 0);
	priv->display_mode_combo_box = gtk_combo_box_text_new();
	gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(priv->display_mode_combo_box),
				       IS_INDICATOR_DISPLAY_MODE_VALUE_ONLY,
				       _("Value only"));
	gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(priv->display_mode_combo_box),
				       IS_INDICATOR_DISPLAY_MODE_LABEL_AND_VALUE,
				       _("Label and value"));
	gtk_widget_set_sensitive(priv->display_mode_combo_box, FALSE);
	gtk_table_attach(GTK_TABLE(priv->table), priv->display_mode_combo_box,
			 1, 3,
			 2, 3,
			 GTK_FILL, GTK_FILL,
			 0, 0);

	label = gtk_label_new(_("Temperature scale"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 0);
	gtk_table_attach(GTK_TABLE(priv->table), label,
			 0, 1,
			 3, 4,
			 GTK_FILL, GTK_FILL,
			 6, 0);

	priv->celsius_radio_button = gtk_radio_button_new_with_label
		(NULL, _("Celsius (\302\260C)"));
	gtk_widget_set_sensitive(priv->celsius_radio_button, FALSE);
	gtk_table_attach(GTK_TABLE(priv->table), priv->celsius_radio_button,
			 1, 2,
			 3, 4,
			 GTK_EXPAND | GTK_FILL, GTK_FILL,
			 0, 0);
	g_signal_connect(priv->celsius_radio_button, "toggled",
			 G_CALLBACK(temperature_scale_toggled), self);
	priv->fahrenheit_radio_button =
		gtk_radio_button_new_with_label_from_widget
		(GTK_RADIO_BUTTON(priv->celsius_radio_button),
		 _("Fahrenheit (\302\260F)"));
	gtk_widget_set_sensitive(priv->fahrenheit_radio_button, FALSE);
	gtk_table_attach(GTK_TABLE(priv->table), priv->fahrenheit_radio_button,
			 2, 3,
			 3, 4,
			 GTK_EXPAND | GTK_FILL, GTK_FILL,
			 6, 0);
	g_signal_connect(priv->fahrenheit_radio_button, "toggled",
			 G_CALLBACK(temperature_scale_toggled), self);

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
	case PROP_INDICATOR:
		g_value_set_object(value, priv->indicator);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
display_mode_changed(GtkComboBox *combo_box,
		     IsIndicator *indicator)
{
	/* IS_INDICATOR_DISPLAY_MODE_INVALID is 0 so need to offset by
	 * 1 */
	is_indicator_set_display_mode(indicator,
				      gtk_combo_box_get_active(combo_box) + 1);
}

static void
indicator_notify_display_mode(IsIndicator *indicator,
			      GParamSpec *pspec,
			      GtkComboBox *combo_box)
{
	/* IS_INDICATOR_DISPLAY_MODE_INVALID is 0 so need to offset by
	 * 1 */
	gtk_combo_box_set_active(combo_box,
				 is_indicator_get_display_mode(indicator) - 1);
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
manager_selection_changed(GtkTreeSelection *selection,
			  IsPreferencesDialog *self)
{
	IsSensor *sensor;
	gboolean sensitive = FALSE;

	sensor = is_manager_get_selected_sensor(is_indicator_get_manager(self->priv->indicator));
	if (sensor) {
		sensitive = TRUE;
		g_object_unref(sensor);
	}
	gtk_widget_set_sensitive(self->priv->sensor_properties_button,
				 sensitive);
}

static void
is_preferences_dialog_set_property(GObject *object,
				   guint property_id, const GValue *value, GParamSpec *pspec)
{
	IsPreferencesDialog *self = IS_PREFERENCES_DIALOG(object);
	IsPreferencesDialogPrivate *priv = self->priv;
	GtkWidget *scrolled_window;
	IsManager *manager;

	switch (property_id) {
	case PROP_INDICATOR:
		priv->indicator = g_object_ref(g_value_get_object(value));
		gtk_widget_set_sensitive(priv->display_mode_combo_box, TRUE);
		/* IS_INDICATOR_DISPLAY_MODE_INVALID is 0 so need to offset by
		 * 1 */
		gtk_combo_box_set_active(GTK_COMBO_BOX(priv->display_mode_combo_box),
					 is_indicator_get_display_mode(priv->indicator) - 1);
		g_signal_connect(priv->display_mode_combo_box, "changed",
				 G_CALLBACK(display_mode_changed),
				 priv->indicator);
		g_signal_connect(priv->indicator, "notify::display-mode",
				 G_CALLBACK(indicator_notify_display_mode),
				 priv->display_mode_combo_box);
		manager = is_indicator_get_manager(priv->indicator);
		/* control properties button sensitivity */
		g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(manager)),
				 "changed", G_CALLBACK(manager_selection_changed),
				 self);
		/* set state of autostart checkbutton */
		gtk_widget_set_sensitive(priv->autostart_check_button, TRUE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->autostart_check_button),
					     is_manager_get_autostart(manager));
		gtk_widget_set_sensitive(priv->celsius_radio_button, TRUE);
		gtk_widget_set_sensitive(priv->fahrenheit_radio_button, TRUE);

		switch (is_manager_get_temperature_scale(manager)) {
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
				 G_CALLBACK(autostart_toggled), manager);
		g_signal_connect(manager, "notify::autostart",
				 G_CALLBACK(manager_notify_autostart),
				 priv->autostart_check_button);
		scrolled_window = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
					       GTK_POLICY_AUTOMATIC,
					       GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolled_window),
				  GTK_WIDGET(manager));
		gtk_table_attach(GTK_TABLE(priv->table), scrolled_window,
				 0, 3,
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

	if (priv->indicator) {
		g_object_unref(priv->indicator);
		priv->indicator = NULL;
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

GtkWidget *is_preferences_dialog_new(IsIndicator *indicator)
{
	return GTK_WIDGET(g_object_new(IS_TYPE_PREFERENCES_DIALOG,
				       "indicator", indicator,
				       NULL));
}
