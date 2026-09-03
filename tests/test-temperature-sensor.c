/*
 * Copyright (C) 2011-2025 Alex Murray <murray.alex@gmail.com>
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

#include "config.h"

#include "is-temperature-sensor.h"
#include <math.h>

static void
test_default_scale_is_celsius(void)
{
  IsSensor *sensor = is_temperature_sensor_new("test/temp");

  g_assert_cmpint(is_temperature_sensor_get_scale(IS_TEMPERATURE_SENSOR(sensor)),
                  ==, IS_TEMPERATURE_SENSOR_SCALE_CELSIUS);

  g_object_unref(sensor);
}

static void
test_scale_conversion_freezing_point(void)
{
  IsSensor *sensor = is_temperature_sensor_new("test/temp");
  IsTemperatureSensor *temp = IS_TEMPERATURE_SENSOR(sensor);

  is_temperature_sensor_set_celsius_value(temp, 0.0);
  g_assert_cmpfloat(is_sensor_get_value(sensor), ==, 0.0);

  is_temperature_sensor_set_scale(temp, IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT);
  g_assert_cmpfloat(is_sensor_get_value(sensor), ==, 32.0);

  g_object_unref(sensor);
}

static void
test_scale_conversion_boiling_point(void)
{
  IsSensor *sensor = is_temperature_sensor_new("test/temp");
  IsTemperatureSensor *temp = IS_TEMPERATURE_SENSOR(sensor);

  is_temperature_sensor_set_celsius_value(temp, 100.0);
  is_temperature_sensor_set_scale(temp, IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT);
  g_assert_cmpfloat(is_sensor_get_value(sensor), ==, 212.0);

  g_object_unref(sensor);
}

static void
test_scale_round_trip(void)
{
  IsSensor *sensor = is_temperature_sensor_new("test/temp");
  IsTemperatureSensor *temp = IS_TEMPERATURE_SENSOR(sensor);

  is_temperature_sensor_set_celsius_value(temp, 37.5);
  is_temperature_sensor_set_scale(temp, IS_TEMPERATURE_SENSOR_SCALE_FAHRENHEIT);
  is_temperature_sensor_set_scale(temp, IS_TEMPERATURE_SENSOR_SCALE_CELSIUS);
  g_assert_cmpfloat(fabs(is_sensor_get_value(sensor) - 37.5), <, 0.0001);

  g_object_unref(sensor);
}

static void
test_set_scale_to_same_scale_is_noop(void)
{
  IsSensor *sensor = is_temperature_sensor_new("test/temp");
  IsTemperatureSensor *temp = IS_TEMPERATURE_SENSOR(sensor);

  is_temperature_sensor_set_celsius_value(temp, 21.0);
  is_temperature_sensor_set_scale(temp, IS_TEMPERATURE_SENSOR_SCALE_CELSIUS);
  g_assert_cmpfloat(is_sensor_get_value(sensor), ==, 21.0);

  g_object_unref(sensor);
}

int
main(int argc, char *argv[])
{
  gtk_test_init(&argc, &argv, NULL);

  g_test_add_func("/temperature-sensor/default-scale-is-celsius",
                  test_default_scale_is_celsius);
  g_test_add_func("/temperature-sensor/scale-conversion/freezing-point",
                  test_scale_conversion_freezing_point);
  g_test_add_func("/temperature-sensor/scale-conversion/boiling-point",
                  test_scale_conversion_boiling_point);
  g_test_add_func("/temperature-sensor/scale-conversion/round-trip",
                  test_scale_round_trip);
  g_test_add_func("/temperature-sensor/set-scale-to-same-scale-is-noop",
                  test_set_scale_to_same_scale_is_noop);

  return g_test_run();
}
