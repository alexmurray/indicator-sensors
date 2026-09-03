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

#include "is-sensor.h"

static void
test_sensor_set_error_basic(void)
{
  IsSensor *sensor = is_sensor_new("test/sensor");

  g_assert_null(is_sensor_get_error(sensor));

  is_sensor_set_error(sensor, "read failed");
  g_assert_cmpstr(is_sensor_get_error(sensor), ==, "read failed");

  is_sensor_set_error(sensor, NULL);
  g_assert_null(is_sensor_get_error(sensor));

  g_object_unref(sensor);
}

/* Regression test for a use-after-free: is_sensor_set_error() used to
 * fetch the existing "error-notification" qdata with g_object_get_data()
 * (a non-owning borrow) and then manually g_object_unref() it, without
 * clearing the qdata association. Replacing the error again then made
 * GLib invoke the still-registered g_object_unref() destroy-notify a
 * second time on the same, now-freed pointer. Repeatedly setting,
 * replacing and clearing the error exercises every transition of that
 * bookkeeping. */
static void
test_sensor_set_error_repeated_transitions_do_not_crash(void)
{
  IsSensor *sensor = is_sensor_new("test/sensor");

  is_sensor_set_error(sensor, "first error");
  is_sensor_set_error(sensor, "second error");
  g_assert_cmpstr(is_sensor_get_error(sensor), ==, "second error");

  is_sensor_set_error(sensor, NULL);
  g_assert_null(is_sensor_get_error(sensor));

  is_sensor_set_error(sensor, "third error");
  g_assert_cmpstr(is_sensor_get_error(sensor), ==, "third error");

  is_sensor_set_error(sensor, "fourth error");
  is_sensor_set_error(sensor, NULL);

  g_object_unref(sensor);
}

static void
on_notify_increment(GObject *object, GParamSpec *pspec, gpointer user_data)
{
  guint *count = user_data;

  (*count)++;
}

static void
test_sensor_set_error_same_value_is_noop(void)
{
  IsSensor *sensor = is_sensor_new("test/sensor");
  guint notify_count = 0;

  is_sensor_set_error(sensor, "error");
  g_signal_connect(sensor, "notify::error", G_CALLBACK(on_notify_increment),
                   &notify_count);

  is_sensor_set_error(sensor, "error");
  g_assert_cmpuint(notify_count, ==, 0);

  is_sensor_set_error(sensor, "different error");
  g_assert_cmpuint(notify_count, ==, 1);

  g_object_unref(sensor);
}

int
main(int argc, char *argv[])
{
  gtk_test_init(&argc, &argv, NULL);

  g_test_add_func("/sensor/set-error/basic", test_sensor_set_error_basic);
  g_test_add_func("/sensor/set-error/repeated-transitions-do-not-crash",
                  test_sensor_set_error_repeated_transitions_do_not_crash);
  g_test_add_func("/sensor/set-error/same-value-is-noop",
                  test_sensor_set_error_same_value_is_noop);

  return g_test_run();
}
