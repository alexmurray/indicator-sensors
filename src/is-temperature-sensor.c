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

#include "is-temperature-sensor.h"

G_DEFINE_TYPE(IsTemperatureSensor, is_temperature_sensor, IS_TYPE_SENSOR);

static void is_temperature_sensor_finalize(GObject *object);


struct _IsTemperatureSensorPrivate
{
	IsTemperatureSensorUnits units;
};

static void
is_temperature_sensor_class_init(IsTemperatureSensorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsTemperatureSensorPrivate));

	gobject_class->finalize = is_temperature_sensor_finalize;
}

static void
is_temperature_sensor_init(IsTemperatureSensor *self)
{
	IsTemperatureSensorPrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_TEMPERATURE_SENSOR,
					    IsTemperatureSensorPrivate);

	self->priv = priv;
}



static void
is_temperature_sensor_finalize(GObject *object)
{
	IsTemperatureSensor *self = (IsTemperatureSensor *)object;

	/* Make compiler happy */
	(void)self;

	G_OBJECT_CLASS(is_temperature_sensor_parent_class)->finalize(object);
}

IsSensor *
is_temperature_sensor_new(const gchar *path,
			  const gchar *label)
{
	return is_temperature_sensor_new_full(path, label,
					      0.0f, 0.0f,
					      IS_TEMPERATURE_SENSOR_UNITS_CELSIUS);
}

static const gchar *
is_temperature_sensor_units_to_string(IsTemperatureSensorUnits units)
{
	const gchar *string = "";

	switch (units) {
	case IS_TEMPERATURE_SENSOR_UNITS_CELSIUS:
		string = "\302\260C";
		break;
	case IS_TEMPERATURE_SENSOR_UNITS_FAHRENHEIT:
		string = "\302\260F";
		break;
	case NUM_IS_TEMPERATURE_SENSOR_UNITS:
	default:
		g_warning("Unable to convert IsTemperatureSensorUnits %d to string",
			  units);
	}
	return string;
}

IsSensor *
is_temperature_sensor_new_full(const gchar *path,
			       const gchar *label,
			       gdouble min,
			       gdouble max,
			       IsTemperatureSensorUnits units)
{
	IsTemperatureSensor *self;

	g_return_val_if_fail(units == IS_TEMPERATURE_SENSOR_UNITS_CELSIUS ||
			     units == IS_TEMPERATURE_SENSOR_UNITS_FAHRENHEIT,
			     NULL);

	self = g_object_new(IS_TYPE_TEMPERATURE_SENSOR,
			    "path", path,
			    "label", label,
			    "min", min,
			    "max", max,
			    "units", is_temperature_sensor_units_to_string(units),
			    NULL);
	self->priv->units = units;
	return IS_SENSOR(self);
}

IsTemperatureSensorUnits
is_temperature_sensor_get_units(IsTemperatureSensor *self)
{
	g_return_val_if_fail(IS_IS_TEMPERATURE_SENSOR(self), 0);

	return self->priv->units;
}

gdouble
celcius_to_fahrenheit(gdouble celcius)
{
	return (celcius * 9.0 / 5.0) + 32.0;
}

gdouble
fahrenheit_to_celcius(gdouble fahrenheit)
{
	return (fahrenheit - 32.0) * 5.0 / 9.0;
}


void is_temperature_sensor_set_units(IsTemperatureSensor *self,
				     IsTemperatureSensorUnits units)
{
	IsTemperatureSensorPrivate *priv;

	g_return_if_fail(IS_IS_TEMPERATURE_SENSOR(self));
	g_return_if_fail(units == IS_TEMPERATURE_SENSOR_UNITS_CELSIUS ||
			 units == IS_TEMPERATURE_SENSOR_UNITS_FAHRENHEIT);

	priv = self->priv;

	if (units != priv->units) {
		gdouble value = is_sensor_get_value(IS_SENSOR(self));
		gdouble min = is_sensor_get_min(IS_SENSOR(self));
		gdouble max = is_sensor_get_max(IS_SENSOR(self));

		priv->units = units;
		is_sensor_set_units(IS_SENSOR(self),
				    is_temperature_sensor_units_to_string(priv->units));
		switch (priv->units) {
		case IS_TEMPERATURE_SENSOR_UNITS_CELSIUS:
			value = celcius_to_fahrenheit(value);
			min = celcius_to_fahrenheit(min);
			max = celcius_to_fahrenheit(max);
			break;
		case IS_TEMPERATURE_SENSOR_UNITS_FAHRENHEIT:
			value = fahrenheit_to_celcius(value);
			min = fahrenheit_to_celcius(min);
			max = fahrenheit_to_celcius(max);
			break;
		case NUM_IS_TEMPERATURE_SENSOR_UNITS:
		default:
			g_assert_not_reached();
		}
		/* set all in one go */
		g_object_set(self,
			     "value", value,
			     "min", min,
			     "max", max,
			     NULL);
	}

}

