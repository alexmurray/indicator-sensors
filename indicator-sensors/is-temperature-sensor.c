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
static void is_temperature_sensor_get_property(GObject *object,
					  guint property_id, GValue *value, GParamSpec *pspec);
static void is_temperature_sensor_set_property(GObject *object,
					  guint property_id, const GValue *value, GParamSpec *pspec);

/* properties */
enum {
	PROP_0,
	PROP_SCALE,
	LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY];

struct _IsTemperatureSensorPrivate
{
	IsTemperatureSensorScale scale;
};

static void
is_temperature_sensor_class_init(IsTemperatureSensorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsTemperatureSensorPrivate));

	gobject_class->finalize = is_temperature_sensor_finalize;
	gobject_class->get_property = is_temperature_sensor_get_property;
	gobject_class->set_property = is_temperature_sensor_set_property;

	/* TODO: convert to an enum type */
	properties[PROP_SCALE] = g_param_spec_int("scale", "temperature scale",
						  "Sensor temperature scale.",
						  IS_TEMPERATURE_SENSOR_SCALE_CELSIUS,
						  IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT,
						  IS_TEMPERATURE_SENSOR_SCALE_CELSIUS,
						  G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_SCALE,
					properties[PROP_SCALE]);
}

static const gchar *
is_temperature_sensor_scale_to_string(IsTemperatureSensorScale scale)
{
	const gchar *string = "";

	switch (scale) {
	case IS_TEMPERATURE_SENSOR_SCALE_CELSIUS:
		string = "\302\260C";
		break;
	case IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT:
		string = "\302\260F";
		break;
	case IS_TEMPERATURE_SENSOR_SCALE_INVALID:
	case NUM_IS_TEMPERATURE_SENSOR_SCALE:
	default:
		g_warning("Unable to convert IsTemperatureSensorScale %d to string",
			  scale);
	}
	return string;
}

static void
is_temperature_sensor_init(IsTemperatureSensor *self)
{
	IsTemperatureSensorPrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_TEMPERATURE_SENSOR,
					    IsTemperatureSensorPrivate);

	self->priv = priv;

	/* initialise scale to celcius */
	priv->scale = IS_TEMPERATURE_SENSOR_SCALE_CELSIUS;
	is_sensor_set_units(IS_SENSOR(self),
			    is_temperature_sensor_scale_to_string(priv->scale));
}

static void
is_temperature_sensor_get_property(GObject *object,
		       guint property_id, GValue *value, GParamSpec *pspec)
{
	IsTemperatureSensor *self = IS_TEMPERATURE_SENSOR(object);

	switch (property_id) {
	case PROP_SCALE:
		g_value_set_int(value, is_temperature_sensor_get_scale(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
is_temperature_sensor_set_property(GObject *object,
				   guint property_id, const GValue *value, GParamSpec *pspec)
{
	IsTemperatureSensor *self = IS_TEMPERATURE_SENSOR(object);

	switch (property_id) {
	case PROP_SCALE:
		is_temperature_sensor_set_scale(self, g_value_get_int(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
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
					      -G_MAXDOUBLE, G_MAXDOUBLE,
					      IS_TEMPERATURE_SENSOR_SCALE_CELSIUS);
}


IsSensor *
is_temperature_sensor_new_full(const gchar *path,
			       const gchar *label,
			       gdouble min,
			       gdouble max,
			       IsTemperatureSensorScale scale)
{
	IsTemperatureSensor *self;

	g_return_val_if_fail(scale == IS_TEMPERATURE_SENSOR_SCALE_CELSIUS ||
			     scale == IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT,
			     NULL);

	self = g_object_new(IS_TYPE_TEMPERATURE_SENSOR,
			    "path", path,
			    "label", label,
			    "min", min,
			    "max", max,
			    "scale", scale,
			    NULL);
	return IS_SENSOR(self);
}

IsTemperatureSensorScale
is_temperature_sensor_get_scale(IsTemperatureSensor *self)
{
	g_return_val_if_fail(IS_IS_TEMPERATURE_SENSOR(self), 0);

	return self->priv->scale;
}

static gdouble
celcius_to_fahrenheit(gdouble celcius)
{
	return (celcius * 9.0 / 5.0) + 32.0;
}

static gdouble
fahrenheit_to_celcius(gdouble fahrenheit)
{
	return (fahrenheit - 32.0) * 5.0 / 9.0;
}


void is_temperature_sensor_set_scale(IsTemperatureSensor *self,
				     IsTemperatureSensorScale scale)
{
	IsTemperatureSensorPrivate *priv;

	g_return_if_fail(IS_IS_TEMPERATURE_SENSOR(self));
	g_return_if_fail(scale == IS_TEMPERATURE_SENSOR_SCALE_CELSIUS ||
			 scale == IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT);

	priv = self->priv;

	if (scale != priv->scale) {
		gdouble value = is_sensor_get_value(IS_SENSOR(self));
		gdouble min = is_sensor_get_min(IS_SENSOR(self));
		gdouble max = is_sensor_get_max(IS_SENSOR(self));

		/* convert from current scale to new */
		switch (priv->scale) {
		case IS_TEMPERATURE_SENSOR_SCALE_CELSIUS:
			value = celcius_to_fahrenheit(value);
			min = celcius_to_fahrenheit(min);
			max = celcius_to_fahrenheit(max);
			break;
		case IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT:
			value = fahrenheit_to_celcius(value);
			min = fahrenheit_to_celcius(min);
			max = fahrenheit_to_celcius(max);
			break;
		case IS_TEMPERATURE_SENSOR_SCALE_INVALID:
		case NUM_IS_TEMPERATURE_SENSOR_SCALE:
		default:
			g_assert_not_reached();
			break;
		}
		priv->scale = scale;
		is_sensor_set_units(IS_SENSOR(self),
				    is_temperature_sensor_scale_to_string(priv->scale));
		/* set all in one go */
		g_object_set(self,
			     "value", value,
			     "min", min,
			     "max", max,
			     NULL);
	}

}

void
is_temperature_sensor_set_celsius_value(IsTemperatureSensor *self,
					gdouble value)
{
	IsTemperatureSensorPrivate *priv;

	g_return_if_fail(IS_IS_TEMPERATURE_SENSOR(self));

	priv = self->priv;

	switch (priv->scale) {
	case IS_TEMPERATURE_SENSOR_SCALE_CELSIUS:
		break;

	case IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT:
		value = celcius_to_fahrenheit(value);
		break;

	case IS_TEMPERATURE_SENSOR_SCALE_INVALID:
	case NUM_IS_TEMPERATURE_SENSOR_SCALE:
	default:
		g_assert_not_reached();
	}
	is_sensor_set_value(IS_SENSOR(self), value);
}
