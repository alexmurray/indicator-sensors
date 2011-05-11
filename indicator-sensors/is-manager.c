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

#include "is-manager.h"
#include "is-store.h"
#include "marshallers.h"
#include "marshallers.c"
#include <glib/gi18n.h>
#include <gio/gio.h>

#define DESKTOP_FILENAME PACKAGE ".desktop"

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
	PROP_ENABLED_SENSORS,
	PROP_AUTOSTART,
	PROP_TEMPERATURE_SCALE,
	LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = {NULL};

struct _IsManagerPrivate
{
	IsStore *store;
	guint poll_timeout;
	glong poll_timeout_id;
	GTree *enabled_paths;
	GSList *enabled_list;
	GFileMonitor *monitor;
	IsTemperatureSensorScale temperature_scale;
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

	properties[PROP_ENABLED_SENSORS] = g_param_spec_boxed("enabled-sensors",
							      "enabled-sensors property",
							      "enabled-sensors property blurp.",
							      G_TYPE_STRV,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_property(gobject_class, PROP_ENABLED_SENSORS,
					properties[PROP_ENABLED_SENSORS]);

	properties[PROP_AUTOSTART] = g_param_spec_boolean("autostart",
							  "autostart property",
							  "start " PACKAGE " automatically on login.",
							  FALSE,
							  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_property(gobject_class, PROP_AUTOSTART,
					properties[PROP_AUTOSTART]);

	/* TODO: convert to an enum type */
	properties[PROP_TEMPERATURE_SCALE] = g_param_spec_int("temperature-scale", "temperature scale",
							      "Sensor temperature scale.",
							      IS_TEMPERATURE_SENSOR_SCALE_CELSIUS,
							      IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT,
							      IS_TEMPERATURE_SENSOR_SCALE_CELSIUS,
							      G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_TEMPERATURE_SCALE,
					properties[PROP_TEMPERATURE_SCALE]);

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

	g_return_val_if_fail(IS_IS_MANAGER(self), FALSE);

	priv = self->priv;
	g_slist_foreach(priv->enabled_list,
			(GFunc)is_sensor_update_value,
			NULL);
	/* only keep going if have sensors to poll */
	if (!priv->enabled_list) {
		g_source_remove(priv->poll_timeout_id);
		priv->poll_timeout_id = 0;
	}
	return priv->enabled_list != NULL;
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
	      GtkTreeIter *iter,
	      IsSensor *sensor)
{
	IsManagerPrivate *priv;

	priv = self->priv;

	is_store_set_enabled(priv->store, iter, TRUE);
	priv->enabled_list = g_slist_insert_sorted_with_data(priv->enabled_list,
							     sensor,
							     (GCompareDataFunc)sensor_cmp_by_path,
							     self);
	g_signal_emit(self, signals[SIGNAL_SENSOR_ENABLED], 0, sensor,
		      g_slist_index(priv->enabled_list, sensor));
	if (!g_tree_lookup(priv->enabled_paths, is_sensor_get_path(sensor))) {
		gchar *path = g_strdup(is_sensor_get_path(sensor));
		g_tree_insert(priv->enabled_paths, path, path);
		g_object_notify_by_pspec(G_OBJECT(self),
					 properties[PROP_ENABLED_SENSORS]);
	}
	/* get sensor to update and start polling if not already running */
	is_sensor_update_value(sensor);
	if (!priv->poll_timeout_id) {
		priv->poll_timeout_id = g_timeout_add_seconds(priv->poll_timeout,
							      (GSourceFunc)update_sensors,
							      self);
	}
}

static void
disable_sensor(IsManager *self,
	       GtkTreeIter *iter,
	       IsSensor *sensor)
{
	gboolean ret;
	IsManagerPrivate *priv;

	priv = self->priv;

	is_store_set_enabled(priv->store, iter, FALSE);
	priv->enabled_list = g_slist_remove(priv->enabled_list,
					    sensor);
	g_signal_emit(self, signals[SIGNAL_SENSOR_DISABLED], 0, sensor);

	ret = g_tree_remove(priv->enabled_paths, is_sensor_get_path(sensor));
	g_assert(ret);
	g_object_notify_by_pspec(G_OBJECT(self),
				 properties[PROP_ENABLED_SENSORS]);
	if (!priv->enabled_list) {
		g_source_remove(priv->poll_timeout_id);
		priv->poll_timeout_id = 0;
	}
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
	if (sensor) {
		/* as was toggled need to invert */
		enabled = !gtk_cell_renderer_toggle_get_active(renderer);
		if (enabled) {
			enable_sensor(self, &iter, sensor);
		} else {
			disable_sensor(self, &iter, sensor);
		}
		g_object_unref(sensor);
	}
	gtk_tree_path_free(path);
}

static void
file_monitor_changed(GFileMonitor *monitor,
		     GFile *file,
		     GFile *other_file,
		     GFileMonitorEvent event_type,
		     IsManager *self)
{
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_AUTOSTART]);
}

