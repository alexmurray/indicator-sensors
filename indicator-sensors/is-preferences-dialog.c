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
	GtkWidget *vbox;
	GtkWidget *autostart_check_button;
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
is_preferences_dialog_init(IsPreferencesDialog *self)
{
	IsPreferencesDialogPrivate *priv;

	self->priv = priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_PREFERENCES_DIALOG,
					    IsPreferencesDialogPrivate);

	gtk_window_set_title(GTK_WINDOW(self), _("Preferences"));
	gtk_window_set_default_size(GTK_WINDOW(self), 400, 500);

	gtk_dialog_add_button(GTK_DIALOG(self),
			      GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);

	priv->autostart_check_button = gtk_check_button_new_with_label
		(_("Start automatically on login"));
	gtk_widget_set_sensitive(priv->autostart_check_button, FALSE);
	priv->vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_end(GTK_BOX(priv->vbox), priv->autostart_check_button,
			 FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))),
			  priv->vbox);
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
		gtk_box_pack_start(GTK_BOX(priv->vbox),
				   scrolled_window, TRUE, TRUE, 0);
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
