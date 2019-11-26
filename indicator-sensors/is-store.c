/*
 * Copyright (C) 2011-2019 Alex Murray <murray.alex@gmail.com>
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

#include "is-store.h"
#include "is-log.h"
#include <gtk/gtk.h>

static void is_store_dispose(GObject *object);
static void is_store_finalize(GObject *object);

/* tree model prototypes */
static void is_store_tree_model_init(GtkTreeModelIface *iface);
static GtkTreeModelFlags _is_store_get_flags(GtkTreeModel *tree_model);
static gint _is_store_get_n_columns(GtkTreeModel *tree_model);
static GType _is_store_get_column_type(GtkTreeModel *tree_model,
                                       gint index);
static gboolean _is_store_get_iter(GtkTreeModel *tree_model,
                                   GtkTreeIter *iter,
                                   GtkTreePath *path);
static GtkTreePath *_is_store_get_path(GtkTreeModel *tree_model,
                                       GtkTreeIter *iter);
static void _is_store_get_value(GtkTreeModel *tree_model,
                                GtkTreeIter *iter,
                                gint column,
                                GValue *value);
static gboolean _is_store_iter_next(GtkTreeModel *tree_model,
                                    GtkTreeIter *iter);
static gboolean _is_store_iter_children(GtkTreeModel *tree_model,
                                        GtkTreeIter *iter,
                                        GtkTreeIter *parent);
static gboolean _is_store_iter_has_child(GtkTreeModel *tree_model,
    GtkTreeIter *iter);
static gint _is_store_iter_n_children(GtkTreeModel *tree_model,
                                      GtkTreeIter *iter);
static gboolean _is_store_iter_nth_child(GtkTreeModel *tree_model,
    GtkTreeIter *iter,
    GtkTreeIter *parent,
    gint n);
static gboolean _is_store_iter_parent(GtkTreeModel *tree_model,
                                      GtkTreeIter *iter,
                                      GtkTreeIter *child);

G_DEFINE_TYPE_EXTENDED(IsStore, is_store, G_TYPE_OBJECT,
                       0,
                       G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL,
                           is_store_tree_model_init));

typedef struct _IsStoreEntry IsStoreEntry;

struct _IsStoreEntry
{
  /* entry a path name and other entries, and a
     sensor + enabled value if it is a leaf sensor */
  gchar *name;

  GSequence *entries;

  IsSensor *sensor;
  gboolean enabled;

  /* bookkeeping */
  GSequenceIter *parent;
  GSequenceIter *iter;
};

static void
entry_free(IsStoreEntry *entry)
{
  if (entry->sensor)
  {
    g_object_unref(entry->sensor);
  }
  g_sequence_free(entry->entries);
  g_free(entry->name);
  g_slice_free(IsStoreEntry, entry);
}

static IsStoreEntry *
entry_new(const gchar *name)
{
  IsStoreEntry *entry = g_slice_new0(IsStoreEntry);
  entry->name = g_strdup(name);
  entry->entries = g_sequence_new((GDestroyNotify)entry_free);
  return entry;
}

struct _IsStorePrivate
{
  GSequence *entries;
  gint stamp;
};

static void
is_store_class_init(IsStoreClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass, sizeof(IsStorePrivate));

  gobject_class->dispose = is_store_dispose;
  gobject_class->finalize = is_store_finalize;
}

static void
is_store_init(IsStore *self)
{
  IsStorePrivate *priv =
    G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_STORE,
                                IsStorePrivate);

  self->priv = priv;
  priv->entries = g_sequence_new((GDestroyNotify)entry_free);
  priv->stamp = g_random_int();
}



static void
is_store_dispose(GObject *object)
{
  IsStore *self = (IsStore *)object;
  IsStorePrivate *priv = self->priv;

  /* Make compiler happy */
  (void)priv;

  G_OBJECT_CLASS(is_store_parent_class)->dispose(object);
}

