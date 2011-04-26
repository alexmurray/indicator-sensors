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

#include "is-sensor.h"


G_DEFINE_TYPE (IsSensor, is_sensor, G_TYPE_OBJECT);

static void is_sensor_finalize(GObject *object);
static void is_sensor_get_property(GObject *object,
				   guint property_id, GValue *value, GParamSpec *pspec);
static void is_sensor_set_property(GObject *object,
				   guint property_id, const GValue *value, GParamSpec *pspec);

/* signal enum */
enum {
	SIGNAL_UPDATE_VALUE,
	SIGNAL_ERROR,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

/* properties */
enum {
	PROP_0,
	PROP_PATH,
	PROP_LABEL,
	PROP_VALUE,
	PROP_MIN,
	PROP_MAX,
	PROP_UNITS,
	LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = {NULL};

struct _IsSensorPrivate
{
	gchar *path;
	gchar *label;
	gdouble value;
	gdouble min;
	gdouble max;
	gchar *units;
};

static void
is_sensor_class_init(IsSensorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsSensorPrivate));

	gobject_class->get_property = is_sensor_get_property;
	gobject_class->set_property = is_sensor_set_property;
	gobject_class->finalize = is_sensor_finalize;

	properties[PROP_PATH] = g_param_spec_string("path", "path property",
						      "path of this sensor.",
						      NULL,
						      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_PATH, properties[PROP_PATH]);
	properties[PROP_VALUE] = g_param_spec_double("value", "sensor value",
						     "value of this sensor.",
						     -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_VALUE, properties[PROP_VALUE]);
	properties[PROP_LABEL] = g_param_spec_string("label", "sensor label",
						     "label of this sensor.",
						     NULL,
						     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_LABEL, properties[PROP_LABEL]);
	properties[PROP_MIN] = g_param_spec_double("min", "sensor min",
						   "min of this sensor.",
						   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_MIN, properties[PROP_MIN]);
	properties[PROP_MAX] = g_param_spec_double("max", "sensor max",
						   "max of this sensor.",
						   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_MAX, properties[PROP_MAX]);
	properties[PROP_UNITS] = g_param_spec_string("units", "sensor units",
						     "units of this sensor.",
						     NULL,
						     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_UNITS, properties[PROP_UNITS]);

	signals[SIGNAL_UPDATE_VALUE] = g_signal_new("update-value",
						    G_OBJECT_CLASS_TYPE(klass),
						    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						    offsetof(IsSensorClass, update_value),
						    NULL, NULL,
						    g_cclosure_marshal_VOID__VOID,
						    G_TYPE_NONE, 0);
	signals[SIGNAL_ERROR] = g_signal_new("error",
					     G_OBJECT_CLASS_TYPE(klass),
					     G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					     offsetof(IsSensorClass, error),
					     NULL, NULL,
					     g_cclosure_marshal_VOID__BOXED,
					     G_TYPE_NONE, 1, G_TYPE_ERROR);
}

static void
is_sensor_init(IsSensor *self)
{
	IsSensorPrivate *priv =
		G_TYPE_INSTANCE_GET_PRIVATE(self, IS_TYPE_SENSOR,
					    IsSensorPrivate);

	self->priv = priv;
}

