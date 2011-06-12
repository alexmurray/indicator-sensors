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

#ifndef __IS_FAN_SENSOR_H__
#define __IS_FAN_SENSOR_H__

#include "is-sensor.h"


G_BEGIN_DECLS

#define IS_TYPE_FAN_SENSOR		\
	(is_fan_sensor_get_type())
#define IS_FAN_SENSOR(obj)				\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),			\
				    IS_TYPE_FAN_SENSOR,	\
				    IsFanSensor))
#define IS_FAN_SENSOR_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),			\
				 IS_TYPE_FAN_SENSOR,	\
				 IsFanSensorClass))
#define IS_IS_FAN_SENSOR(obj)                                   \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),				\
				    IS_TYPE_FAN_SENSOR))
#define IS_IS_FAN_SENSOR_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),			\
				 IS_TYPE_FAN_SENSOR))
#define IS_FAN_SENSOR_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),			\
				   IS_TYPE_FAN_SENSOR,	\
				   IsFanSensorClass))

typedef struct _IsFanSensor      IsFanSensor;
typedef struct _IsFanSensorClass IsFanSensorClass;

struct _IsFanSensorClass
{
	IsSensorClass parent_class;
};

struct _IsFanSensor
{
	IsSensor parent;
};

GType is_fan_sensor_get_type(void) G_GNUC_CONST;
IsSensor *is_fan_sensor_new(const gchar *path);

G_END_DECLS

#endif /* __IS_FAN_SENSOR_H__ */