static void
is_store_finalize(GObject *object)
{
  IsStore *self = (IsStore *)object;
  IsStorePrivate *priv = self->priv;

  g_sequence_free(priv->entries);

  /* Make compiler happy */
  (void)priv;

  G_OBJECT_CLASS(is_store_parent_class)->finalize(object);
}

static void is_store_tree_model_init(GtkTreeModelIface *iface)
{
  iface->get_flags = _is_store_get_flags;
  iface->get_n_columns = _is_store_get_n_columns;
  iface->get_column_type = _is_store_get_column_type;
  iface->get_iter = _is_store_get_iter;
  iface->get_path = _is_store_get_path;
  iface->get_value = _is_store_get_value;
  iface->iter_next = _is_store_iter_next;
  iface->iter_children = _is_store_iter_children;
  iface->iter_has_child = _is_store_iter_has_child;
  iface->iter_n_children = _is_store_iter_n_children;
  iface->iter_nth_child = _is_store_iter_nth_child;
  iface->iter_parent = _is_store_iter_parent;
}

static GtkTreeModelFlags _is_store_get_flags(GtkTreeModel *tree_model)
{
  g_return_val_if_fail(IS_IS_STORE(tree_model), (GtkTreeModelFlags)0);

  return (GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint _is_store_get_n_columns(GtkTreeModel *tree_model)
{
  g_return_val_if_fail(IS_IS_STORE(tree_model), 0);

  return IS_STORE_N_COLUMNS;
}

static const GType column_types[IS_STORE_N_COLUMNS] =
{
  G_TYPE_STRING, /* IS_STORE_COL_NAME */
  G_TYPE_STRING, /* IS_STORE_COL_LABEL */
  G_TYPE_STRING, /* IS_STORE_COL_ICON */
  G_TYPE_BOOLEAN, /* IS_STORE_COL_IS_SENSOR */
  G_TYPE_OBJECT, /* IS_STORE_COL_SENSOR */
  G_TYPE_BOOLEAN, /* IS_STORE_COL_ENABLED */
};

static GType _is_store_get_column_type(GtkTreeModel *tree_model,
                                       gint col)
{
  g_return_val_if_fail(IS_IS_STORE(tree_model), G_TYPE_INVALID);
  g_return_val_if_fail((col >= 0) &&
                       (col < IS_STORE_N_COLUMNS), G_TYPE_INVALID);

  return column_types[col];
}

static gboolean
get_iter_for_indices(IsStore *self,
                     const gint *indices,
                     const gint depth,
                     gint i,
                     GSequence *entries,
                     GtkTreeIter *iter)
{
  gint index;
  GSequenceIter *entry_iter;
  IsStoreEntry *entry;
  gboolean ret = FALSE;

  g_assert(i < depth);

  index = indices[i];
  if (index < 0 || index >= g_sequence_get_length(entries))
  {
    goto out;
  }
  entry_iter = g_sequence_get_iter_at_pos(entries,
                                          index);
  entry = (IsStoreEntry *) g_sequence_get(entry_iter);
  /* if this is the required depth return this iter */
  if (i == depth - 1)
  {
    iter->stamp = self->priv->stamp;
    iter->user_data = entry->iter;
    ret = TRUE;
  }
  else
  {
    /* parent shouldn't have a sensor */
    g_assert(!entry->sensor);
    ret = get_iter_for_indices(self, indices, depth, ++i,
                               entry->entries, iter);
  }

out:
  return ret;
}

gboolean _is_store_get_iter(GtkTreeModel *tree_model,
                            GtkTreeIter *iter,
                            GtkTreePath *path)
{
  IsStore *self;
  IsStorePrivate *priv;
  gint *indices, depth;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(tree_model), FALSE);
  g_return_val_if_fail(path != NULL, FALSE);

  self = IS_STORE(tree_model);
  priv = self->priv;

  indices = gtk_tree_path_get_indices(path);
  depth = gtk_tree_path_get_depth(path);

  ret = get_iter_for_indices(self, indices, depth, 0, priv->entries, iter);
  return ret;
}

static GtkTreePath *_is_store_get_path(GtkTreeModel *tree_model,
                                       GtkTreeIter *iter)
{
  IsStore *self;
  IsStorePrivate *priv;
  GtkTreePath *path;
  GSequenceIter *entry_iter;

  g_return_val_if_fail(IS_IS_STORE(tree_model), NULL);
  g_return_val_if_fail(iter != NULL, NULL);

  self = IS_STORE(tree_model);
  priv = self->priv;

  g_return_val_if_fail(iter->stamp == priv->stamp, NULL);
  g_assert(iter->user_data);

  path = gtk_tree_path_new();
  entry_iter = (GSequenceIter *)iter->user_data;

  while (entry_iter != NULL)
  {
    IsStoreEntry *entry;

    gtk_tree_path_prepend_index(path,
                                g_sequence_iter_get_position(entry_iter));
    entry = (IsStoreEntry *)g_sequence_get(entry_iter);
    entry_iter = entry->parent;
  }

  return path;
}

static void _is_store_get_value(GtkTreeModel *tree_model,
                                GtkTreeIter *iter,
                                gint column,
                                GValue *value)
{
  IsStore *self;
  IsStorePrivate *priv;
  IsStoreEntry *entry = NULL;

  g_return_if_fail(IS_IS_STORE(tree_model));
  g_return_if_fail(iter != NULL);

  self = IS_STORE(tree_model);
  priv = self->priv;

  g_return_if_fail(iter->stamp == priv->stamp);
  g_assert(iter->user_data);

  g_value_init(value, column_types[column]);

  entry = (IsStoreEntry *)
          g_sequence_get((GSequenceIter *)iter->user_data);
  g_assert(entry);

  switch (column)
  {
    case IS_STORE_COL_NAME:
      g_value_set_string(value, entry->name);
      break;

    case IS_STORE_COL_LABEL:
      g_value_set_string(value, (entry->sensor ?
                                 is_sensor_get_label(entry->sensor) :
                                 NULL));
      break;

    case IS_STORE_COL_IS_SENSOR:
      g_value_set_boolean(value, (entry->sensor != NULL));
      break;

    case IS_STORE_COL_SENSOR:
      g_value_set_object(value, entry->sensor);
      break;

    case IS_STORE_COL_ENABLED:
      g_value_set_boolean(value, entry->enabled);
      break;

    case IS_STORE_COL_ICON:
      g_value_set_string(value, (entry->sensor ?
                                 is_sensor_get_icon(entry->sensor) :
                                 NULL));
      break;

    default:
      g_assert_not_reached();
  }
}

static gboolean _is_store_iter_next(GtkTreeModel *tree_model,
                                    GtkTreeIter *iter)
{
  IsStore *self;
  IsStorePrivate *priv;
  GSequenceIter *next;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(tree_model), FALSE);
  g_return_val_if_fail(iter != NULL, FALSE);

  self = IS_STORE(tree_model);
  priv = self->priv;

  g_return_val_if_fail(iter->stamp == priv->stamp, FALSE);
  g_assert(iter->user_data);

  next = g_sequence_iter_next((GSequenceIter *)iter->user_data);
  if (!g_sequence_iter_is_end(next))
  {
    iter->user_data = next;
    ret = TRUE;
  }
  return ret;
}