static void
is_manager_init(IsManager *self)
{
	IsManagerPrivate *priv;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	gchar *path;
	GFile *file;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_MANAGER,
					   IsManagerPrivate);

	self->priv = priv;
	priv->enabled_paths = g_tree_new_full((GCompareDataFunc)g_strcmp0, NULL,
					      g_free, NULL);
	priv->store = is_store_new();
	gtk_tree_view_set_model(GTK_TREE_VIEW(self),
				GTK_TREE_MODEL(priv->store));
	priv->poll_timeout = DEFAULT_POLL_TIMEOUT;
	path = g_build_filename(g_get_user_config_dir(), "autostart",
				DESKTOP_FILENAME, NULL);
	file = g_file_new_for_path(path);
	priv->monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE,
					    NULL, NULL);
	g_signal_connect(priv->monitor, "changed",
			 G_CALLBACK(file_monitor_changed), self);
	g_object_unref(file);
	g_free(path);
	priv->temperature_scale = IS_TEMPERATURE_SENSOR_SCALE_CELSIUS;

	/* id column */
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Sensor"),
						       renderer,
						       "text", IS_STORE_COL_NAME,
						       NULL);
	gtk_tree_view_column_set_expand(col, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(self), col);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	col = gtk_tree_view_column_new_with_attributes(_("Label"),
						       renderer,
						       "text", IS_STORE_COL_LABEL,
						       NULL);
	gtk_tree_view_column_set_expand(col, TRUE);
	g_signal_connect(renderer, "edited", G_CALLBACK(sensor_label_edited),
			 self);
	gtk_tree_view_append_column(GTK_TREE_VIEW(self), col);

	renderer = gtk_cell_renderer_toggle_new();
	col = gtk_tree_view_column_new_with_attributes(_("Enabled"),
						       renderer,
						       "active", IS_STORE_COL_ENABLED,
						       NULL);
	gtk_tree_view_column_set_expand(col, FALSE);
	g_signal_connect(renderer, "toggled", G_CALLBACK(sensor_toggled),
			 self);
	gtk_tree_view_append_column(GTK_TREE_VIEW(self), col);
}

