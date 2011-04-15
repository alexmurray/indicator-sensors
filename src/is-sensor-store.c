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

#include "is-sensor-store.h"
#include <gtk/gtk.h>

static void is_sensor_store_dispose(GObject *object);
static void is_sensor_store_finalize(GObject *object);

/* tree model prototypes */
static void is_sensor_store_tree_model_init(GtkTreeModelIface *iface);
static GtkTreeModelFlags is_sensor_store_get_flags(GtkTreeModel *tree_model);
static gint is_sensor_store_get_n_columns(GtkTreeModel *tree_model);
static GType is_sensor_store_get_column_type(GtkTreeModel *tree_model,
					     gint index);
static gboolean is_sensor_store_get_iter(GtkTreeModel *tree_model,
					 GtkTreeIter *iter,
					 GtkTreePath *path);
static GtkTreePath *is_sensor_store_get_path(GtkTreeModel *tree_model,
					     GtkTreeIter *iter);
static void is_sensor_store_get_value(GtkTreeModel *tree_model,
				      GtkTreeIter *iter,
				      gint column,
				      GValue *value);
static gboolean is_sensor_store_iter_next(GtkTreeModel *tree_model,
					  GtkTreeIter *iter);
static gboolean is_sensor_store_iter_children(GtkTreeModel *tree_model,
					      GtkTreeIter *iter,
					      GtkTreeIter *parent);
static gboolean is_sensor_store_iter_has_child(GtkTreeModel *tree_model,
					       GtkTreeIter *iter);
static gint is_sensor_store_iter_n_children(GtkTreeModel *tree_model,
					    GtkTreeIter *iter);
static gboolean is_sensor_store_iter_nth_child(GtkTreeModel *tree_model,
					       GtkTreeIter *iter,
					       GtkTreeIter *parent,
					       gint n);
static gboolean is_sensor_store_iter_parent(GtkTreeModel *tree_model,
					    GtkTreeIter *iter,
					    GtkTreeIter *child);

G_DEFINE_TYPE_EXTENDED(IsSensorStore, is_sensor_store, G_TYPE_OBJECT,
		       0,
		       G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL,
					     is_sensor_store_tree_model_init));

typedef struct _IsSensorStoreFamily IsSensorStoreFamily;
typedef struct _IsSensorStoreEntry IsSensorStoreEntry;

struct _IsSensorStoreEntry
{
	IsSensor *sensor;
	gboolean enabled;

	/* bookkeeping */
	IsSensorStoreFamily *family;
	GSequenceIter *iter;
};

static IsSensorStoreEntry *
entry_new(IsSensor *sensor,
		    IsSensorStoreFamily *family)
{
	IsSensorStoreEntry *entry = g_slice_new0(IsSensorStoreEntry);
	entry->sensor = g_object_ref(sensor);
	entry->family = family;
	return entry;
}

static void
entry_free(IsSensorStoreEntry *entry)
{
	g_object_unref(entry->sensor);
	g_slice_free(IsSensorStoreEntry, entry);
}

struct _IsSensorStoreFamily
{
	/* cache the name */
	gchar *name;
	GSequence *entries;

	/* bookkeeping */
	GSequenceIter *iter;
};

static IsSensorStoreFamily *
family_new(const gchar *name)
{
	IsSensorStoreFamily *family = g_slice_new0(IsSensorStoreFamily);
	family->name = g_strdup(name);
	family->entries = g_sequence_new((GDestroyNotify)entry_free);
	return family;
}

static void
family_free(IsSensorStoreFamily *family)
{
	g_free(family->name);
	g_sequence_free(family->entries);
	g_slice_free(IsSensorStoreFamily, family);
}


struct _IsSensorStorePrivate
{
	GSequence *families;
	gint stamp;
};

static void
is_sensor_store_class_init(IsSensorStoreClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsSensorStorePrivate));

	gobject_class->dispose = is_sensor_store_dispose;
	gobject_class->finalize = is_sensor_store_finalize;
}

