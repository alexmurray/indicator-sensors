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

#ifndef __IS_SENSOR_STORE_H__
#define __IS_SENSOR_STORE_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include "is-sensor.h"

G_BEGIN_DECLS

#define IS_TYPE_SENSOR_STORE			\
	(is_sensor_store_get_type())
#define IS_SENSOR_STORE(obj)					\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),			\
				    IS_TYPE_SENSOR_STORE,	\
				    IsSensorStore))
#define IS_SENSOR_STORE_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),		\
				 IS_TYPE_SENSOR_STORE,	\
				 IsSensorStoreClass))
#define IS_IS_SENSOR_STORE(obj)					\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),			\
				    IS_TYPE_SENSOR_STORE))
#define IS_IS_SENSOR_STORE_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),		\
				 IS_TYPE_SENSOR_STORE))
#define IS_SENSOR_STORE_GET_CLASS(obj)				\
	(G_TYPE_INSTANCE_GET_CLASS((obj),			\
				   IS_TYPE_SENSOR_STORE,	\
				   IsSensorStoreClass))

typedef struct _IsSensorStore      IsSensorStore;
typedef struct _IsSensorStoreClass IsSensorStoreClass;
typedef struct _IsSensorStorePrivate IsSensorStorePrivate;

struct _IsSensorStoreClass
{
	GObjectClass parent_class;
};

struct _IsSensorStore
{
	GObject parent;
	IsSensorStorePrivate *priv;
};

enum
{
	IS_SENSOR_STORE_COL_FAMILY = 0,
	IS_SENSOR_STORE_COL_ID,
	IS_SENSOR_STORE_COL_LABEL,
	IS_SENSOR_STORE_COL_SENSOR,
	IS_SENSOR_STORE_COL_ENABLED,
	IS_SENSOR_STORE_N_COLUMNS,
};

GType is_sensor_store_get_type(void) G_GNUC_CONST;
IsSensorStore *is_sensor_store_new(void);
gboolean is_sensor_store_add_sensor(IsSensorStore *self,
				    IsSensor *sensor,
				    gboolean enabled);
gboolean is_sensor_store_remove_family(IsSensorStore *self,
				       const gchar *family);
gboolean is_sensor_store_set_label(IsSensorStore *self,
				   GtkTreeIter *iter,
				   const gchar *label);
gboolean is_sensor_store_set_enabled(IsSensorStore *self,
				     GtkTreeIter *iter,
				     gboolean enabled);

G_END_DECLS

#endif /* __IS_SENSOR_STORE_H__ */
