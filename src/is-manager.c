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

#include "is-manager.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE(IsManager, is_manager, GTK_TYPE_TREE_VIEW);

static void is_manager_dispose(GObject *object);
static void is_manager_finalize(GObject *object);
static void
is_manager_get_property(GObject *object,
			guint property_id, GValue *value, GParamSpec *pspec);
static void
is_manager_set_property(GObject *object,
			guint property_id, const GValue *value, GParamSpec *pspec);

/* signal enum */
enum {
	SIGNAL_SENSOR_ADDED,
	SIGNAL_SENSOR_REMOVED,
	SIGNAL_SENSOR_ENABLED,
	SIGNAL_SENSOR_DISABLED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};


#define DEFAULT_POLL_TIMEOUT 5

/* properties */
enum {
	PROP_POLL_TIMEOUT = 1,
	LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = {NULL};

struct _IsManagerPrivate
{
	IsStore *store;
	guint poll_timeout;
	glong poll_timeout_id;
	GSList *enabled_sensors;
};

static void
is_manager_class_init(IsManagerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsManagerPrivate));

	gobject_class->get_property = is_manager_get_property;
	gobject_class->set_property = is_manager_set_property;
	gobject_class->dispose = is_manager_dispose;
	gobject_class->finalize = is_manager_finalize;

	properties[PROP_POLL_TIMEOUT] = g_param_spec_uint("poll-timeout",
							  "poll-timeout property",
							  "poll-timeout property blurp.",
							  0, G_MAXUINT,
							  DEFAULT_POLL_TIMEOUT,
							  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_property(gobject_class, PROP_POLL_TIMEOUT,
					properties[PROP_POLL_TIMEOUT]);

	signals[SIGNAL_SENSOR_ADDED] = g_signal_new("sensor-added",
						    G_OBJECT_CLASS_TYPE(klass),
						    G_SIGNAL_RUN_LAST,
						    0,
						    NULL, NULL,
						    g_cclosure_marshal_VOID__OBJECT,
						    G_TYPE_NONE, 1,
						    IS_TYPE_SENSOR);

	signals[SIGNAL_SENSOR_REMOVED] = g_signal_new("sensor-removed",
						      G_OBJECT_CLASS_TYPE(klass),
						      G_SIGNAL_RUN_LAST,
						      0,
						      NULL, NULL,
						      g_cclosure_marshal_VOID__OBJECT,
						      G_TYPE_NONE, 1,
						      IS_TYPE_SENSOR);

	signals[SIGNAL_SENSOR_ENABLED] = g_signal_new("sensor-enabled",
						      G_OBJECT_CLASS_TYPE(klass),
						      G_SIGNAL_RUN_LAST,
						      0,
						      NULL, NULL,
						      g_cclosure_marshal_VOID__OBJECT,
						      G_TYPE_NONE, 1,
						      IS_TYPE_SENSOR);

	signals[SIGNAL_SENSOR_DISABLED] = g_signal_new("sensor-disabled",
						       G_OBJECT_CLASS_TYPE(klass),
						       G_SIGNAL_RUN_LAST,
						       0,
						       NULL, NULL,
						       g_cclosure_marshal_VOID__OBJECT,
						       G_TYPE_NONE, 1,
						       IS_TYPE_SENSOR);

}

static gboolean
process_store(GtkTreeModel *model,
	      GtkTreePath *path,
	      GtkTreeIter *iter,
	      IsManager *self)
{
	IsManagerPrivate *priv;
	gboolean enabled;
	IsSensor *sensor;

	priv = self->priv;

	gtk_tree_model_get(model, iter,
			   IS_STORE_COL_ENABLED, &enabled,
			   -1);

	if (!enabled) {
		goto out;
	}
	gtk_tree_model_get(model, iter,
			   IS_STORE_COL_SENSOR, &sensor,
			   -1);
	if (!sensor) {
		goto out;
	}

	priv->enabled_sensors = g_slist_append(priv->enabled_sensors,
					       g_object_ref(sensor));
out:
	/* always keep iterating */
	return FALSE;
}

static gboolean
update_sensors(IsManager *self)
{
	g_return_if_fail(IS_IS_MANAGER(self));

	g_slist_foreach(self->priv->enabled_sensors,
			(GFunc)is_sensor_update_value,
			self);
	return TRUE;
}