static void
is_sensor_store_init(IsSensorStore *self)
{
	IsSensorStorePrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_SENSOR_STORE,
					    IsSensorStorePrivate);

	self->priv = priv;
	priv->families = g_sequence_new((GDestroyNotify)family_free);
	priv->stamp = g_random_int();
}



static void
is_sensor_store_dispose(GObject *object)
{
	IsSensorStore *self = (IsSensorStore *)object;
	IsSensorStorePrivate *priv = self->priv;

	/* Make compiler happy */
	(void)priv;

	G_OBJECT_CLASS(is_sensor_store_parent_class)->dispose(object);
}

static void
is_sensor_store_finalize(GObject *object)
{
	IsSensorStore *self = (IsSensorStore *)object;
	IsSensorStorePrivate *priv = self->priv;

	g_sequence_free(priv->families);

	/* Make compiler happy */
	(void)priv;

	G_OBJECT_CLASS(is_sensor_store_parent_class)->finalize(object);
}

static void is_sensor_store_tree_model_init(GtkTreeModelIface *iface)
{
	iface->get_flags = is_sensor_store_get_flags;
	iface->get_n_columns = is_sensor_store_get_n_columns;
	iface->get_column_type = is_sensor_store_get_column_type;
	iface->get_iter = is_sensor_store_get_iter;
	iface->get_path = is_sensor_store_get_path;
	iface->get_value = is_sensor_store_get_value;
	iface->iter_next = is_sensor_store_iter_next;
	iface->iter_children = is_sensor_store_iter_children;
	iface->iter_has_child = is_sensor_store_iter_has_child;
	iface->iter_n_children = is_sensor_store_iter_n_children;
	iface->iter_nth_child = is_sensor_store_iter_nth_child;
	iface->iter_parent = is_sensor_store_iter_parent;
}

static GtkTreeModelFlags is_sensor_store_get_flags(GtkTreeModel *tree_model)
{
  g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), (GtkTreeModelFlags)0);

  return (GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint is_sensor_store_get_n_columns(GtkTreeModel *tree_model)
{
  g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), 0);

  return IS_SENSOR_STORE_N_COLUMNS;
}

static const GType column_types[IS_SENSOR_STORE_N_COLUMNS] = {
  G_TYPE_STRING, /* IS_SENSOR_STORE_COL_FAMILY */
  G_TYPE_STRING, /* IS_SENSOR_STORE_COL_ID */
  G_TYPE_STRING, /* IS_SENSOR_STORE_COL_LABEL */
  G_TYPE_OBJECT, /* IS_SENSOR_STORE_COL_SENSOR */
  G_TYPE_BOOLEAN, /* IS_SENSOR_STORE_COL_ENABLED */
};

static GType is_sensor_store_get_column_type(GtkTreeModel *tree_model,
                                             gint col)
{
  g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), G_TYPE_INVALID);
  g_return_val_if_fail((col >= 0) &&
		       (col < IS_SENSOR_STORE_N_COLUMNS), G_TYPE_INVALID);

  g_debug("returning type %s for col %d",
	  g_type_name(column_types[col]), col);
  return column_types[col];
}

gboolean is_sensor_store_get_iter(GtkTreeModel *tree_model,
				  GtkTreeIter *iter,
				  GtkTreePath *path)
{
	IsSensorStore *self;
	IsSensorStorePrivate *priv;
	GSequenceIter *family_iter = NULL;
	GSequenceIter *entry_iter = NULL;
	gint *indices, i, depth;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), FALSE);
	g_return_val_if_fail(path != NULL, FALSE);

	self = IS_SENSOR_STORE(tree_model);
	priv = self->priv;

	indices = gtk_tree_path_get_indices(path);
	depth = gtk_tree_path_get_depth(path);

	g_assert(depth <= 2);

	i = indices[0];
	if (i >= 0 && i < g_sequence_get_length(priv->families))
	{
		family_iter = g_sequence_get_iter_at_pos(priv->families,
							 i);
		ret = TRUE;
		if (depth == 2)
		{
			IsSensorStoreFamily *family;
			int j = indices[1];

			family = (IsSensorStoreFamily *)g_sequence_get(family_iter);

			ret = FALSE;
			if (j >= 0 && j < g_sequence_get_length(family->entries))
			{
				entry_iter = g_sequence_get_iter_at_pos(family->entries,
									j);
				ret = TRUE;
			}
		}
	}

	if (ret)
	{
		iter->stamp = priv->stamp;
		iter->user_data = family_iter;
		iter->user_data2 = entry_iter;
		iter->user_data3 = NULL;
	}
	return ret;
}