static gboolean _is_store_iter_children(GtkTreeModel *tree_model,
                                        GtkTreeIter *iter,
                                        GtkTreeIter *parent)
{
  IsStore *self;
  IsStorePrivate *priv;
  IsStoreEntry *entry;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(tree_model), FALSE);

  self = IS_STORE(tree_model);
  priv = self->priv;

  /* special case - return first node */
  if (!parent)
  {
    GSequenceIter *seq_iter =
      g_sequence_get_begin_iter(priv->entries);
    if (seq_iter)
    {
      iter->stamp = priv->stamp;
      iter->user_data = seq_iter;
      ret = TRUE;
    }
    goto out;
  }
  g_return_val_if_fail(parent->stamp == priv->stamp, FALSE);
  g_assert(parent->user_data);

  entry = (IsStoreEntry *)g_sequence_get(parent->user_data);
  if (!entry->sensor)
  {
    iter->stamp = priv->stamp;
    iter->user_data = g_sequence_get_begin_iter(entry->entries);
    ret = TRUE;
  }

out:
  return ret;
}

static gboolean _is_store_iter_has_child(GtkTreeModel *tree_model,
    GtkTreeIter *iter)
{
  IsStore *self;
  IsStorePrivate *priv;
  IsStoreEntry *entry;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(tree_model), FALSE);
  g_return_val_if_fail(iter != NULL, FALSE);

  self = IS_STORE(tree_model);
  priv = self->priv;

  g_return_val_if_fail(iter->stamp == priv->stamp, FALSE);
  g_assert(iter->user_data);

  /* end iter is invalid and has no entry associated with it */
  if (!g_sequence_iter_is_end((GSequenceIter *)iter->user_data))
  {
    entry = (IsStoreEntry *)
            g_sequence_get((GSequenceIter *)iter->user_data);

    ret = (g_sequence_get_length(entry->entries) > 0);
  }

  return ret;
}