static void
is_manager_init(IsManager *self)
{
	IsManagerPrivate *priv;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	GtkTreeModel *model;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_MANAGER,
					   IsManagerPrivate);

	self->priv = priv;
	priv->store = is_store_new();
	gtk_tree_view_set_model(GTK_TREE_VIEW(self),
				GTK_TREE_MODEL(priv->store));
	priv->poll_timeout = DEFAULT_POLL_TIMEOUT;

	/* id column */
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("ID"),
						       renderer,
						       "text", IS_STORE_COL_ID,
						       NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(self), col);

	col = gtk_tree_view_column_new_with_attributes(_("Label"),
						       renderer,
						       "text", IS_STORE_COL_LABEL,
						       NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(self), col);

	renderer = gtk_cell_renderer_toggle_new();
	col = gtk_tree_view_column_new_with_attributes(_("Enabled"),
						       renderer,
						       "active", IS_STORE_COL_ENABLED,
						       NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(self), col);

}

static void
is_manager_get_property(GObject *object,
			guint property_id, GValue *value, GParamSpec *pspec)
{
	IsManager *self = IS_MANAGER(object);
	IsManagerPrivate *priv = self->priv;

	/* Make compiler happy */
	(void)priv;

	switch (property_id) {
	case PROP_POLL_TIMEOUT:
		g_value_set_uint(value, is_manager_get_poll_timeout(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
is_manager_set_property(GObject *object,
			guint property_id, const GValue *value, GParamSpec *pspec)
{
	IsManager *self = IS_MANAGER(object);
	IsManagerPrivate *priv = self->priv;

	/* Make compiler happy */
	(void)priv;

	switch (property_id) {
	case PROP_POLL_TIMEOUT:
		is_manager_set_poll_timeout(self, g_value_get_uint(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}


static void
is_manager_dispose(GObject *object)
{
	IsManager *self = (IsManager *)object;
	IsManagerPrivate *priv = self->priv;

	if (priv->poll_timeout_id) {
		g_source_remove(priv->poll_timeout_id);
		priv->poll_timeout_id = 0;
	}

	G_OBJECT_CLASS(is_manager_parent_class)->dispose(object);
}

static void
is_manager_finalize(GObject *object)
{
	IsManager *self = (IsManager *)object;
	IsManagerPrivate *priv = self->priv;

	g_slist_foreach(priv->enabled_sensors, (GFunc)g_object_unref, NULL);
	g_slist_free(priv->enabled_sensors);

	G_OBJECT_CLASS(is_manager_parent_class)->finalize(object);
}

IsManager *
is_manager_new(void)
{
	return g_object_new(IS_TYPE_MANAGER, NULL);
}

guint
is_manager_get_poll_timeout(IsManager *self)
{
	g_return_val_if_fail(IS_IS_MANAGER(self), 0);

	return self->priv->poll_timeout;
}

void
is_manager_set_poll_timeout(IsManager *self,
			    guint poll_timeout)
{
	IsManagerPrivate *priv;

	g_return_if_fail(IS_IS_MANAGER(self));

	priv = self->priv;
	if (priv->poll_timeout != poll_timeout) {
		priv->poll_timeout = poll_timeout;
		if (priv->poll_timeout_id) {
			g_source_remove(priv->poll_timeout_id);
			priv->poll_timeout_id = g_timeout_add_seconds(priv->poll_timeout,
								      (GSourceFunc)update_sensors,
								      self);
		}
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_POLL_TIMEOUT]);
	}
}


gboolean
is_manager_add_sensor(IsManager *self,
		      IsSensor *sensor,
		      gboolean enabled)
{
	IsManagerPrivate *priv;

	g_return_val_if_fail(IS_IS_MANAGER(self), FALSE);
	g_return_val_if_fail(IS_IS_SENSOR(sensor), FALSE);

	priv = self->priv;

	return is_store_add_sensor(priv->store, sensor, enabled);
}

gboolean
is_manager_remove_all_sensors(IsManager *self,
			      const gchar *family)
{
	IsManagerPrivate *priv;
	GTree *sensors;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_MANAGER(self), FALSE);
	g_return_val_if_fail(family != NULL, FALSE);

	priv = self->priv;

	ret = is_store_remove_family(priv->store, family);

out:
	return ret;
}