static GtkTreePath *is_sensor_store_get_path(GtkTreeModel *tree_model,
					     GtkTreeIter *iter)
{
	IsSensorStore *self;
	IsSensorStorePrivate *priv;
	GtkTreePath *path;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), NULL);
	g_return_val_if_fail(iter != NULL, NULL);

	self = IS_SENSOR_STORE(tree_model);
	priv = self->priv;

	g_return_val_if_fail(iter->stamp == priv->stamp, NULL);
	g_assert(iter->user_data);

	path = gtk_tree_path_new();
	gtk_tree_path_append_index(path,
				   g_sequence_iter_get_position((GSequenceIter *)iter->user_data));
	if (iter->user_data2) {
		gtk_tree_path_append_index(path,
					   g_sequence_iter_get_position((GSequenceIter *)iter->user_data2));
	}

	return path;
}

static void is_sensor_store_get_value(GtkTreeModel *tree_model,
				      GtkTreeIter *iter,
				      gint column,
				      GValue *value)
{
	IsSensorStore *self;
	IsSensorStorePrivate *priv;
	IsSensorStoreFamily *family;
	IsSensorStoreEntry *entry = NULL;
	IsSensor *sensor = NULL;

	g_return_if_fail(IS_IS_SENSOR_STORE(tree_model));
	g_return_if_fail(iter != NULL);

	self = IS_SENSOR_STORE(tree_model);
	priv = self->priv;

	g_return_if_fail(iter->stamp == priv->stamp);
	g_assert(iter->user_data);

	g_value_init(value, column_types[column]);

	family = (IsSensorStoreFamily *)
		g_sequence_get((GSequenceIter *)iter->user_data);
	g_assert(family);
	if (iter->user_data2) {
		entry = (IsSensorStoreEntry *)
			g_sequence_get((GSequenceIter *)iter->user_data2);
		g_assert(entry);
		sensor = entry->sensor;
	}

	switch (column) {
	case IS_SENSOR_STORE_COL_FAMILY:
		g_value_set_string(value, sensor ? is_sensor_get_family(sensor) : family->name);
		break;

	case IS_SENSOR_STORE_COL_ID:
		g_value_set_string(value, sensor ? is_sensor_get_id(sensor) : family->name);
		break;

	case IS_SENSOR_STORE_COL_LABEL:
		g_value_set_string(value, sensor ? is_sensor_get_label(sensor) : NULL);
		break;

	case IS_SENSOR_STORE_COL_SENSOR:
		g_value_set_object(value, sensor);
		break;

	case IS_SENSOR_STORE_COL_ENABLED:
		g_value_set_boolean(value, entry ? entry->enabled : FALSE);
		break;

	default:
		g_assert_not_reached();
	}
}

static gboolean is_sensor_store_iter_next(GtkTreeModel *tree_model,
					  GtkTreeIter *iter)
{
	IsSensorStore *self;
	IsSensorStorePrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	self = IS_SENSOR_STORE(tree_model);
	priv = self->priv;

	g_return_val_if_fail(iter->stamp == priv->stamp, FALSE);
	g_assert(iter->user_data);

	if (iter->user_data2) {
		GSequenceIter *next = g_sequence_iter_next((GSequenceIter *)iter->user_data2);
		if (!g_sequence_iter_is_end(next)) {
			iter->user_data2 = next;
			ret = TRUE;
		}
	} else {
		GSequenceIter *next = g_sequence_iter_next((GSequenceIter *)iter->user_data);
		if (!g_sequence_iter_is_end(next)) {
			iter->user_data = next;
			ret = TRUE;
		}
	}
	return ret;
}

