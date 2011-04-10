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

#ifndef __IS_SENSOR_H__
#define __IS_SENSOR_H__

#include <glib-object.h>


G_BEGIN_DECLS

#define IS_TYPE_SENSOR				\
	(is_sensor_get_type())
#define IS_SENSOR(obj)					\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),		\
				    IS_TYPE_SENSOR,	\
				    IsSensor))
#define IS_SENSOR_CLASS(klass)				\
	(G_TYPE_CHECK_CLASS_CAST((klass),		\
				 IS_TYPE_SENSOR,	\
				 IsSensorClass))
#define IS_IS_SENSOR(obj)				\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),		\
				    IS_TYPE_SENSOR))
#define IS_IS_SENSOR_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),		\
				 IS_TYPE_SENSOR))
#define IS_SENSOR_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),		\
				   IS_TYPE_SENSOR,	\
				   IsSensorClass))

typedef struct _IsSensor      IsSensor;
typedef struct _IsSensorClass IsSensorClass;
typedef struct _IsSensorPrivate IsSensorPrivate;

struct _IsSensorClass
{
	GObjectClass parent_class;
	void (*update)(IsSensor *sensor);
};

struct _IsSensor
{
	GObject parent;
	IsSensorPrivate *priv;
};

GType is_sensor_get_type(void) G_GNUC_CONST;
IsSensor *is_sensor_new(const gchar *family,
			const gchar *id,
			const gchar *label,
			gdouble min,
			gdouble max);
void is_sensor_update(IsSensor *self);
const gchar *is_sensor_get_family(IsSensor *self);
const gchar *is_sensor_get_id(IsSensor *self);
const gchar *is_sensor_get_label(IsSensor *self);
void is_sensor_set_label(IsSensor *self, const gchar *label);
gdouble is_sensor_get_value(IsSensor *self);
void is_sensor_set_value(IsSensor *self, gdouble value);
gdouble is_sensor_get_min(IsSensor *self);
void is_sensor_set_min(IsSensor *self, gdouble min);
gdouble is_sensor_get_max(IsSensor *self);
void is_sensor_set_max(IsSensor *self, gdouble max);

G_END_DECLS

#endif /* __IS_SENSOR_H__ */