static void
is_manager_get_property(GObject *object,
			guint property_id, GValue *value, GParamSpec *pspec)
{
	IsManager *self = IS_MANAGER(object);

	switch (property_id) {
	case PROP_POLL_TIMEOUT:
		g_value_set_uint(value, is_manager_get_poll_timeout(self));
		break;
	case PROP_ENABLED_SENSORS:
		g_value_take_boxed(value, is_manager_get_enabled_sensors(self));
		break;
	case PROP_AUTOSTART:
		g_value_set_boolean(value, is_manager_get_autostart(self));
		break;
	case PROP_TEMPERATURE_SCALE:
		g_value_set_int(value, is_manager_get_temperature_scale(self));
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

	switch (property_id) {
	case PROP_POLL_TIMEOUT:
		is_manager_set_poll_timeout(self, g_value_get_uint(value));
		break;
	case PROP_ENABLED_SENSORS:
		is_manager_set_enabled_sensors(self,
					       (const gchar **)g_value_get_boxed(value));
		break;
	case PROP_AUTOSTART:
		is_manager_set_autostart(self, g_value_get_boolean(value));
		break;
	case PROP_TEMPERATURE_SCALE:
		is_manager_set_temperature_scale(self,
						 g_value_get_int(value));
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
	g_object_unref(priv->monitor);

	G_OBJECT_CLASS(is_manager_parent_class)->dispose(object);
}

static void
is_manager_finalize(GObject *object)
{
	IsManager *self = (IsManager *)object;
	IsManagerPrivate *priv = self->priv;

	g_tree_unref(priv->enabled_paths);
	g_slist_free(priv->enabled_list);

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
		      IsSensor *sensor)
{
	IsManagerPrivate *priv;
	GtkTreeIter iter;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_MANAGER(self), FALSE);
	g_return_val_if_fail(IS_IS_SENSOR(sensor), FALSE);

	priv = self->priv;

	ret = is_store_add_sensor(priv->store, sensor, &iter);
	if (!ret) {
		goto out;
	}
	/* set scale as appropriate */
	if (IS_IS_TEMPERATURE_SENSOR(sensor)) {
		is_temperature_sensor_set_scale(IS_TEMPERATURE_SENSOR(sensor),
						priv->temperature_scale);
	}
	g_signal_emit(self, signals[SIGNAL_SENSOR_ADDED], 0, sensor);
	/* enable sensor if is in enabled-sensors list */
	if (g_tree_lookup(priv->enabled_paths, is_sensor_get_path(sensor))) {
		enable_sensor(self, &iter, sensor);
	}

out:
	return ret;
}

GSList *
is_manager_get_enabled_sensors_list(IsManager *self)
{
	IsManagerPrivate *priv;
	GSList *_list, *list = NULL;

	g_return_val_if_fail(IS_IS_MANAGER(self), NULL);
	priv = self->priv;

	for (_list = priv->enabled_list;
	     _list != NULL;
	     _list = _list->next) {
		list = g_slist_prepend(list, g_object_ref(_list->data));
	}
	list = g_slist_reverse(list);
	return list;
}

gboolean
is_manager_set_enabled_sensors(IsManager *self,
			       const gchar **enabled_sensors)
{
	IsManagerPrivate *priv;
	int i, n;
	GSList *list;
	GTree *tree;

	g_return_val_if_fail(IS_IS_MANAGER(self), FALSE);

	priv = self->priv;

	tree = g_tree_new_full((GCompareDataFunc)g_strcmp0, NULL, g_free, NULL);

	/* copy this list as new tree of enabled sensors */
	n = g_strv_length((gchar **)enabled_sensors);

	/* copy list and also enable all sensors in the list */
	for (i = 0; i < n; i++) {
		gchar *path;
		GtkTreeIter iter;

		path = g_strdup(enabled_sensors[i]);
		g_tree_insert(tree, path, path);

		if (is_store_get_iter(priv->store, path, &iter)) {
			IsSensor *sensor;

			gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
					   IS_STORE_COL_SENSOR, &sensor,
					   -1);
			enable_sensor(self, &iter, sensor);
		}
	}

	for (list = priv->enabled_list; list != NULL; list = list->next)
	{
		IsSensor *sensor = (IsSensor *)list->data;
		const gchar *path;

		path = is_sensor_get_path(sensor);
		if (!g_tree_lookup(tree, path)) {
			GtkTreeIter iter;
			is_store_get_iter(priv->store, path, &iter);
			disable_sensor(self, &iter, sensor);
		}
	}
	g_tree_destroy(priv->enabled_paths);
	priv->enabled_paths = tree;
	g_object_notify_by_pspec(G_OBJECT(self),
				 properties[PROP_ENABLED_SENSORS]);
	return TRUE;
}

