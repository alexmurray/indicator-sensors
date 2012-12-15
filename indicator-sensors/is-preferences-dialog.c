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
#include "is-indicator.h"
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
	PROP_APPLICATION = 1,
	LAST_PROPERTY
};

struct _IsPreferencesDialogPrivate
{
	IsApplication *application;
        GSettings *application_settings;
        GSettings *indicator_settings;
	GtkWidget *grid;
	GtkWidget *autostart_check_button;
	GtkWidget *celsius_radio_button;
	GtkWidget *fahrenheit_radio_button;
	GtkWidget *display_icon_check_button;
	GtkWidget *display_label_check_button;
	GtkWidget *display_value_check_button;
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

	g_object_class_install_property(gobject_class, PROP_APPLICATION,
					g_param_spec_object("application", "application property",
							    "application property blurp.",
							    IS_TYPE_APPLICATION,
							    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
temperature_scale_toggled(GtkToggleButton *toggle_button,
			  IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv;

	priv = self->priv;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->celsius_radio_button))) {
                g_settings_set_int(priv->application_settings, "temperature-scale",
                                   IS_TEMPERATURE_SENSOR_SCALE_CELSIUS);
	} else {
                g_settings_set_int(priv->application_settings, "temperature-scale",
                                   IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT);
	}
}

static void
is_preferences_dialog_init(IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv;
	GtkWidget *label;
	GtkWidget *box;
	gchar *markup;

	self->priv = priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_PREFERENCES_DIALOG,
					    IsPreferencesDialogPrivate);

        priv->application_settings = g_settings_new("indicator-sensors.application");
        priv->indicator_settings = g_settings_new("indicator-sensors.indicator");

	gtk_window_set_title(GTK_WINDOW(self), _("Hardware Sensors Indicator Preferences"));
	gtk_window_set_default_size(GTK_WINDOW(self), 500, 600);

	priv->sensor_properties_button =
		gtk_dialog_add_button(GTK_DIALOG(self),
				      GTK_STOCK_PROPERTIES,
				      IS_PREFERENCES_DIALOG_RESPONSE_SENSOR_PROPERTIES);
	gtk_widget_set_sensitive(priv->sensor_properties_button, FALSE);
	gtk_dialog_add_button(GTK_DIALOG(self),
			      GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT);

	/* pack content into box */
	priv->grid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(priv->grid), 6);
	gtk_grid_set_row_spacing(GTK_GRID(priv->grid), 6);
	gtk_container_set_border_width(GTK_CONTAINER(priv->grid), 6);

	label = gtk_label_new(NULL);
	markup = g_strdup_printf("<span weight='bold'>%s</span>",
				 _("General"));
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_grid_attach(GTK_GRID(priv->grid), label,
			0, 0,
			1, 1) ;
	priv->autostart_check_button = gtk_check_button_new_with_label
		(_("Start automatically on login"));
	gtk_widget_set_sensitive(priv->autostart_check_button, FALSE);
	gtk_grid_attach(GTK_GRID(priv->grid), priv->autostart_check_button,
			0, 1,
			3, 1);

	label = gtk_label_new(_("Primary sensor display"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 0);
	gtk_grid_attach(GTK_GRID(priv->grid), label,
			0, 2,
			1, 1);

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	priv->display_icon_check_button = gtk_check_button_new_with_label (_("Icon"));
	gtk_widget_set_sensitive(priv->display_icon_check_button, FALSE);
	gtk_box_pack_start(GTK_BOX(box), priv->display_icon_check_button,
			   TRUE, TRUE, 0);

	priv->display_label_check_button = gtk_check_button_new_with_label (_("Label"));
	gtk_widget_set_sensitive(priv->display_label_check_button, FALSE);
	gtk_box_pack_start(GTK_BOX(box), priv->display_label_check_button,
			   TRUE, TRUE, 0);

	priv->display_value_check_button = gtk_check_button_new_with_label (_("Value"));
	gtk_widget_set_sensitive(priv->display_value_check_button, FALSE);
	gtk_box_pack_start(GTK_BOX(box), priv->display_value_check_button,
			   TRUE, TRUE, 0);

	gtk_grid_attach(GTK_GRID(priv->grid), box,
			1, 2,
			2, 1);

	label = gtk_label_new(_("Temperature scale"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 0);
	gtk_grid_attach(GTK_GRID(priv->grid), label,
			0, 3,
			1, 1);

	priv->celsius_radio_button = gtk_radio_button_new_with_label
		(NULL, _("Celsius (\302\260C)"));
	gtk_widget_set_sensitive(priv->celsius_radio_button, FALSE);
	gtk_widget_set_hexpand(priv->celsius_radio_button, TRUE);
	gtk_grid_attach(GTK_GRID(priv->grid), priv->celsius_radio_button,
			1, 3,
			1, 1);
	g_signal_connect(priv->celsius_radio_button, "toggled",
			 G_CALLBACK(temperature_scale_toggled), self);
	priv->fahrenheit_radio_button =
		gtk_radio_button_new_with_label_from_widget
		(GTK_RADIO_BUTTON(priv->celsius_radio_button),
		 _("Fahrenheit (\302\260F)"));
	gtk_widget_set_sensitive(priv->fahrenheit_radio_button, FALSE);
	gtk_widget_set_hexpand(priv->fahrenheit_radio_button, TRUE);
	gtk_grid_attach(GTK_GRID(priv->grid), priv->fahrenheit_radio_button,
			2, 3,
			1, 1);
	g_signal_connect(priv->fahrenheit_radio_button, "toggled",
			 G_CALLBACK(temperature_scale_toggled), self);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self))),
			   priv->grid, TRUE, TRUE, 0);
}