static gint _is_store_iter_n_children(GtkTreeModel *tree_model,
                                      GtkTreeIter *iter)
{
  IsStore *self;
  IsStorePrivate *priv;
  IsStoreEntry *entry;
  gint n;

  g_return_val_if_fail(IS_IS_STORE(tree_model), 0);

  self = IS_STORE(tree_model);
  priv = self->priv;

  if (!iter)
  {
    n = g_sequence_get_length(priv->entries);
    goto out;
  }
  g_return_val_if_fail(iter->stamp == priv->stamp, 0);
  g_assert(iter->user_data);

  entry = (IsStoreEntry *)
          g_sequence_get((GSequenceIter *)iter->user_data);
  n = g_sequence_get_length(entry->entries);

out:
  return n;
}



static gboolean _is_store_iter_nth_child(GtkTreeModel *tree_model,
    GtkTreeIter *iter,
    GtkTreeIter *parent,
    gint n)
{
  IsStore *self;
  IsStorePrivate *priv;
  IsStoreEntry *entry;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(tree_model), FALSE);

  self = IS_STORE(tree_model);
  priv = self->priv;

  if (!parent)
  {
    g_return_val_if_fail(n >= 0 && n < g_sequence_get_length(priv->entries),
                         FALSE);
    iter->stamp = priv->stamp;
    iter->user_data = g_sequence_get_iter_at_pos(priv->entries,
                      n);
    ret = TRUE;
    goto out;
  }

  g_return_val_if_fail(parent->stamp == priv->stamp, FALSE);
  g_assert(parent->user_data);

  entry = (IsStoreEntry *)
          g_sequence_get((GSequenceIter *)parent->user_data);
  if (entry->entries)
  {
    iter->stamp = priv->stamp;
    iter->user_data = g_sequence_get_iter_at_pos(entry->entries, n);
    ret = TRUE;
  }

out:
  return ret;
}

static gboolean _is_store_iter_parent(GtkTreeModel *tree_model,
                                      GtkTreeIter *iter,
                                      GtkTreeIter *child)
{
  IsStore *self;
  IsStorePrivate *priv;
  IsStoreEntry *entry;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(tree_model), FALSE);
  g_return_val_if_fail(child != NULL, FALSE);

  self = IS_STORE(tree_model);
  priv = self->priv;

  g_return_val_if_fail(child->stamp == priv->stamp, FALSE);
  g_assert(child->user_data);
  entry = (IsStoreEntry *)
          g_sequence_get((GSequenceIter *)child->user_data);
  if (entry->parent)
  {
    iter->stamp = priv->stamp;
    iter->user_data = entry->parent;
    ret = TRUE;
  }
  return ret;
}

