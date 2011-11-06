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

#include "is-sensor-dialog.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE(IsSensorDialog, is_sensor_dialog, GTK_TYPE_DIALOG);

static void is_sensor_dialog_dispose(GObject *object);
static void is_sensor_dialog_finalize(GObject *object);
static void is_sensor_dialog_get_property(GObject *object,
					       guint property_id, GValue *value, GParamSpec *pspec);
static void is_sensor_dialog_set_property(GObject *object,
					       guint property_id, const GValue *value, GParamSpec *pspec);

/* properties */
enum {
	PROP_SENSOR = 1,
	LAST_PROPERTY
};

struct _IsSensorDialogPrivate
{
	IsSensor *sensor;
	GtkWidget *table;
	GtkWidget *path_label;
	GtkWidget *label_entry;
	GtkWidget *alarm_mode_combo_box;
	GtkWidget *alarm_value_spin_button;
	GtkWidget *units_label;
};

static void
is_sensor_dialog_class_init(IsSensorDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsSensorDialogPrivate));

	gobject_class->get_property = is_sensor_dialog_get_property;
	gobject_class->set_property = is_sensor_dialog_set_property;
	gobject_class->dispose = is_sensor_dialog_dispose;
	gobject_class->finalize = is_sensor_dialog_finalize;

	g_object_class_install_property(gobject_class, PROP_SENSOR,
					g_param_spec_object("sensor", "sensor property",
							    "sensor property blurp.",
							    IS_TYPE_SENSOR,
							    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
is_sensor_dialog_init(IsSensorDialog *self)
{
	IsSensorDialogPrivate *priv;
	GtkWidget *label;

	self->priv = priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_SENSOR_DIALOG,
					    IsSensorDialogPrivate);

	gtk_window_set_title(GTK_WINDOW(self), _(PACKAGE_NAME " Sensor Properties"));
	gtk_window_set_default_size(GTK_WINDOW(self), 250, 0);

	gtk_dialog_add_button(GTK_DIALOG(self),
			      GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT);

	/* pack content into box */
	priv->table = gtk_table_new(3, 4, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(priv->table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(priv->table), 1);
	gtk_container_set_border_width(GTK_CONTAINER(priv->table), 12);

	priv->path_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(priv->path_label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(priv->table), priv->path_label,
			 0, 4,
			 0, 1,
			 GTK_FILL, GTK_FILL,
			 0, 0);
	gtk_table_set_row_spacing(GTK_TABLE(priv->table), 0, 6);

	label = gtk_label_new(_("Label"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(priv->table), label,
			 0, 1,
			 1, 2,
			 GTK_FILL, GTK_FILL,
			 6, 0);
	priv->label_entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(priv->table), priv->label_entry,
			 1, 4,
			 1, 2,
			 GTK_FILL, GTK_FILL,
			 0, 0);
	gtk_table_set_row_spacing(GTK_TABLE(priv->table), 1, 6);
	label = gtk_label_new(_("Alarm"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(priv->table), label,
			 0, 1,
			 2, 3,
			 GTK_FILL, GTK_FILL,
			 6, 0);
	priv->alarm_mode_combo_box = gtk_combo_box_text_new();
	gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(priv->alarm_mode_combo_box),
				       IS_SENSOR_ALARM_MODE_DISABLED,
				       _("Disabled"));
	gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(priv->alarm_mode_combo_box),
				       IS_SENSOR_ALARM_MODE_LOW,
				       _("Below"));
	gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(priv->alarm_mode_combo_box),
				       IS_SENSOR_ALARM_MODE_HIGH,
				       _("Above"));
	gtk_widget_set_sensitive(priv->alarm_mode_combo_box, FALSE);
	gtk_table_attach(GTK_TABLE(priv->table), priv->alarm_mode_combo_box,
			 1, 2,
			 2, 3,
			 GTK_FILL, GTK_FILL,
			 0, 0);

	priv->alarm_value_spin_button = gtk_spin_button_new_with_range(-G_MAXDOUBLE,
								       G_MAXDOUBLE,
								       1.0f);
	gtk_widget_set_sensitive(priv->alarm_value_spin_button, FALSE);
	gtk_table_attach(GTK_TABLE(priv->table), priv->alarm_value_spin_button,
			 2, 3,
			 2, 3,
			 GTK_FILL, GTK_FILL,
			 0, 0);
	priv->units_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(priv->units_label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(priv->table), priv->units_label,
			 3, 4,
			 2, 3,
			 GTK_EXPAND | GTK_FILL, GTK_FILL,
			 0, 0);

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))),
			  priv->table);
}