static gboolean
add_key_to_array(const gchar *key,
		 const gchar *value,
		 GArray *array)
{
	gchar *path = g_strdup(key);
	g_array_append_val(array, path);
	/* keep going */
	return FALSE;
}


gchar **
is_manager_get_enabled_sensors(IsManager *self)
{
	IsManagerPrivate *priv;
	GArray *array;

	g_return_val_if_fail(IS_IS_MANAGER(self), NULL);

	priv = self->priv;

	array = g_array_new(TRUE, FALSE, sizeof(gchar *));

	g_tree_foreach(priv->enabled_paths,
		       (GTraverseFunc)add_key_to_array, array);

	/* if not freeing element data g_array_free() returns the element data
	   which is exactly what we want */
	return (gchar **)g_array_free(array, FALSE);
}

static gboolean
add_sensor_to_list(GtkTreeModel *model,
		   GtkTreePath *path,
		   GtkTreeIter *iter,
		   GSList **list)
{
	IsSensor *sensor;

	gtk_tree_model_get(model, iter,
			   IS_STORE_COL_SENSOR, &sensor,
			   -1);

	if (sensor) {
		/* take reference */
		*list = g_slist_prepend(*list, sensor);
	}
	return FALSE;
}

GSList *
is_manager_get_all_sensors_list(IsManager *self)
{
	IsManagerPrivate *priv;
	GSList *list = NULL;

	g_return_val_if_fail(IS_IS_MANAGER(self), NULL);
	priv = self->priv;

	gtk_tree_model_foreach(GTK_TREE_MODEL(priv->store),
			       (GtkTreeModelForeachFunc)add_sensor_to_list,
			       &list);
	list = g_slist_reverse(list);

	return list;
}

IsSensor *
is_manager_get_sensor(IsManager *self,
		      const gchar *path)
{
	IsManagerPrivate *priv;
	gboolean ret;
	GtkTreeIter iter;
	IsSensor *sensor = NULL;

	g_return_val_if_fail(IS_IS_MANAGER(self), NULL);

	priv = self->priv;

	ret = is_store_get_iter(priv->store, path, &iter);
	if (ret) {
		gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
				   IS_STORE_COL_SENSOR, &sensor,
				   -1);
	}

	return sensor;
}

#define AUTOSTART_KEY "X-GNOME-Autostart-enabled"

gboolean
is_manager_get_autostart(IsManager *self)
{
	GKeyFile *key_file;
	gchar *path;
	GError *error = NULL;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_MANAGER(self), FALSE);

	key_file = g_key_file_new();
	path = g_build_filename(g_get_user_config_dir(), "autostart",
				DESKTOP_FILENAME, NULL);
	ret = g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, &error);
	if (!ret) {
		g_debug("Failed to load autostart desktop file '%s': %s",
			path, error->message);
		g_error_free(error);
		goto out;
	}
	ret = g_key_file_get_boolean(key_file, G_KEY_FILE_DESKTOP_GROUP,
				     AUTOSTART_KEY, &error);
	if (error) {
		g_debug("Failed to get key '%s' from autostart desktop file '%s': %s",
			AUTOSTART_KEY, path, error->message);
		g_error_free(error);
	}

out:
	g_free(path);
	g_key_file_free(key_file);
	return ret;
}