static void
is_preferences_dialog_get_property(GObject *object,
				   guint property_id, GValue *value, GParamSpec *pspec)
{
	IsPreferencesDialog *self = IS_PREFERENCES_DIALOG(object);
	IsPreferencesDialogPrivate *priv = self->priv;

	switch (property_id) {
	case PROP_APPLICATION:
		g_value_set_object(value, priv->application);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
update_flags_sensitivities(IsPreferencesDialog *self)
{
        IsPreferencesDialogPrivate *priv = self->priv;
	IsIndicatorDisplayFlags flags = g_settings_get_int(priv->indicator_settings,
                                                           "display-flags");

        if (flags == IS_INDICATOR_DISPLAY_ICON) {
                /* only icon is shown - make sure it can't be disabled */
                gtk_widget_set_sensitive(priv->display_icon_check_button,
                                         FALSE);
        } else if (flags == IS_INDICATOR_DISPLAY_LABEL) {
                /* only label is shown - make sure it can't be disabled */
                gtk_widget_set_sensitive(priv->display_label_check_button,
                                         FALSE);
        } else if (flags == IS_INDICATOR_DISPLAY_VALUE) {
                /* only value is shown - make sure it can't be disabled */
                gtk_widget_set_sensitive(priv->display_value_check_button,
                                         FALSE);
        } else {
                /* set all sensitive */
                gtk_widget_set_sensitive(priv->display_icon_check_button,
                                         TRUE);
                gtk_widget_set_sensitive(priv->display_label_check_button,
                                         TRUE);
                gtk_widget_set_sensitive(priv->display_value_check_button,
                                         TRUE);
        }
}

static void
display_icon_toggled(GtkToggleButton *toggle_button,
		     IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv = self->priv;
	gboolean display_icon = gtk_toggle_button_get_active(toggle_button);
	IsIndicatorDisplayFlags flags = g_settings_get_int(priv->indicator_settings,
                                                           "display-flags");

	if (display_icon) {
		flags |= IS_INDICATOR_DISPLAY_ICON;
	} else {
		flags &= ~IS_INDICATOR_DISPLAY_ICON;
	}
        g_settings_set_int(priv->indicator_settings,
                           "display-flags", flags);
        update_flags_sensitivities(self);
}

static void
display_label_toggled(GtkToggleButton *toggle_button,
		     IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv = self->priv;
	gboolean display_label = gtk_toggle_button_get_active(toggle_button);
	IsIndicatorDisplayFlags flags = g_settings_get_int(priv->indicator_settings,
                                                           "display-flags");

	if (display_label) {
		flags |= IS_INDICATOR_DISPLAY_LABEL;
	} else {
		flags &= ~IS_INDICATOR_DISPLAY_LABEL;
	}
        g_settings_set_int(priv->indicator_settings,
                           "display-flags", flags);
        update_flags_sensitivities(self);
}

static void
display_value_toggled(GtkToggleButton *toggle_button,
                      IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv = self->priv;
	gboolean display_value = gtk_toggle_button_get_active(toggle_button);
	IsIndicatorDisplayFlags flags = g_settings_get_int(priv->indicator_settings,
                                                           "display-flags");

	if (display_value) {
		flags |= IS_INDICATOR_DISPLAY_VALUE;
	} else {
		flags &= ~IS_INDICATOR_DISPLAY_VALUE;
	}
        g_settings_set_int(priv->indicator_settings,
                           "display-flags", flags);
        update_flags_sensitivities(self);
}

static void
settings_display_flags_changed(GSettings *settings,
                               gchar *key,
			       IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv = self->priv;
	IsIndicatorDisplayFlags flags = g_settings_get_int(priv->indicator_settings,
                                                           "display-flags");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->display_icon_check_button),
				     flags & IS_INDICATOR_DISPLAY_ICON);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->display_label_check_button),
				     flags & IS_INDICATOR_DISPLAY_LABEL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->display_value_check_button),
				     flags & IS_INDICATOR_DISPLAY_VALUE);
}

