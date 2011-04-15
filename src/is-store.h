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

#ifndef __IS_STORE_H__
#define __IS_STORE_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include "is-sensor.h"

G_BEGIN_DECLS

#define IS_TYPE_STORE				\
	(is_store_get_type())
#define IS_STORE(obj)					\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),		\
				    IS_TYPE_STORE,	\
				    IsStore))
#define IS_STORE_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),	\
				 IS_TYPE_STORE,	\
				 IsStoreClass))
#define IS_IS_STORE(obj)				\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),		\
				    IS_TYPE_STORE))
#define IS_IS_STORE_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),		\
				 IS_TYPE_STORE))
#define IS_STORE_GET_CLASS(obj)				\
	(G_TYPE_INSTANCE_GET_CLASS((obj),		\
				   IS_TYPE_STORE,	\
				   IsStoreClass))

typedef struct _IsStore      IsStore;
typedef struct _IsStoreClass IsStoreClass;
typedef struct _IsStorePrivate IsStorePrivate;

struct _IsStoreClass
{
	GObjectClass parent_class;
};

struct _IsStore
{
	GObject parent;
	IsStorePrivate *priv;
};

enum
{
	IS_STORE_COL_FAMILY = 0,
	IS_STORE_COL_ID,
	IS_STORE_COL_LABEL,
	IS_STORE_COL_SENSOR,
	IS_STORE_COL_ENABLED,
	IS_STORE_N_COLUMNS,
};

GType is_store_get_type(void) G_GNUC_CONST;
IsStore *is_store_new(void);
gboolean is_store_add_sensor(IsStore *self,
			     IsSensor *sensor,
			     gboolean enabled);
gboolean is_store_remove_family(IsStore *self,
				const gchar *family);
gboolean is_store_set_label(IsStore *self,
			    GtkTreeIter *iter,
			    const gchar *label);
gboolean is_store_set_enabled(IsStore *self,
			      GtkTreeIter *iter,
			      gboolean enabled);

G_END_DECLS

#endif /* __IS_STORE_H__ */
