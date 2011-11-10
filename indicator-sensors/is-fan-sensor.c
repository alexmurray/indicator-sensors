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

#include "is-fan-sensor.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE(IsFanSensor, is_fan_sensor, IS_TYPE_SENSOR);

static void
is_fan_sensor_class_init(IsFanSensorClass *klass)
{
	/* nothing to do */
}

static void
is_fan_sensor_init(IsFanSensor *self)
{
	/* nothing to do */
}

IsSensor *
is_fan_sensor_new(const gchar *path)
{
	return g_object_new(IS_TYPE_FAN_SENSOR,
			    "path", path,
			    "units", _("RPM"),
			    "icon", IS_STOCK_FAN,
			    NULL);
}