void
is_manager_set_autostart(IsManager *self,
			 gboolean autostart)
{
	GKeyFile *key_file;
	gboolean ret;
	gchar *autostart_file;
	GError *error = NULL;

	g_return_if_fail(IS_IS_MANAGER(self));

	key_file = g_key_file_new();
	autostart_file = g_build_filename(g_get_user_config_dir(), "autostart",
					  DESKTOP_FILENAME, NULL);
	ret = g_key_file_load_from_file(key_file, autostart_file,
					G_KEY_FILE_KEEP_COMMENTS |
					G_KEY_FILE_KEEP_TRANSLATIONS,
					&error);
	if (!ret) {
		gchar *application_file;

		g_debug("Failed to load autostart desktop file '%s': %s",
			autostart_file, error->message);
		g_clear_error(&error);
		application_file = g_build_filename("applications",
						    DESKTOP_FILENAME, NULL);
		ret = g_key_file_load_from_data_dirs(key_file,
						     application_file,
						     NULL,
						     G_KEY_FILE_KEEP_COMMENTS |
						     G_KEY_FILE_KEEP_TRANSLATIONS,
						     &error);
		if (!ret) {
			g_warning("Failed to load application desktop file: %s",
				  error->message);
			g_clear_error(&error);
			/* create file by hand */
			g_key_file_set_string(key_file,
					      G_KEY_FILE_DESKTOP_GROUP,
					      G_KEY_FILE_DESKTOP_KEY_TYPE,
					      "Application");
			g_key_file_set_string(key_file,
					      G_KEY_FILE_DESKTOP_GROUP,
					      G_KEY_FILE_DESKTOP_KEY_NAME,
					      _(PACKAGE_NAME));
			g_key_file_set_string(key_file,
					      G_KEY_FILE_DESKTOP_GROUP,
					      G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME,
					      _(PACKAGE_NAME));
 			g_key_file_set_string(key_file,
					      G_KEY_FILE_DESKTOP_GROUP,
					      G_KEY_FILE_DESKTOP_KEY_EXEC,
					      PACKAGE);
 			g_key_file_set_string(key_file,
					      G_KEY_FILE_DESKTOP_GROUP,
					      G_KEY_FILE_DESKTOP_KEY_ICON,
					      PACKAGE);
 			g_key_file_set_string(key_file,
					      G_KEY_FILE_DESKTOP_GROUP,
					      G_KEY_FILE_DESKTOP_KEY_CATEGORIES,
					      "System;");
                }
		g_free(application_file);
	}
	g_key_file_set_boolean(key_file, G_KEY_FILE_DESKTOP_GROUP,
			       AUTOSTART_KEY, autostart);
	ret = g_file_set_contents(autostart_file,
				  g_key_file_to_data(key_file, NULL, NULL),
				  -1,
				  &error);
	if (!ret) {
		g_warning("Failed to write autostart desktop file '%s': %s",
			  autostart_file, error->message);
		g_clear_error(&error);
	}
	g_free(autostart_file);
	g_key_file_free(key_file);
}

IsTemperatureSensorScale is_manager_get_temperature_scale(IsManager *self)
{
	g_return_val_if_fail(IS_IS_MANAGER(self),
			     IS_TEMPERATURE_SENSOR_SCALE_INVALID);

	return self->priv->temperature_scale;
}

static gboolean
set_temperature_sensor_scale(GtkTreeModel *model,
			     GtkTreePath *path,
			     GtkTreeIter *iter,
			     IsTemperatureSensorScale *scale)
{
	IsSensor *sensor;

	gtk_tree_model_get(model, iter,
			   IS_STORE_COL_SENSOR, &sensor,
			   -1);

	if (sensor) {
		if (IS_IS_TEMPERATURE_SENSOR(sensor)) {
			is_temperature_sensor_set_scale(IS_TEMPERATURE_SENSOR(sensor),
							*scale);
		}
		g_object_unref(sensor);
	}
	return FALSE;
}

void is_manager_set_temperature_scale(IsManager *self,
				      IsTemperatureSensorScale scale)
{
	IsManagerPrivate *priv;

	g_return_if_fail(IS_IS_MANAGER(self));
	g_return_if_fail(scale == IS_TEMPERATURE_SENSOR_SCALE_CELSIUS ||
			 scale == IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT);

	priv = self->priv;

	/* set scale on all temperature sensors */
	if (priv->temperature_scale != scale) {
		priv->temperature_scale = scale;
		gtk_tree_model_foreach(GTK_TREE_MODEL(priv->store),
				       (GtkTreeModelForeachFunc)set_temperature_sensor_scale,
				       &priv->temperature_scale);
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_TEMPERATURE_SCALE]);
	}
}


