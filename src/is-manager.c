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
#include "marshallers.h"
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
						      g_cclosure_user_marshal_VOID__OBJECT_INT,
						      G_TYPE_NONE, 2,
						      IS_TYPE_SENSOR,
						      G_TYPE_INT);

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
update_sensors(IsManager *self)
{
	IsManagerPrivate *priv;

	g_return_if_fail(IS_IS_MANAGER(self));

	priv = self->priv;
	g_slist_foreach(priv->enabled_sensors,
			(GFunc)is_sensor_update_value,
			NULL);
	/* only keep going if have sensors to poll */
	if (!priv->enabled_sensors) {
		g_source_remove(priv->poll_timeout_id);
		priv->poll_timeout_id = 0;
	}
	return priv->enabled_sensors != NULL;
}

static void sensor_label_edited(GtkCellRendererText *renderer,
				gchar *path_string,
				gchar *new_label,
				IsManager *self)
{
	IsManagerPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	IsSensor *sensor;

	priv = self->priv;

	path = gtk_tree_path_new_from_string(path_string);
	model = GTK_TREE_MODEL(priv->store);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter,
			   IS_STORE_COL_SENSOR, &sensor,
			   -1);
	is_sensor_set_label(sensor, new_label);
	g_object_unref(sensor);
	gtk_tree_path_free(path);
}

static int
sensor_cmp_by_path(IsSensor *a, IsSensor *b, IsManager *self)
{
	IsManagerPrivate *priv;
	GtkTreeIter a_iter, b_iter;
	GtkTreePath *a_path, *b_path;
	gint ret;

	priv = self->priv;

	is_store_get_iter_for_sensor(priv->store, a, &a_iter);
	is_store_get_iter_for_sensor(priv->store, b, &b_iter);
	a_path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->store), &a_iter);
	b_path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->store), &b_iter);
	ret = gtk_tree_path_compare(a_path, b_path);
	gtk_tree_path_free(b_path);
	gtk_tree_path_free(a_path);
	return ret;
}

static void
enable_sensor(IsManager *self,
	      IsSensor *sensor)
{
	IsManagerPrivate *priv;

	priv = self->priv;
	priv->enabled_sensors = g_slist_insert_sorted_with_data(priv->enabled_sensors,
								sensor,
								(GCompareDataFunc)sensor_cmp_by_path,
								self);

	/* get sensor to update and start polling if not already running */
	is_sensor_update_value(sensor);
	if (!priv->poll_timeout_id) {
		priv->poll_timeout_id = g_timeout_add_seconds(priv->poll_timeout,
							      (GSourceFunc)update_sensors,
							      self);
	}
	g_signal_emit(self, signals[SIGNAL_SENSOR_ENABLED], 0, sensor,
		      g_slist_index(priv->enabled_sensors, sensor));
}

static void
disable_sensor(IsManager *self,
	       IsSensor *sensor)
{
	IsManagerPrivate *priv;

	priv = self->priv;
	priv->enabled_sensors = g_slist_remove(priv->enabled_sensors,
					       sensor);

	if (!priv->enabled_sensors) {
		g_source_remove(priv->poll_timeout_id);
		priv->poll_timeout_id = 0;
	}
	g_signal_emit(self, signals[SIGNAL_SENSOR_DISABLED], 0, sensor);
}

static void sensor_toggled(GtkCellRendererToggle *renderer,
			   gchar *path_string,
			   IsManager *self)
{
	IsManagerPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	IsSensor *sensor;
	gboolean enabled;

	priv = self->priv;

	path = gtk_tree_path_new_from_string(path_string);
	model = GTK_TREE_MODEL(priv->store);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter,
			   IS_STORE_COL_SENSOR, &sensor,
			   -1);
	/* as was toggled need to invert */
	enabled = !gtk_cell_renderer_toggle_get_active(renderer);
	is_store_set_enabled(priv->store, &iter, enabled);
	if (enabled) {
		enable_sensor(self, sensor);
	} else {
		disable_sensor(self, sensor);
	}
	g_object_unref(sensor);
	gtk_tree_path_free(path);
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
	g_signal_connect(renderer, "edited", G_CALLBACK(sensor_label_edited),
			 self);
	gtk_tree_view_append_column(GTK_TREE_VIEW(self), col);

	renderer = gtk_cell_renderer_toggle_new();
	col = gtk_tree_view_column_new_with_attributes(_("Enabled"),
						       renderer,
						       "active", IS_STORE_COL_ENABLED,
						       NULL);
	g_signal_connect(renderer, "toggled", G_CALLBACK(sensor_toggled),
			 self);
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
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_MANAGER(self), FALSE);
	g_return_val_if_fail(IS_IS_SENSOR(sensor), FALSE);

	priv = self->priv;

	ret = is_store_add_sensor(priv->store, sensor, enabled);
	if (!ret) {
		goto out;
	}
	g_signal_emit(self, signals[SIGNAL_SENSOR_ADDED], 0, sensor);
	if (!enabled) {
		goto out;
	}
	enable_sensor(self, sensor);

out:
	return ret;
}

typedef struct _RemoveSensorsWithFamilyData
{
	IsManager *self;
	const gchar *family;
} RemoveSensorsWithFamilyData;

static gboolean
remove_sensors_with_family(GtkTreeModel *model,
			   GtkTreePath *path,
			   GtkTreeIter *iter,
			   RemoveSensorsWithFamilyData *data)
{
	/* see if entry has family, and if so remove it */
	IsManager *self;
	IsManagerPrivate *priv;
	IsSensor *sensor;
	gboolean enabled;

	self = data->self;
	priv = self->priv;
	gtk_tree_model_get(model, iter,
			   IS_STORE_COL_SENSOR, &sensor,
			   IS_STORE_COL_ENABLED, &enabled,
			   -1);
	if (!sensor)
	{
		goto out;
	}
	if (g_strcmp0(is_sensor_get_family(sensor), data->family))
	{
		g_object_unref(sensor);
		goto out;
	}

	if (enabled) {
		disable_sensor(self, sensor);
	}
	is_store_remove_sensor(priv->store, sensor);
	g_signal_emit(self, signals[SIGNAL_SENSOR_REMOVED], 0, sensor);
	g_object_unref(sensor);

out:
	return FALSE;
}

void
is_manager_remove_all_sensors(IsManager *self,
			      const gchar *family)
{
	IsManagerPrivate *priv;
	RemoveSensorsWithFamilyData data;

	g_return_if_fail(IS_IS_MANAGER(self));
	g_return_if_fail(family != NULL);

	priv = self->priv;

	data.self = self;
	data.family = family;
	gtk_tree_model_foreach(GTK_TREE_MODEL(priv->store),
			       (GtkTreeModelForeachFunc)remove_sensors_with_family,
			       &data);
}
