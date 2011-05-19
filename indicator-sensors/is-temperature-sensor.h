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

#ifndef __IS_TEMPERATURE_SENSOR_H__
#define __IS_TEMPERATURE_SENSOR_H__

#include "is-sensor.h"


G_BEGIN_DECLS

#define IS_TYPE_TEMPERATURE_SENSOR		\
	(is_temperature_sensor_get_type())
#define IS_TEMPERATURE_SENSOR(obj)				\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),			\
				    IS_TYPE_TEMPERATURE_SENSOR,	\
				    IsTemperatureSensor))
#define IS_TEMPERATURE_SENSOR_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),			\
				 IS_TYPE_TEMPERATURE_SENSOR,	\
				 IsTemperatureSensorClass))
#define IS_IS_TEMPERATURE_SENSOR(obj)                                   \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),				\
				    IS_TYPE_TEMPERATURE_SENSOR))
#define IS_IS_TEMPERATURE_SENSOR_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),			\
				 IS_TYPE_TEMPERATURE_SENSOR))
#define IS_TEMPERATURE_SENSOR_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),			\
				   IS_TYPE_TEMPERATURE_SENSOR,	\
				   IsTemperatureSensorClass))

typedef struct _IsTemperatureSensor      IsTemperatureSensor;
typedef struct _IsTemperatureSensorClass IsTemperatureSensorClass;
typedef struct _IsTemperatureSensorPrivate IsTemperatureSensorPrivate;

struct _IsTemperatureSensorClass
{
	IsSensorClass parent_class;
};

struct _IsTemperatureSensor
{
	IsSensor parent;
	IsTemperatureSensorPrivate *priv;
};

typedef enum {
	IS_TEMPERATURE_SENSOR_SCALE_INVALID,
	IS_TEMPERATURE_SENSOR_SCALE_CELSIUS,
	IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT,
	NUM_IS_TEMPERATURE_SENSOR_SCALE,
} IsTemperatureSensorScale;

GType is_temperature_sensor_get_type(void) G_GNUC_CONST;
IsSensor *is_temperature_sensor_new(const gchar *path,
				    const gchar *label);
void is_temperature_sensor_set_scale(IsTemperatureSensor *sensor,
				     IsTemperatureSensorScale scale);
IsTemperatureSensorScale is_temperature_sensor_get_scale(IsTemperatureSensor *sensor);
void is_temperature_sensor_set_celsius_value(IsTemperatureSensor *sensor,
					     gdouble value);

G_END_DECLS

#endif /* __IS_TEMPERATURE_SENSOR_H__ */
