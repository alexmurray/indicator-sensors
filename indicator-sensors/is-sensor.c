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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <math.h>
#include "is-sensor.h"
#include "is-notify.h"
#include "is-log.h"

G_DEFINE_TYPE (IsSensor, is_sensor, G_TYPE_OBJECT);

static void is_sensor_finalize(GObject *object);
static void is_sensor_get_property(GObject *object,
                                   guint property_id, GValue *value, GParamSpec *pspec);
static void is_sensor_set_property(GObject *object,
                                   guint property_id, const GValue *value, GParamSpec *pspec);

/* signal enum */
enum
{
  SIGNAL_UPDATE_VALUE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

/* properties */
enum
{
  PROP_0,
  PROP_PATH,
  PROP_LABEL,
  PROP_VALUE,
  PROP_DIGITS,
  PROP_ALARM_VALUE,
  PROP_ALARM_MODE,
  PROP_UNITS,
  PROP_UPDATE_INTERVAL,
  PROP_ALARMED,
  PROP_ICON,
  PROP_LOW_VALUE,
  PROP_HIGH_VALUE,
  PROP_ICON_PATH,
  PROP_ERROR,
  LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = {NULL};

struct _IsSensorPrivate
{
  gchar *path;
  gchar *label;
  gdouble value;
  guint digits;
  gdouble alarm_value;
  IsSensorAlarmMode alarm_mode;
  gchar *units;
  guint update_interval;
  gint64 last_update;
  gboolean alarmed;
  gchar *icon;
  gdouble low_value;
  gdouble high_value;
  guint enable_alarm_id;
  guint disable_alarm_id;
  gchar *icon_path;
  gchar *error;
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
  properties[PROP_LABEL] = g_param_spec_string("label", "sensor label",
                           "label of this sensor.",
                           NULL,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_LABEL, properties[PROP_LABEL]);
  properties[PROP_VALUE] = g_param_spec_double("value", "sensor value",
                           "value of this sensor.",
                           -G_MAXDOUBLE, G_MAXDOUBLE, IS_SENSOR_VALUE_UNSET,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_VALUE, properties[PROP_VALUE]);
  properties[PROP_DIGITS] = g_param_spec_uint("digits", "number of digits for this sensor",
                            "the number of decimal places to display for this sensor.",
                            0, G_MAXUINT, 1,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_DIGITS, properties[PROP_DIGITS]);
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
  properties[PROP_ICON] = g_param_spec_string("icon", "sensor icon",
                          "icon of this sensor.",
                          IS_STOCK_CHIP,
                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_ICON, properties[PROP_ICON]);
  properties[PROP_LOW_VALUE] = g_param_spec_double("low-value", "sensor low value",
                               "low value of this sensor.",
                               -G_MAXDOUBLE, G_MAXDOUBLE,
                               0.0,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_LOW_VALUE, properties[PROP_LOW_VALUE]);
  properties[PROP_HIGH_VALUE] = g_param_spec_double("high-value", "sensor high value",
                                "high value of this sensor.",
                                -G_MAXDOUBLE, G_MAXDOUBLE,
                                0.0,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_HIGH_VALUE, properties[PROP_HIGH_VALUE]);
  properties[PROP_ICON_PATH] = g_param_spec_string("icon-path",
                               "sensor icon path",
                               "path to the icon of this sensor.",
                               IS_STOCK_CHIP,
                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_ICON_PATH,
                                  properties[PROP_ICON_PATH]);
  properties[PROP_ERROR] = g_param_spec_string("error",
                           "sensor error",
                           "error of this sensor.",
                           NULL,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property(gobject_class, PROP_ERROR,
                                  properties[PROP_ERROR]);

  signals[SIGNAL_UPDATE_VALUE] = g_signal_new("update-value",
                                 G_OBJECT_CLASS_TYPE(klass),
                                 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                 offsetof(IsSensorClass, update_value),
                                 NULL, NULL,
                                 g_cclosure_marshal_VOID__VOID,
                                 G_TYPE_NONE, 0);
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

  switch (property_id)
  {
    case PROP_PATH:
      g_value_set_string(value, is_sensor_get_path(self));
      break;
    case PROP_LABEL:
      g_value_set_string(value, is_sensor_get_label(self));
      break;
    case PROP_VALUE:
      g_value_set_double(value, is_sensor_get_value(self));
      break;
    case PROP_DIGITS:
      g_value_set_uint(value, is_sensor_get_digits(self));
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
    case PROP_ICON:
      g_value_set_string(value, is_sensor_get_icon(self));
      break;
    case PROP_LOW_VALUE:
      g_value_set_double(value, is_sensor_get_low_value(self));
      break;
    case PROP_HIGH_VALUE:
      g_value_set_double(value, is_sensor_get_high_value(self));
      break;
    case PROP_ICON_PATH:
      g_value_set_string(value, is_sensor_get_icon_path(self));
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

  switch (property_id)
  {
    case PROP_PATH:
      priv->path = g_value_dup_string(value);
      break;
    case PROP_LABEL:
      is_sensor_set_label(self, g_value_get_string(value));
      break;
    case PROP_VALUE:
      is_sensor_set_value(self, g_value_get_double(value));
      break;
    case PROP_DIGITS:
      is_sensor_set_digits(self, g_value_get_uint(value));
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
    case PROP_ICON:
      is_sensor_set_icon(self, g_value_get_string(value));
      break;
    case PROP_LOW_VALUE:
      is_sensor_set_low_value(self, g_value_get_double(value));
      break;
    case PROP_HIGH_VALUE:
      is_sensor_set_high_value(self, g_value_get_double(value));
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
                      "digits", 1,
                      "value", IS_SENSOR_VALUE_UNSET,
                      "alarm-value", 0.0,
                      "alarm-mode", IS_SENSOR_ALARM_MODE_DISABLED,
                      "units", "?",
                      "update-interval", 5,
                      "icon", IS_STOCK_CHIP,
                      "low-value", 0.0,
                      "high-value", 0.0,
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

  if (label != NULL && g_strcmp0(label, "") != 0)
  {
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

static gboolean
enable_alarm(IsSensor *self)
{
  NotifyNotification *notification;

  g_return_val_if_fail(self != NULL, FALSE);

  notification = is_notify(IS_NOTIFY_LEVEL_WARNING,
                           _("Sensor Alarm"),
                           "%s: %2.1f%s",
                           is_sensor_get_label(self),
                           is_sensor_get_value(self),
                           is_sensor_get_units(self));
  is_debug("sensor", "Displaying alarm notification");
  g_object_set_data_full(G_OBJECT(self), "notification",
                         notification, g_object_unref);
  self->priv->alarmed = TRUE;
  g_object_notify_by_pspec(G_OBJECT(self),
                           properties[PROP_ALARMED]);
  self->priv->enable_alarm_id = 0;
  /* remove as source */
  return FALSE;
}

static gboolean
disable_alarm(IsSensor *self)
{
  /* hide any notification */
  NotifyNotification *notification;

  g_return_val_if_fail(self != NULL, FALSE);

  notification = g_object_get_data(G_OBJECT(self), "notification");
  is_debug("sensor", "Closing alarm notification");
  notify_notification_close(notification, NULL);
  g_object_set_data(G_OBJECT(self), "notification",
                    NULL);
  self->priv->alarmed = FALSE;
  g_object_notify_by_pspec(G_OBJECT(self),
                           properties[PROP_ALARMED]);
  self->priv->disable_alarm_id = 0;
  return FALSE;
}


typedef enum
{
  VERY_LOW_SENSOR_VALUE = 0,
  LOW_SENSOR_VALUE,
  NORMAL_SENSOR_VALUE,
  HIGH_SENSOR_VALUE,
  VERY_HIGH_SENSOR_VALUE
} SensorValueRange;

/* Cast a given value to a valid SensorValueRange */
#define SENSOR_VALUE_RANGE(x) ((SensorValueRange)(CLAMP(x, VERY_LOW_SENSOR_VALUE, VERY_HIGH_SENSOR_VALUE)))

static gdouble sensor_value_range_normalised(gdouble value,
    gdouble low_value,
    gdouble high_value)
{
  return ((value - low_value)/(high_value - low_value));
}

static SensorValueRange sensor_value_range(gdouble sensor_value,
    gdouble low_value,
    gdouble high_value)
{
  gdouble range;
  range = sensor_value_range_normalised(sensor_value, low_value, high_value)*(gdouble)(VERY_HIGH_SENSOR_VALUE);

  /* check if need to round up, otherwise let int conversion
   * round down for us and make sure it is a valid range
   * value */
  return SENSOR_VALUE_RANGE(((gint)range + ((range - ((gint)range)) >= 0.5)));
}

#define DEFAULT_ICON_SIZE 22

#define NUM_OVERLAY_ICONS 5

static const gchar * const value_overlay_icons[NUM_OVERLAY_ICONS] =
{
  "very-low-value-icon",
  "low-value-icon",
  "normal-value-icon",
  "high-value-icon",
  "very-high-value-icon"
};

static const gchar *value_overlay_icon(gdouble value,
                                       gdouble low,
                                       gdouble high)
{
  SensorValueRange value_range = sensor_value_range(value, low, high);
  return value_overlay_icons[value_range];
}

static gboolean
prepare_cache_icon(const gchar *base_icon_name,
                   const gchar *overlay_name,
                   const gchar *icon_path,
                   GError **error)
{
  GdkPixbuf *base_icon, *overlay_icon, *new_icon;
  gchar *icon_dir;
  GtkIconTheme *icon_theme;
  gboolean ret;

  ret = g_file_test(icon_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);

  if (ret)
  {
    goto out;
  }

  icon_theme = gtk_icon_theme_get_default();
  base_icon = gtk_icon_theme_load_icon(icon_theme,
                                       base_icon_name,
                                       DEFAULT_ICON_SIZE,
                                       DEFAULT_ICON_SIZE,
                                       error);
  if (!base_icon)
  {
    goto out;
  }
  overlay_icon = gtk_icon_theme_load_icon(icon_theme,
                                          overlay_name,
                                          DEFAULT_ICON_SIZE,
                                          DEFAULT_ICON_SIZE,
                                          error);

  if (!overlay_icon)
  {
    g_object_unref(base_icon);
    goto out;
  }

  new_icon = gdk_pixbuf_copy(base_icon);
  gdk_pixbuf_composite(overlay_icon, new_icon,
                       0, 0,
                       DEFAULT_ICON_SIZE, DEFAULT_ICON_SIZE,
                       0, 0,
                       1.0, 1.0,
                       GDK_INTERP_BILINEAR,
                       255);
  g_object_unref(overlay_icon);

  /* ensure path to icon exists */
  icon_dir = g_path_get_dirname(icon_path);
  g_mkdir_with_parents(icon_dir, 0755);
  g_free(icon_dir);

  /* write out icon */
  ret = gdk_pixbuf_save(new_icon, icon_path, "png", error, NULL);
  g_object_unref(new_icon);
  g_object_unref(base_icon);

out:
  return ret;
}

static gchar *
get_icon_path(IsSensor *self)
{
  gdouble value, low, high;
  const gchar *base_name, *overlay_name;
  gchar *icon_name, *icon_path = NULL, *cache_dir;
  gboolean ret;
  GError *error = NULL;

  base_name = is_sensor_get_icon(self);
  value = is_sensor_get_value(self);
  low = is_sensor_get_low_value(self);
  high = is_sensor_get_high_value(self);

  /* if no base icon return NULL */
  if (!base_name)
  {
    is_warning("sensor", "No base icon for sensor %s [%s]",
               is_sensor_get_label(self),
               is_sensor_get_path(self));
    icon_path = NULL;
    goto out;
  }

  /* no range - return base icon */
  if (fabs(low - high) <= DBL_EPSILON)
  {
    /* use base_name */
    icon_path = g_strdup(base_name);
    goto out;
  }

  overlay_name = value_overlay_icon(value, low, high);
  cache_dir = g_build_filename(g_get_user_cache_dir(),
                               PACKAGE, "icons", NULL);
  icon_name = g_strdup_printf("%s-%s.png", base_name, overlay_name);
  icon_path = g_build_filename(cache_dir, icon_name, NULL);
  g_free(icon_name);
  g_free(cache_dir);

  /* prepare cache icon */
  ret = prepare_cache_icon(base_name, overlay_name, icon_path, &error);
  if (!ret)
  {
    is_warning("sensor", "Couldn't create cache icon %s from base %s and overlay %s for sensor %s: %s",
               icon_path, base_name, overlay_name,
               is_sensor_get_path(self),
               error ? error->message : "NO ERROR");
    g_clear_error(&error);
    /* return base_name instead */
    g_free(icon_path);
    icon_path = g_strdup(base_name);
    goto out;
  }

out:
  return icon_path;
}

static void
update_icon_path(IsSensor *self)
{
  gchar *icon_path = get_icon_path(self);
  if (g_strcmp0(icon_path, self->priv->icon_path))
  {
    g_free(self->priv->icon_path);
    self->priv->icon_path = icon_path;
    icon_path = NULL;
    g_object_notify_by_pspec(G_OBJECT(self),
                             properties[PROP_ICON_PATH]);
  }
  g_free(icon_path);
}

static void
update_alarmed(IsSensor *self)
{
  IsSensorPrivate *priv = self->priv;
  gboolean alarmed = FALSE;

  switch (priv->alarm_mode)
  {
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

  /* override alarmed if value hasn't been set yet */
  if (!priv->last_update)
  {
    alarmed = FALSE;
  }

  if (priv->alarmed != alarmed)
  {
    GSettings *settings;
    gint delay;

    settings = g_settings_new("indicator-sensors.sensor");
    delay = g_settings_get_int(settings, "alarm-delay");

    g_object_unref(settings);
    /* use a hysteresis value so we don't become alarmed too
     * quickly for fluctuating values */
    if (priv->alarmed)
    {
      if (priv->enable_alarm_id)
      {
        g_source_remove(priv->enable_alarm_id);
        priv->enable_alarm_id = 0;
      }
      if (!priv->disable_alarm_id)
      {
        priv->disable_alarm_id =
          g_timeout_add_seconds(delay,
                                (GSourceFunc)disable_alarm,
                                self);
        is_debug("sensor", "Alarm triggered - will %sable in %d seconds",
                 alarmed ? "en" : "dis", delay);
      }
    }
    else
    {
      if (priv->disable_alarm_id)
      {
        g_source_remove(priv->disable_alarm_id);
        priv->disable_alarm_id = 0;
      }
      if (!priv->enable_alarm_id)
      {
        priv->enable_alarm_id =
          g_timeout_add_seconds(delay,
                                (GSourceFunc)enable_alarm,
                                self);
        is_debug("sensor", "Alarm triggered - will %sable in %d seconds",
                 alarmed ? "en" : "dis", delay);
      }
    }
  }
}

void
is_sensor_set_value(IsSensor *self,
                    gdouble value)
{
  IsSensorPrivate *priv;

  g_return_if_fail(IS_IS_SENSOR(self));

  priv = self->priv;

  if (fabs(priv->value - value) > DBL_EPSILON)
  {
    priv->value = value;
    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_VALUE]);
    update_alarmed(self);
    update_icon_path(self);
  }
}

guint
is_sensor_get_digits(IsSensor *self)
{
  g_return_val_if_fail(IS_IS_SENSOR(self), 0);
  return self->priv->digits;
}

void
is_sensor_set_digits(IsSensor *self,
                     guint digits)
{
  IsSensorPrivate *priv;

  g_return_if_fail(IS_IS_SENSOR(self));

  priv = self->priv;

  if (digits != priv->digits)
  {
    priv->digits = digits;
    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_DIGITS]);
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

  if (fabs(priv->alarm_value - alarm_value) > DBL_EPSILON)
  {
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

  if (priv->alarm_mode != alarm_mode)
  {
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
  if (self->priv->update_interval != update_interval)
  {
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
      secs - priv->last_update >= priv->update_interval)
  {
    priv->last_update = secs;
    g_signal_emit(self, signals[SIGNAL_UPDATE_VALUE], 0);
  }
}

const gchar *
is_sensor_get_icon(IsSensor *self)
{
  g_return_val_if_fail(IS_IS_SENSOR(self), NULL);
  return self->priv->icon;
}

void
is_sensor_set_icon(IsSensor *self,
                   const gchar *icon)
{
  g_return_if_fail(IS_IS_SENSOR(self));
  g_free(self->priv->icon);
  self->priv->icon = icon ? g_strdup(icon) : NULL;
  g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ICON]);
  update_icon_path(self);
}

gdouble
is_sensor_get_low_value(IsSensor *self)
{
  g_return_val_if_fail(IS_IS_SENSOR(self), 0.0f);
  return self->priv->low_value;
}

void
is_sensor_set_low_value(IsSensor *self,
                        gdouble low_value)
{
  IsSensorPrivate *priv;

  g_return_if_fail(IS_IS_SENSOR(self));

  priv = self->priv;

  if (fabs(priv->low_value - low_value) > DBL_EPSILON)
  {
    priv->low_value = low_value;
    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_LOW_VALUE]);
    update_icon_path(self);
  }
}

gdouble
is_sensor_get_high_value(IsSensor *self)
{
  g_return_val_if_fail(IS_IS_SENSOR(self), 0.0f);
  return self->priv->high_value;
}

void
is_sensor_set_high_value(IsSensor *self,
                         gdouble high_value)
{
  IsSensorPrivate *priv;

  g_return_if_fail(IS_IS_SENSOR(self));

  priv = self->priv;

  if (fabs(priv->high_value - high_value) > DBL_EPSILON)
  {
    priv->high_value = high_value;
    g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_HIGH_VALUE]);
    update_icon_path(self);
  }
}

const gchar *
is_sensor_get_icon_path(IsSensor *self)
{
  g_return_val_if_fail(IS_IS_SENSOR(self), NULL);
  return self->priv->icon_path;
}

const gchar *
is_sensor_get_error(IsSensor *self)
{
  g_return_val_if_fail(IS_IS_SENSOR(self), NULL);
  return self->priv->error;
}

void
is_sensor_set_error(IsSensor *self,
                    const gchar *error)
{
  NotifyNotification *notification;

  g_return_if_fail(IS_IS_SENSOR(self));

  notification = g_object_get_data(G_OBJECT(self), "error-notification");
  if (notification != NULL)
  {
    is_debug("sensor", "Closing existing error notification");
    notify_notification_close(notification, NULL);
  }
  g_free(self->priv->error);
  self->priv->error = error ? g_strdup(error) : NULL;

  if (self->priv->error)
  {
    notification = is_notify(IS_NOTIFY_LEVEL_WARNING,
                             _("Sensor Error"),
                             "%s",
                             self->priv->error);
    is_debug("sensor", "Displaying error notification");
    g_object_set_data_full(G_OBJECT(self), "error-notification",
                           notification, g_object_unref);
  }
  g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ERROR]);
}