static void
autostart_toggled(GtkToggleButton *toggle_button,
		  IsApplication *application)
{
	is_application_set_autostart(application,
                                     gtk_toggle_button_get_active(toggle_button));
}

static void
application_notify_autostart(IsApplication *application,
			 GParamSpec *pspec,
			 GtkToggleButton *check_button)
{
	gtk_toggle_button_set_active(check_button,
				     is_application_get_autostart(application));
}

static void
manager_selection_changed(GtkTreeSelection *selection,
			  IsPreferencesDialog *self)
{
	IsSensor *sensor;
	gboolean sensitive = FALSE;

	sensor = is_manager_get_selected_sensor(is_application_get_manager(self->priv->application));
	if (sensor) {
		sensitive = TRUE;
		g_object_unref(sensor);
	}
	gtk_widget_set_sensitive(self->priv->sensor_properties_button,
				 sensitive);
}

static void
manager_row_activated(GtkTreeView *tree_view,
		      GtkTreePath *path,
		      GtkTreeViewColumn *column,
		      IsPreferencesDialog *self)
{
	/* when row is activated. simply select it and emit the properties
	   signal */
	gtk_tree_selection_select_path(gtk_tree_view_get_selection(tree_view), path);
	gtk_dialog_response(GTK_DIALOG(self),
			    IS_PREFERENCES_DIALOG_RESPONSE_SENSOR_PROPERTIES);
}

static void
is_preferences_dialog_set_property(GObject *object,
				   guint property_id, const GValue *value, GParamSpec *pspec)
{
	IsPreferencesDialog *self = IS_PREFERENCES_DIALOG(object);
	IsPreferencesDialogPrivate *priv = self->priv;
	GtkWidget *scrolled_window;
        IsManager *manager;
	IsIndicatorDisplayFlags flags;

	switch (property_id) {
	case PROP_APPLICATION:
		priv->application = g_object_ref(g_value_get_object(value));
		gtk_widget_set_sensitive(priv->display_icon_check_button, TRUE);
		gtk_widget_set_sensitive(priv->display_label_check_button, TRUE);
		gtk_widget_set_sensitive(priv->display_value_check_button, TRUE);

		flags = g_settings_get_int(self->priv->indicator_settings,
                                           "display-flags");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->display_icon_check_button),
					     flags & IS_INDICATOR_DISPLAY_ICON);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->display_label_check_button),
					     flags & IS_INDICATOR_DISPLAY_LABEL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->display_value_check_button),
					     flags & IS_INDICATOR_DISPLAY_VALUE);
		g_signal_connect(priv->display_icon_check_button, "toggled",
				 G_CALLBACK(display_icon_toggled),
				 self);
		g_signal_connect(priv->display_label_check_button, "toggled",
				 G_CALLBACK(display_label_toggled),
				 self);
		g_signal_connect(priv->display_value_check_button, "toggled",
				 G_CALLBACK(display_value_toggled),
				 self);
		g_signal_connect(priv->indicator_settings, "changed::display-flags",
				 G_CALLBACK(settings_display_flags_changed),
				 self);
		manager = is_application_get_manager(priv->application);
		g_signal_connect(manager, "row-activated",
				 G_CALLBACK(manager_row_activated), self);
		/* control properties button sensitivity */
		g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(manager)),
				 "changed", G_CALLBACK(manager_selection_changed),
				 self);
		/* set state of autostart checkbutton */
		gtk_widget_set_sensitive(priv->autostart_check_button, TRUE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->autostart_check_button),
					     is_application_get_autostart(priv->application));
		gtk_widget_set_sensitive(priv->celsius_radio_button, TRUE);
		gtk_widget_set_sensitive(priv->fahrenheit_radio_button, TRUE);

		switch (is_application_get_temperature_scale(priv->application)) {
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
				 G_CALLBACK(autostart_toggled), priv->application);
		g_signal_connect(priv->application, "notify::autostart",
				 G_CALLBACK(application_notify_autostart),
				 priv->autostart_check_button);
		scrolled_window = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
					       GTK_POLICY_AUTOMATIC,
					       GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolled_window),
				  GTK_WIDGET(manager));
		gtk_widget_set_hexpand(scrolled_window, TRUE);
		gtk_widget_set_vexpand(scrolled_window, TRUE);
		gtk_grid_attach(GTK_GRID(priv->grid),
				scrolled_window,
				0, 4,
				3, 1);
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

	if (priv->application) {
		g_object_unref(priv->application);
		priv->application = NULL;
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

GtkWidget *is_preferences_dialog_new(IsApplication *application)
{
	return GTK_WIDGET(g_object_new(IS_TYPE_PREFERENCES_DIALOG,
				       "application", application,
				       NULL));
}