static gboolean is_sensor_store_iter_children(GtkTreeModel *tree_model,
					      GtkTreeIter *iter,
					      GtkTreeIter *parent)
{
	IsSensorStore *self;
	IsSensorStorePrivate *priv;
	IsSensorStoreFamily *family;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), FALSE);

	self = IS_SENSOR_STORE(tree_model);
	priv = self->priv;

	/* special case - return first node */
	if (!parent) {
		GSequenceIter *seq_iter =
			g_sequence_get_begin_iter(priv->families);
		if (seq_iter) {
			iter->stamp = priv->stamp;
			iter->user_data = seq_iter;
			iter->user_data3 = iter->user_data2 = NULL;
			ret = TRUE;
		}
		goto out;
	}
	g_return_val_if_fail(parent->stamp == priv->stamp, FALSE);
	g_assert(parent->user_data);

	/* entries have no children so return error */
	if (parent->user_data2) {
		goto out;
	}

	family = (IsSensorStoreFamily *)g_sequence_get(parent->user_data);
	iter->stamp = priv->stamp;
	iter->user_data = parent->user_data;
	iter->user_data2 = g_sequence_get_begin_iter(family->entries);
	iter->user_data3 = NULL;
	ret = TRUE;

out:
	return ret;
}

static gboolean is_sensor_store_iter_has_child(GtkTreeModel *tree_model,
					       GtkTreeIter *iter)
{
	IsSensorStore *self;
	IsSensorStorePrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	self = IS_SENSOR_STORE(tree_model);
	priv = self->priv;

	g_return_val_if_fail(iter->stamp == priv->stamp, FALSE);
	g_assert(iter->user_data);

	/* entries have no children */
	ret = (!iter->user_data2);
	return ret;
}

static gint is_sensor_store_iter_n_children(GtkTreeModel *tree_model,
					    GtkTreeIter *iter)
{
	IsSensorStore *self;
	IsSensorStorePrivate *priv;
	gint n = 0;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), 0);

	self = IS_SENSOR_STORE(tree_model);
	priv = self->priv;

	if (!iter) {
		n = g_sequence_get_length(priv->families);
		goto out;
	}
	g_return_val_if_fail(iter->stamp == priv->stamp, 0);
	g_assert(iter->user_data);

	/* entries have no children */
	if (!iter->user_data2) {
		IsSensorStoreFamily *family = (IsSensorStoreFamily *)
			g_sequence_get(iter->user_data);
		n = g_sequence_get_length(family->entries);
	}

out:
	return n;
}



static gboolean is_sensor_store_iter_nth_child(GtkTreeModel *tree_model,
					       GtkTreeIter *iter,
					       GtkTreeIter *parent,
					       gint n)
{
	IsSensorStore *self;
	IsSensorStorePrivate *priv;
	IsSensorStoreFamily *family;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), FALSE);

	self = IS_SENSOR_STORE(tree_model);
	priv = self->priv;

	if (!parent &&
	    (n >= 0 && n < g_sequence_get_length(priv->families)))
	{
		iter->stamp = priv->stamp;
		iter->user_data = g_sequence_get_iter_at_pos(priv->families,
							     n);
		iter->user_data3 = iter->user_data2 = NULL;
		ret = TRUE;
		goto out;
	}

	g_return_val_if_fail(parent->stamp == priv->stamp, FALSE);
	g_assert(parent->user_data);

	/* entries have no children */
	if (parent->user_data2) {
		goto out;
	}

	family = (IsSensorStoreFamily *)g_sequence_get(parent->user_data);
	iter->stamp = priv->stamp;
	iter->user_data = parent->user_data;
	iter->user_data2 = g_sequence_get_iter_at_pos(family->entries, n);
	iter->user_data3 = NULL;
	ret = TRUE;