static IsStoreEntry *
find_entry(IsStore *self,
           const gchar *path)
{
  IsStorePrivate *priv;
  GSequence *entries;
  GSequenceIter *iter;
  IsStoreEntry *entry = NULL;
  gchar **names;
  int i;

  priv = self->priv;

  entries = priv->entries;
  names = g_strsplit(path, "/", 0);

  /* iterate through name components */
  for (i = 0; names[i] != NULL; i++)
  {
    const gchar *name = names[i];

    entry = NULL;

    for (iter = g_sequence_get_begin_iter(entries);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter))
    {
      entry = (IsStoreEntry *)g_sequence_get(iter);
      if (g_strcmp0(entry->name, name) == 0)
      {
        entries = entry->entries;
        break;
      }
      entry = NULL;
    }
    if (!entry)
    {
      /* no entry exists with this name component */
      break;
    }
  }
  g_strfreev(names);
  return entry;
}

IsStore *
is_store_new(void)
{
  return g_object_new(IS_TYPE_STORE, NULL);
}

gboolean
is_store_add_sensor(IsStore *self,
                    IsSensor *sensor,
                    GtkTreeIter *iter)
{
  IsStorePrivate *priv;
  GSequence *entries;
  IsStoreEntry *entry = NULL;
  GSequenceIter *parent = NULL;
  gchar **names = NULL;
  int i;
  GtkTreePath *path;
  GtkTreeIter _iter;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(self), FALSE);
  g_return_val_if_fail(IS_IS_SENSOR(sensor), FALSE);

  priv = self->priv;
  entry = find_entry(self, is_sensor_get_path(sensor));
  if (entry)
  {
    is_warning("store", "sensor %s already exists in store, not adding duplicate",
               is_sensor_get_path(sensor));
    goto out;
  }

  entries = priv->entries;
  names = g_strsplit(is_sensor_get_path(sensor), "/", 0);
  /* otherwise iterate through to create the entry */
  for (i = 0; names[i] != NULL; i++)
  {
    GSequenceIter *seq_iter;
    gchar *name = names[i];

    entry = NULL;

    for (seq_iter = g_sequence_get_begin_iter(entries);
         !g_sequence_iter_is_end(seq_iter);
         seq_iter = g_sequence_iter_next(seq_iter))
    {
      entry = (IsStoreEntry *)g_sequence_get(seq_iter);
      if (g_strcmp0(entry->name, name) == 0)
      {
        entries = entry->entries;
        parent = seq_iter;
        break;
      }
      entry = NULL;
    }
    if (!entry)
    {
      /* create entry for this name component */
      entry = entry_new(name);
      entry->iter = g_sequence_append(entries, entry);
      entry->parent = parent;
      entries = entry->entries;
      _iter.stamp = priv->stamp;
      _iter.user_data = entry->iter;
      path = gtk_tree_model_get_path(GTK_TREE_MODEL(self),
                                     &_iter);
      gtk_tree_model_row_inserted(GTK_TREE_MODEL(self), path,
                                  &_iter);
      gtk_tree_path_free(path);
      /* parent of the next entry we create will be this
       * entry */
      parent = entry->iter;
    }
  }
  g_strfreev(names);

  g_assert(entry);
  g_assert(find_entry(self, is_sensor_get_path(sensor)) == entry);

  is_debug("store", "inserted sensor %s with label %s",
           is_sensor_get_path(sensor), is_sensor_get_label(sensor));
  entry->sensor = g_object_ref(sensor);
  _iter.stamp = priv->stamp;
  _iter.user_data = entry->iter;
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(self),
                                 &_iter);
  gtk_tree_model_row_changed(GTK_TREE_MODEL(self), path,
                             &_iter);
  gtk_tree_path_free(path);
  /* return a copy of iter */
  if (iter != NULL)
  {
    iter->stamp = priv->stamp;
    iter->user_data = entry->iter;
  }
  ret = TRUE;