static void
is_sensor_dialog_get_property(GObject *object,
				   guint property_id, GValue *value, GParamSpec *pspec)
{
	IsSensorDialog *self = IS_SENSOR_DIALOG(object);
	IsSensorDialogPrivate *priv = self->priv;

	switch (property_id) {
	case PROP_SENSOR:
		g_value_set_object(value, priv->sensor);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
label_changed(GtkEntry *entry,
	      IsSensorDialog *self)
{
	is_sensor_set_label(self->priv->sensor,
			    gtk_entry_get_text(entry));
}

static void
sensor_notify_label(IsSensor *sensor,
		    GParamSpec *pspec,
		    IsSensorDialog *self)
{
	gtk_entry_set_text(GTK_ENTRY(self->priv->label_entry),
			   is_sensor_get_label(sensor));
}

static void
alarm_mode_changed(GtkComboBox *combo_box,
		   IsSensorDialog *self)
{
	is_sensor_set_alarm_mode(self->priv->sensor,
				 gtk_combo_box_get_active(combo_box));
}

static void
sensor_notify_alarm_mode(IsSensor *sensor,
			 GParamSpec *pspec,
			 IsSensorDialog *self)
{
	IsSensorAlarmMode alarm_mode = is_sensor_get_alarm_mode(sensor);
	gtk_widget_set_sensitive(self->priv->alarm_value_spin_button,
				 alarm_mode != IS_SENSOR_ALARM_MODE_DISABLED);
	gtk_combo_box_set_active(GTK_COMBO_BOX(self->priv->alarm_mode_combo_box),
				 alarm_mode);
}

static void
alarm_value_changed(GtkSpinButton *spin_button,
		    IsSensorDialog *self)
{
	is_sensor_set_alarm_value(self->priv->sensor,
				  gtk_spin_button_get_value(spin_button));
}

static void
sensor_notify_alarm_value(IsSensor *sensor,
			  GParamSpec *pspec,
			  IsSensorDialog *self)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->priv->alarm_value_spin_button),
				  is_sensor_get_alarm_value(sensor));
}

static void
sensor_notify_units(IsSensor *sensor,
		    GParamSpec *pspec,
		    IsSensorDialog *self)
{
	gtk_label_set_text(GTK_LABEL(self->priv->units_label),
			   is_sensor_get_units(sensor));
}

static void
is_sensor_dialog_set_property(GObject *object,
				   guint property_id, const GValue *value, GParamSpec *pspec)
{
	IsSensorDialog *self = IS_SENSOR_DIALOG(object);
	IsSensorDialogPrivate *priv = self->priv;
	gchar *markup;

	switch (property_id) {
	case PROP_SENSOR:
		priv->sensor = g_object_ref(g_value_get_object(value));
		markup = g_strdup_printf("<span weight='bold'>%s: %s</span>",
					 _("Sensor"),
					 is_sensor_get_path(priv->sensor));
		gtk_label_set_markup(GTK_LABEL(priv->path_label),
				     markup);
		g_free(markup);
		gtk_widget_set_sensitive(priv->label_entry, TRUE);
		gtk_entry_set_text(GTK_ENTRY(priv->label_entry),
				   is_sensor_get_label(priv->sensor));
		g_signal_connect(priv->label_entry, "changed",
				 G_CALLBACK(label_changed),
				 self);
		g_signal_connect(priv->sensor, "notify::label",
				 G_CALLBACK(sensor_notify_label),
				 self);
		gtk_widget_set_sensitive(priv->alarm_mode_combo_box, TRUE);
		gtk_combo_box_set_active(GTK_COMBO_BOX(priv->alarm_mode_combo_box),
					 is_sensor_get_alarm_mode(priv->sensor));
		g_signal_connect(priv->alarm_mode_combo_box, "changed",
				 G_CALLBACK(alarm_mode_changed),
				 self);
		g_signal_connect(priv->sensor, "notify::alarm-mode",
				 G_CALLBACK(sensor_notify_alarm_mode),
				 self);

		/* set sensitive so we can update */
		gtk_widget_set_sensitive(priv->alarm_value_spin_button, TRUE);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(priv->alarm_value_spin_button),
					   is_sensor_get_digits(priv->sensor));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->alarm_value_spin_button),
					  is_sensor_get_alarm_value(priv->sensor));
		g_signal_connect(priv->alarm_value_spin_button, "value-changed",
				 G_CALLBACK(alarm_value_changed),
				 self);
		g_signal_connect(priv->sensor, "notify::alarm-value",
				 G_CALLBACK(sensor_notify_alarm_value),
				 self);
		gtk_widget_set_sensitive(priv->alarm_value_spin_button,
					 (is_sensor_get_alarm_mode(priv->sensor) !=
					  IS_SENSOR_ALARM_MODE_DISABLED));
		gtk_widget_set_sensitive(priv->units_label, TRUE);
		gtk_label_set_text(GTK_LABEL(priv->units_label),
					 is_sensor_get_units(priv->sensor));
		g_signal_connect(priv->sensor, "notify::units",
				 G_CALLBACK(sensor_notify_units),
				 self);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}


static void
is_sensor_dialog_dispose(GObject *object)
{
	IsSensorDialog *self = (IsSensorDialog *)object;
	IsSensorDialogPrivate *priv = self->priv;

	if (priv->sensor) {
		g_object_unref(priv->sensor);
		priv->sensor = NULL;
	}
	G_OBJECT_CLASS(is_sensor_dialog_parent_class)->dispose(object);
}

static void
is_sensor_dialog_finalize(GObject *object)
{
	IsSensorDialog *self = (IsSensorDialog *)object;

	/* Make compiler happy */
	(void)self;

	G_OBJECT_CLASS(is_sensor_dialog_parent_class)->finalize(object);
}

GtkWidget *is_sensor_dialog_new(IsSensor *sensor)
{
	return GTK_WIDGET(g_object_new(IS_TYPE_SENSOR_DIALOG,
				       "sensor", sensor,
				       NULL));
}
