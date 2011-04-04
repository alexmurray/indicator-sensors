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

static void is_sensor_dispose(GObject *object);
static void is_sensor_finalize(GObject *object);
static void is_sensor_get_property(GObject *object,
				   guint property_id, GValue *value, GParamSpec *pspec);
static void is_sensor_set_property(GObject *object,
				   guint property_id, const GValue *value, GParamSpec *pspec);

/* signal enum */
enum {
	SIGNAL_DUMMY,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

/* properties */
enum {
	PROP_FAMILY = 1,
	PROP_ID = 2,
	LAST_PROPERTY
};

struct _IsSensorPrivate
{
	gchar *family;
	gchar *id;
};

static void
is_sensor_class_init(IsSensorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(IsSensorPrivate));

	gobject_class->get_property = is_sensor_get_property;
	gobject_class->set_property = is_sensor_set_property;
	gobject_class->dispose = is_sensor_dispose;
	gobject_class->finalize = is_sensor_finalize;

	g_object_class_install_property(gobject_class, PROP_FAMILY,
					g_param_spec_string("family", "family property",
							    "family of this sensor.",
							    NULL,
							    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property(gobject_class, PROP_ID,
					g_param_spec_string("id", "id property",
							    "id of this sensor.",
							    NULL,
							    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	signals[SIGNAL_DUMMY] = g_signal_new("dummy",
					     G_OBJECT_CLASS_TYPE(klass),
					     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
					     0,
					     NULL, NULL,
					     g_cclosure_marshal_VOID__VOID,
					     G_TYPE_NONE, 0);

}

static void
is_sensor_init (IsSensor *self)
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
	IsSensorPrivate *priv = self->priv;

	/* Make compiler happy */
	(void)priv;

	switch (property_id) {
	case PROP_FAMILY:
		g_value_set_string(value, is_sensor_get_family(self));
		break;
	case PROP_ID:
		g_value_set_string(value, is_sensor_get_id(self));
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

	/* Make compiler happy */
	(void)priv;

	switch (property_id) {
	case PROP_FAMILY:
		g_assert(!priv->family);
		priv->family = g_value_dup_string(value);
		break;
	case PROP_ID:
		g_assert(!priv->id);
		priv->id = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}


static void
is_sensor_dispose (GObject *object)
{
	IsSensor *self = (IsSensor *)object;
	IsSensorPrivate *priv = self->priv;

	G_OBJECT_CLASS(is_sensor_parent_class)->dispose(object);
}

static void
is_sensor_finalize (GObject *object)
{
	IsSensor *self = (IsSensor *)object;

	/* Make compiler happy */
	(void)self;

	G_OBJECT_CLASS(is_sensor_parent_class)->finalize(object);
}

IsSensor *
is_sensor_new(const gchar *family,
	      const gchar *id)
{
	return g_object_new(IS_TYPE_SENSOR,
			    "family", family,
			    "id", id,
			    NULL);
}

const gchar *
is_sensor_get_family(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), NULL);
	return self->priv->family;
}

const gchar *
is_sensor_get_id(IsSensor *self)
{
	g_return_val_if_fail(IS_IS_SENSOR(self), NULL);
	return self->priv->id;
}