out:
  return ret;
}

static void
remove_entry(IsStore *self,
             IsStoreEntry *entry)
{
  IsStorePrivate *priv;
  GtkTreeIter iter;
  GtkTreePath *path;
  GSequenceIter *parent_iter;

  priv = self->priv;

  parent_iter = entry->parent;
  iter.stamp = priv->stamp;
  iter.user_data = entry->iter;
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(self), &iter);
  g_sequence_remove(entry->iter);
  gtk_tree_model_row_deleted(GTK_TREE_MODEL(self), path);
  gtk_tree_path_free(path);

  /* remove parent if it has no children now */
  if (parent_iter)
  {
    IsStoreEntry *parent = g_sequence_get(parent_iter);
    if (g_sequence_get_length(parent->entries) == 0)
    {
      remove_entry(self, parent);
    }
  }
}

gboolean
is_store_remove_path(IsStore *self,
                     const gchar *path)
{
  IsStoreEntry *entry = NULL;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(self), FALSE);
  g_return_val_if_fail(path != NULL, FALSE);

  entry = find_entry(self, path);
  if (entry)
  {
    remove_entry(self, entry);
    ret = TRUE;
  }
  return ret;
}

gboolean
is_store_set_label(IsStore *self,
                   GtkTreeIter *iter,
                   const gchar *label)
{
  IsStorePrivate *priv;
  IsStoreEntry *entry = NULL;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(self), FALSE);
  g_return_val_if_fail(iter != NULL, FALSE);

  priv = self->priv;

  g_return_val_if_fail(iter->stamp == priv->stamp, FALSE);
  g_return_val_if_fail(iter->user_data, FALSE);

  entry = (IsStoreEntry *)g_sequence_get(iter->user_data);
  if (entry->sensor &&
      g_strcmp0(is_sensor_get_label(entry->sensor), label) != 0)
  {
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
is_store_set_enabled(IsStore *self,
                     GtkTreeIter *iter,
                     gboolean enabled)
{
  IsStorePrivate *priv;
  IsStoreEntry *entry = NULL;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(self), FALSE);
  g_return_val_if_fail(iter != NULL, FALSE);

  priv = self->priv;

  g_return_val_if_fail(iter->stamp == priv->stamp, FALSE);
  g_return_val_if_fail(iter->user_data, FALSE);

  entry = (IsStoreEntry *)g_sequence_get(iter->user_data);
  if (entry->sensor && entry->enabled != enabled)
  {
    GtkTreePath *path;

    entry->enabled = enabled;

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(self), iter);
    gtk_tree_model_row_changed(GTK_TREE_MODEL(self), path, iter);
    gtk_tree_path_free(path);

    ret = TRUE;
  }
  return ret;
}

gboolean
is_store_remove(IsStore *self,
                GtkTreeIter *iter)
{
  IsStorePrivate *priv;
  IsStoreEntry *entry;

  g_return_val_if_fail(IS_IS_STORE(self), FALSE);
  g_return_val_if_fail(iter != NULL, FALSE);

  priv = self->priv;

  g_return_val_if_fail(iter->stamp == priv->stamp, FALSE);
  g_return_val_if_fail(iter->user_data, FALSE);

  entry = (IsStoreEntry *)
          g_sequence_get((GSequenceIter *)(iter->user_data));
  remove_entry(self, entry);
  return TRUE;
}

gboolean is_store_get_iter(IsStore *self,
                           const gchar *path,
                           GtkTreeIter *iter)
{
  IsStorePrivate *priv;
  IsStoreEntry *entry = NULL;
  gboolean ret = FALSE;

  g_return_val_if_fail(IS_IS_STORE(self), FALSE);
  g_return_val_if_fail(path != NULL, FALSE);

  priv = self->priv;

  entry = find_entry(self, path);
  if (!entry)
  {
    goto out;
  }
  iter->stamp = priv->stamp;
  iter->user_data = entry->iter;
  ret = TRUE;

out:
  return ret;
}