out:
	return ret;
}

static gboolean is_sensor_store_iter_parent(GtkTreeModel *tree_model,
					    GtkTreeIter *iter,
					    GtkTreeIter *child)
{
	IsSensorStore *self;
	IsSensorStorePrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(tree_model), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);

	self = IS_SENSOR_STORE(tree_model);
	priv = self->priv;

	g_return_val_if_fail(child->stamp == priv->stamp, FALSE);
	g_assert(child->user_data);
	if (child->user_data2) {
		iter->stamp = child->stamp;
		iter->user_data = child->user_data;
		iter->user_data3 = iter->user_data2 = NULL;
		ret = TRUE;
	}
	return ret;
}

/**
 * Compare two entries - compares on id, but if cmp_data is non-NULL use that
 * instead of first param
 */
static int
entry_cmp(IsSensorStoreEntry *a,
	  IsSensorStoreEntry *b,
	  gpointer cmp_data)
{
	g_assert(a && (b || cmp_data));

	return g_strcmp0(is_sensor_get_id(a->sensor),
			 (cmp_data ? (gchar *)cmp_data : is_sensor_get_id(b->sensor)));
}

static IsSensorStoreEntry *
find_entry(IsSensorStoreFamily *family,
	   const gchar *id)
{
	IsSensorStoreEntry *entry = NULL;

	GSequenceIter *iter = g_sequence_lookup(family->entries,
						(gpointer)id,
						(GCompareDataFunc)entry_cmp,
						(gpointer)id);
	return iter ? (IsSensorStoreEntry *)g_sequence_get(iter) : NULL;
}

/**
 * Compare two families - compares on name, but if cmp_data is non-NULL use that
 * instead of first param
 */
static int
family_cmp(IsSensorStoreFamily *a,
	   IsSensorStoreFamily *b,
	   gpointer cmp_data)
{
	g_assert(a && (b || cmp_data));
	return g_strcmp0(a->name,
			 (cmp_data ? (gchar *)cmp_data : b->name));
}

static IsSensorStoreFamily *
find_family(IsSensorStore *self,
	    const gchar *name)
{
	IsSensorStoreFamily *family = NULL;
	IsSensorStorePrivate *priv = self->priv;

	GSequenceIter *iter = g_sequence_lookup(priv->families,
						(gpointer)name,
						(GCompareDataFunc)family_cmp,
						(gpointer)name);
	return iter ? (IsSensorStoreFamily *)g_sequence_get(iter) : NULL;
}

IsSensorStore *
is_sensor_store_new(void)
{
	return g_object_new(IS_TYPE_SENSOR_STORE, NULL);
}

gboolean
is_sensor_store_add_sensor(IsSensorStore *self,
			   IsSensor *sensor,
			   gboolean enabled)
{
	IsSensorStorePrivate *priv;
	IsSensorStoreFamily *family = NULL;
	IsSensorStoreEntry *entry = NULL;
	GtkTreePath *path;
	GtkTreeIter iter;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(self), FALSE);
	g_return_val_if_fail(IS_IS_SENSOR(sensor), FALSE);

	priv = self->priv;
	family = find_family(self, is_sensor_get_family(sensor));
	if (!family) {
		family = family_new(is_sensor_get_family(sensor));
		family->iter = g_sequence_append(priv->families,
						 family);
		g_debug("IsSensorStore: inserted new family %s at %d",
			is_sensor_get_family(sensor),
			g_sequence_iter_get_position(family->iter));
		iter.stamp = priv->stamp;
		iter.user_data = family->iter;
		iter.user_data2 = NULL;
		iter.user_data3 = NULL;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(self), &iter);
		gtk_tree_model_row_inserted(GTK_TREE_MODEL(self), path, &iter);
		gtk_tree_path_free(path);
	} else {
		entry = find_entry(family, is_sensor_get_id(sensor));
		if (entry) {
			g_warning("sensor [%s]:%s already exists in store, not adding duplicate",
				  is_sensor_get_family(sensor),
				  is_sensor_get_id(sensor));
			goto out;
		}
	}
	entry = entry_new(sensor, family);
	entry->iter = g_sequence_append(family->entries,
					entry);
	entry->enabled = enabled;
	g_debug("IsSensorStore: inserted new entry %s at %d",
		is_sensor_get_id(sensor), g_sequence_iter_get_position(entry->iter));
	iter.stamp = priv->stamp;
	iter.user_data = family->iter;
	iter.user_data2 = entry->iter;
	iter.user_data3 = NULL;
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(self), &iter);
	gtk_tree_model_row_inserted(GTK_TREE_MODEL(self), path, &iter);
	gtk_tree_path_free(path);
	ret = TRUE;