static void
is_sensor_get_property(GObject *object,
		       guint property_id, GValue *value, GParamSpec *pspec)
{
	IsSensor *self = IS_SENSOR(object);

	switch (property_id) {
	case PROP_PATH:
		g_value_set_string(value, is_sensor_get_path(self));
		break;
	case PROP_LABEL:
		g_value_set_string(value, is_sensor_get_label(self));
		break;
	case PROP_VALUE:
		g_value_set_double(value, is_sensor_get_value(self));
		break;
	case PROP_MIN:
		g_value_set_double(value, is_sensor_get_min(self));
		break;
	case PROP_MAX:
		g_value_set_double(value, is_sensor_get_max(self));
		break;
	case PROP_UNITS:
		g_value_set_string(value, is_sensor_get_units(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
is_sensor_set_property(GObject *object,
		       guint property_id, const GValue *value, GParamSpec *pspec)
{
	IsSensor *self = IS_SENSOR(object);
	IsSensorPrivate *priv = self->priv;

	switch (property_id) {
	case PROP_PATH:
		priv->path = g_value_dup_string(value);
		break;
	case PROP_LABEL:
		is_sensor_set_label(self, g_value_get_string(value));
		break;
	case PROP_VALUE:
		is_sensor_set_value(self, g_value_get_double(value));
		break;
	case PROP_MIN:
		is_sensor_set_min(self, g_value_get_double(value));
		break;
	case PROP_MAX:
		is_sensor_set_max(self, g_value_get_double(value));
		break;
	case PROP_UNITS:
		is_sensor_set_units(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}


static void
is_sensor_finalize (GObject *object)
{
	IsSensor *self = (IsSensor *)object;
	IsSensorPrivate *priv = self->priv;

	g_free(priv->path);
	priv->path = NULL;
	g_free(priv->label);
	priv->label = NULL;
	g_free(priv->units);
	priv->units = NULL;

	G_OBJECT_CLASS(is_sensor_parent_class)->finalize(object);
}

IsSensor *
is_sensor_new(const gchar *path,
	      const gchar *label,
	      gdouble min,
	      gdouble max,
	      const gchar *units)
{
	return g_object_new(IS_TYPE_SENSOR,
			    "path", path,
			    "label", label,
			    "min", min,
			    "max", max,
			    "units", units,
			    NULL);
}

const gchar *
is_sensor_get_path(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), NULL);
	return self->priv->path;
}

const gchar *
is_sensor_get_label(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), NULL);
	return self->priv->label;
}

void
is_sensor_set_label(IsSensor *self,
		    const gchar *label)
{
	g_return_if_fail(IS_IS_SENSOR(self));
	if (label != NULL && g_strcmp0(label, "") != 0) {
		g_free(self->priv->label);
		self->priv->label = label ? g_strdup(label) : NULL;
		g_object_notify_by_pspec(G_OBJECT(self),
					 properties[PROP_LABEL]);
	}
}

gdouble
is_sensor_get_value(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), 0.0f);
	return self->priv->value;
}

void
is_sensor_set_value(IsSensor *self,
		    gdouble value)
{
	g_return_if_fail(IS_IS_SENSOR(self));
	if (self->priv->value != value) {
		self->priv->value = value;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_VALUE]);
	}
}

gdouble
is_sensor_get_min(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), 0.0f);
	return self->priv->min;
}

void
is_sensor_set_min(IsSensor *self,
		  gdouble min)
{
	g_return_if_fail(IS_IS_SENSOR(self));
	if (self->priv->min != min) {
		self->priv->min = min;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_MIN]);
	}
}

gdouble
is_sensor_get_max(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), 0.0f);
	return self->priv->max;
}

void
is_sensor_set_max(IsSensor *self,
		  gdouble max)
{
	g_return_if_fail(IS_IS_SENSOR(self));
	if (self->priv->max != max) {
		self->priv->max = max;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_MAX]);
	}
}

const gchar *
is_sensor_get_units(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), NULL);
	return self->priv->units;
}

void
is_sensor_set_units(IsSensor *self,
		    const gchar *units)
{
	g_return_if_fail(IS_IS_SENSOR(self));
	g_free(self->priv->units);
	self->priv->units = units ? g_strdup(units) : NULL;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_UNITS]);
}

void
is_sensor_update_value(IsSensor *self)
{
	g_return_if_fail(IS_IS_SENSOR(self));

	g_signal_emit(self, signals[SIGNAL_UPDATE_VALUE], 0);
}

void
is_sensor_emit_error(IsSensor *self, GError *error)
{
	g_return_if_fail(IS_IS_SENSOR(self) && error != NULL);

	g_signal_emit(self, signals[SIGNAL_ERROR], 0, error);
}

