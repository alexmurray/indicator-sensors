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

#include "is-log.h"
#include "is-notify.h"
#include <stdarg.h>
#include <stdio.h>

GNotification *
is_notify(IsNotifyLevel level, const gchar *title, const gchar *format, ...)
{
  GNotification *notification = NULL;
  va_list args;

  va_start(args, format);
  notification = is_notifyv(level, title, format, args);
  va_end(args);
  return notification;
}

static GNotificationPriority
is_notify_level_to_priority(IsNotifyLevel level)
{
  GNotificationPriority priority = G_NOTIFICATION_PRIORITY_NORMAL;

  g_return_val_if_fail((int)level >= IS_NOTIFY_LEVEL_ERROR &&
                           level < NUM_IS_NOTIFY_LEVELS,
                       G_NOTIFICATION_PRIORITY_NORMAL);

  switch (level)
  {
  case IS_NOTIFY_LEVEL_ERROR:
    priority = G_NOTIFICATION_PRIORITY_HIGH;
    break;
  case IS_NOTIFY_LEVEL_WARNING:
    priority = G_NOTIFICATION_PRIORITY_NORMAL;
    break;
  case IS_NOTIFY_LEVEL_INFO:
    priority = G_NOTIFICATION_PRIORITY_LOW;
    break;
  case NUM_IS_NOTIFY_LEVELS:
  default:
    g_assert_not_reached();
  }
  return priority;
}

static const gchar *
is_notify_level_to_icon(IsNotifyLevel level)
{
  const gchar *icon = NULL;

  g_return_val_if_fail((int)level >= IS_NOTIFY_LEVEL_ERROR &&
                           level < NUM_IS_NOTIFY_LEVELS,
                       NULL);

  switch (level)
  {
  case IS_NOTIFY_LEVEL_ERROR:
    icon = "dialog-error";
    break;

  case IS_NOTIFY_LEVEL_WARNING:
    icon = "dialog-warning";
    break;

  case IS_NOTIFY_LEVEL_INFO:
    icon = "dialog-information";
    break;

  case NUM_IS_NOTIFY_LEVELS:
  default:
    g_assert_not_reached();
  }

  return icon;
}

GNotification *
is_notifyv(IsNotifyLevel level, const gchar *title, const gchar *format,
           va_list args)
{
  GNotification *notification;
  gchar *body;
  GIcon *icon;

  body = g_strdup_vprintf(format, args);
  is_debug("notify", "Notify: %s - %s", title, body);
  notification = g_notification_new(title);
  g_notification_set_body(notification, body);
  g_notification_set_priority(notification, is_notify_level_to_priority(level));
  icon = g_themed_icon_new(is_notify_level_to_icon(level));
  g_notification_set_icon(notification, icon);
  g_object_unref(icon);
  g_free(body);
  return notification;
}
