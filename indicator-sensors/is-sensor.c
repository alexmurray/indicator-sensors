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
#include <math.h>

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
	PROP_ALARM_VALUE,
	PROP_ALARM_MODE,
	PROP_UNITS,
	PROP_UPDATE_INTERVAL,
	PROP_ALARMED,
	LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = {NULL};

struct _IsSensorPrivate
{
	gchar *path;
	gchar *label;
	gdouble value;
	gdouble alarm_value;
	IsSensorAlarmMode alarm_mode;
	gchar *units;
	guint update_interval;
	gint64 last_update;
	gboolean alarmed;
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
	properties[PROP_ALARM_VALUE] = g_param_spec_double("alarm-value", "sensor alarm limit",
							   "alarm limit of this sensor.",
							   -G_MAXDOUBLE, G_MAXDOUBLE,
							   0.0,
							   G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ALARM_VALUE, properties[PROP_ALARM_VALUE]);
	properties[PROP_ALARM_MODE] = g_param_spec_int("alarm-mode", "sensor alarm mode",
						       "alarm mode of this sensor.",
						       IS_SENSOR_ALARM_MODE_DISABLED,
						       IS_SENSOR_ALARM_MODE_HIGH,
						       IS_SENSOR_ALARM_MODE_DISABLED,
						       G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ALARM_MODE, properties[PROP_ALARM_MODE]);
	properties[PROP_UNITS] = g_param_spec_string("units", "sensor units",
						     "units of this sensor.",
						     NULL,
						     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_UNITS, properties[PROP_UNITS]);
	properties[PROP_UPDATE_INTERVAL] = g_param_spec_uint("update-interval",
							     "sensor update interval",
							     "update interval of this sensor in seconds.",
							     0, G_MAXUINT,
							     5,
							     G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_UPDATE_INTERVAL, properties[PROP_UPDATE_INTERVAL]);
	properties[PROP_ALARMED] = g_param_spec_boolean("alarmed", "is sensor alarmed",
							"alarmed state of this sensor.",
							FALSE,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ALARMED, properties[PROP_ALARMED]);

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
	case PROP_ALARM_VALUE:
		g_value_set_double(value, is_sensor_get_alarm_value(self));
		break;
	case PROP_ALARM_MODE:
		g_value_set_int(value, is_sensor_get_alarm_mode(self));
		break;
	case PROP_UNITS:
		g_value_set_string(value, is_sensor_get_units(self));
		break;
	case PROP_UPDATE_INTERVAL:
		g_value_set_uint(value, is_sensor_get_update_interval(self));
		break;
	case PROP_ALARMED:
		g_value_set_boolean(value, is_sensor_get_alarmed(self));
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
	case PROP_ALARM_VALUE:
		is_sensor_set_alarm_value(self, g_value_get_double(value));
		break;
	case PROP_ALARM_MODE:
		is_sensor_set_alarm_mode(self, g_value_get_int(value));
		break;
	case PROP_UNITS:
		is_sensor_set_units(self, g_value_get_string(value));
		break;
	case PROP_UPDATE_INTERVAL:
		is_sensor_set_update_interval(self, g_value_get_uint(value));
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
is_sensor_new(const gchar *path)
{
	return g_object_new(IS_TYPE_SENSOR,
			    "path", path,
			    "label", NULL,
			    "alarm-value", 0.0,
			    "alarm-mode", IS_SENSOR_ALARM_MODE_DISABLED,
			    "units", "?",
			    "update-interval", 5,
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
	IsSensorPrivate *priv;

	g_return_if_fail(IS_IS_SENSOR(self));

	priv = self->priv;

	if (label != NULL && g_strcmp0(label, "") != 0) {
		g_free(priv->label);
		priv->label = g_strdup(label);
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

static void
update_alarmed(IsSensor *self)
{
	IsSensorPrivate *priv = self->priv;
	gboolean alarmed = FALSE;

	switch (priv->alarm_mode) {
	case IS_SENSOR_ALARM_MODE_DISABLED:
		alarmed = FALSE;
		break;

	case IS_SENSOR_ALARM_MODE_LOW:
		alarmed = (priv->value <= priv->alarm_value);
		break;

	case IS_SENSOR_ALARM_MODE_HIGH:
		alarmed = (priv->value >= priv->alarm_value);
		break;

	default:
		g_assert_not_reached();
	}

	if (priv->alarmed != alarmed) {
		priv->alarmed = alarmed;
		g_object_notify_by_pspec(G_OBJECT(self),
					 properties[PROP_ALARMED]);
	}
}

void
is_sensor_set_value(IsSensor *self,
		    gdouble value)
{
	IsSensorPrivate *priv;

	g_return_if_fail(IS_IS_SENSOR(self));

	priv = self->priv;

	if (fabs(priv->value - value) > DBL_EPSILON) {
		priv->value = value;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_VALUE]);
		update_alarmed(self);
	}
}

gdouble
is_sensor_get_alarm_value(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), 0.0f);
	return self->priv->alarm_value;
}

void
is_sensor_set_alarm_value(IsSensor *self,
			  gdouble alarm_value)
{
	IsSensorPrivate *priv;

	g_return_if_fail(IS_IS_SENSOR(self));

	priv = self->priv;

	if (fabs(priv->alarm_value - alarm_value) > DBL_EPSILON) {
		priv->alarm_value = alarm_value;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ALARM_VALUE]);
		update_alarmed(self);
	}
}

IsSensorAlarmMode
is_sensor_get_alarm_mode(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), IS_SENSOR_ALARM_MODE_DISABLED);
	return self->priv->alarm_mode;
}

void
is_sensor_set_alarm_mode(IsSensor *self,
			 IsSensorAlarmMode alarm_mode)
{
	IsSensorPrivate *priv;

	g_return_if_fail(IS_IS_SENSOR(self));

	priv = self->priv;

	if (priv->alarm_mode != alarm_mode) {
		priv->alarm_mode = alarm_mode;
		g_object_notify_by_pspec(G_OBJECT(self),
					 properties[PROP_ALARM_MODE]);
		update_alarmed(self);
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

guint
is_sensor_get_update_interval(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), 0);
	return self->priv->update_interval;
}

void
is_sensor_set_update_interval(IsSensor *self,
			      guint update_interval)
{
	g_return_if_fail(IS_IS_SENSOR(self));
	if (self->priv->update_interval != update_interval) {
		self->priv->update_interval = update_interval;
		g_object_notify_by_pspec(G_OBJECT(self),
					 properties[PROP_UPDATE_INTERVAL]);
	}
}

gboolean
is_sensor_get_alarmed(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), 0.0f);
	return self->priv->alarmed;
}

void
is_sensor_update_value(IsSensor *self)
{
	IsSensorPrivate *priv;
	gint64 secs;

	g_return_if_fail(IS_IS_SENSOR(self));

	priv = self->priv;

	/* respect update_interval */
	secs = g_get_monotonic_time() / G_USEC_PER_SEC;
	if (!priv->last_update ||
	    secs - priv->last_update >= priv->update_interval) {
		priv->last_update = secs;
		g_signal_emit(self, signals[SIGNAL_UPDATE_VALUE], 0);
	}
}

void
is_sensor_emit_error(IsSensor *self, GError *error)
{
	g_return_if_fail(IS_IS_SENSOR(self) && error != NULL);

	g_signal_emit(self, signals[SIGNAL_ERROR], 0, error);
}
