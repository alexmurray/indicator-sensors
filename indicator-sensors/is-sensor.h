/*
 * Copyright (C) 2011-2019 Alex Murray <murray.alex@gmail.com>
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

#ifndef __IS_SENSOR_H__
#define __IS_SENSOR_H__

#include <glib-object.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS

#define IS_TYPE_SENSOR        \
  (is_sensor_get_type())
#define IS_SENSOR(obj)          \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),    \
                              IS_TYPE_SENSOR, \
                              IsSensor))
#define IS_SENSOR_CLASS(klass)        \
  (G_TYPE_CHECK_CLASS_CAST((klass),   \
                           IS_TYPE_SENSOR,  \
                           IsSensorClass))
#define IS_IS_SENSOR(obj)       \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),    \
                              IS_TYPE_SENSOR))
#define IS_IS_SENSOR_CLASS(klass)     \
  (G_TYPE_CHECK_CLASS_TYPE((klass),   \
                           IS_TYPE_SENSOR))
#define IS_SENSOR_GET_CLASS(obj)      \
  (G_TYPE_INSTANCE_GET_CLASS((obj),   \
                             IS_TYPE_SENSOR,  \
                             IsSensorClass))

typedef struct _IsSensor      IsSensor;
typedef struct _IsSensorClass IsSensorClass;
typedef struct _IsSensorPrivate IsSensorPrivate;

struct _IsSensorClass
{
  GObjectClass parent_class;
  /* signals */
  void (*update_value)(IsSensor *sensor);
};

struct _IsSensor
{
  GObject parent;
  IsSensorPrivate *priv;
};

typedef enum
{
  IS_SENSOR_ALARM_MODE_DISABLED = 0,
  IS_SENSOR_ALARM_MODE_LOW,
  IS_SENSOR_ALARM_MODE_HIGH,
} IsSensorAlarmMode;

/* device icons */
#define IS_STOCK_CPU "indicator-sensors-cpu"
#define IS_STOCK_DISK "indicator-sensors-disk"
#define IS_STOCK_BATTERY "indicator-sensors-battery"
#define IS_STOCK_MEMORY "indicator-sensors-memory"
#define IS_STOCK_GPU "indicator-sensors-gpu"
#define IS_STOCK_CHIP "indicator-sensors-chip"
#define IS_STOCK_FAN "indicator-sensors-fan"
#define IS_STOCK_CASE "indicator-sensors-case"

#define IS_SENSOR_VALUE_UNSET (-G_MAXDOUBLE)

GType is_sensor_get_type(void) G_GNUC_CONST;
IsSensor *is_sensor_new(const gchar *path);
void is_sensor_update_value(IsSensor *self);
const gchar *is_sensor_get_path(IsSensor *self);
const gchar *is_sensor_get_label(IsSensor *self);
void is_sensor_set_label(IsSensor *self, const gchar *label);
const gchar *is_sensor_get_units(IsSensor *self);
void is_sensor_set_units(IsSensor *self, const gchar *units);
gdouble is_sensor_get_value(IsSensor *self);
void is_sensor_set_value(IsSensor *self, gdouble value);
guint is_sensor_get_digits(IsSensor *self);
void is_sensor_set_digits(IsSensor *self, guint digits);
gdouble is_sensor_get_alarm_value(IsSensor *self);
void is_sensor_set_alarm_value(IsSensor *self, gdouble limit);
IsSensorAlarmMode is_sensor_get_alarm_mode(IsSensor *self);
void is_sensor_set_alarm_mode(IsSensor *self, IsSensorAlarmMode mode);
guint is_sensor_get_update_interval(IsSensor *self);
void is_sensor_set_update_interval(IsSensor *self, guint update_interval);
gboolean is_sensor_get_alarmed(IsSensor *self);
const gchar *is_sensor_get_icon(IsSensor *self);
void is_sensor_set_icon(IsSensor *self, const gchar *icon);
gdouble is_sensor_get_low_value(IsSensor *self);
void is_sensor_set_low_value(IsSensor *self, gdouble value);
gdouble is_sensor_get_high_value(IsSensor *self);
void is_sensor_set_high_value(IsSensor *self, gdouble value);
const gchar *is_sensor_get_icon_path(IsSensor *self);
const gchar *is_sensor_get_error(IsSensor *self);
void is_sensor_set_error(IsSensor *self, const gchar *error);

void sensor_prepare_cache_icons();

G_END_DECLS

#endif /* __IS_SENSOR_H__ */