out:
	return TRUE;
}

gboolean
is_sensor_store_remove_family(IsSensorStore *self,
			      const gchar *name)
{
	IsSensorStorePrivate *priv;
	IsSensorStoreFamily *family = NULL;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(self), FALSE);
	g_return_val_if_fail(family != NULL, FALSE);

	priv = self->priv;

	family = find_family(self, name);
	if (family) {
		GtkTreeIter iter;
		GtkTreePath *path;

		iter.stamp = priv->stamp;
		iter.user_data = family->iter;
		iter.user_data3 = iter.user_data2 = NULL;
		gtk_tree_model_get_path(GTK_TREE_MODEL(self), &iter);
		g_sequence_remove(family->iter);
		gtk_tree_model_row_deleted(GTK_TREE_MODEL(self), path);
		gtk_tree_path_free(path);
		ret = TRUE;
	}
	return ret;
}

gboolean
is_sensor_store_set_label(IsSensorStore *self,
			  GtkTreeIter *iter,
			  const gchar *label)
{
	IsSensorStorePrivate *priv;
	IsSensorStoreEntry *entry = NULL;
	gchar *ascii_label1, *ascii_label2;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(self), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = self->priv;

	g_return_val_if_fail(iter->stamp == priv->stamp, FALSE);
	g_return_val_if_fail(iter->user_data && iter->user_data2, FALSE);

	entry = (IsSensorStoreEntry *)g_sequence_get(iter->user_data2);
	if (g_strcmp0(is_sensor_get_label(entry->sensor), label) != 0) {
		GtkTreePath *path;

		is_sensor_set_label(entry->sensor, label);

		path = gtk_tree_model_get_path(GTK_TREE_MODEL(self), iter);
		gtk_tree_model_row_changed(GTK_TREE_MODEL(self), path, iter);
		gtk_tree_path_free(path);

		ret = TRUE;
	}
	return ret;
}

gboolean
is_sensor_store_set_enabled(IsSensorStore *self,
			    GtkTreeIter *iter,
			    gboolean enabled)
{
	IsSensorStorePrivate *priv;
	IsSensorStoreEntry *entry = NULL;
	gboolean ret = FALSE;

	g_return_val_if_fail(IS_IS_SENSOR_STORE(self), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = self->priv;

	g_return_val_if_fail(iter->stamp == priv->stamp, FALSE);
	g_return_val_if_fail(iter->user_data && iter->user_data2, FALSE);

	entry = (IsSensorStoreEntry *)g_sequence_get(iter->user_data2);
	if (entry->enabled != enabled) {
		GtkTreePath *path;

		entry->enabled = enabled;

		path = gtk_tree_model_get_path(GTK_TREE_MODEL(self), iter);
		gtk_tree_model_row_changed(GTK_TREE_MODEL(self), path, iter);
		gtk_tree_path_free(path);

		ret = TRUE;
	}
	return ret;
}
