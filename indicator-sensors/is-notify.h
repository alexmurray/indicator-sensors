/*
 * Copyright (C) 2011 Alex Murray <alexmurray@fastmail.fm>
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

#ifndef __IS_NOTIFY_H__
#define __IS_NOTIFY_H__

#include <glib.h>
#include <stdarg.h>
#include <libnotify/notification.h>

G_BEGIN_DECLS

typedef enum
{
	IS_NOTIFY_LEVEL_ERROR = 0,
	IS_NOTIFY_LEVEL_WARNING,
	IS_NOTIFY_LEVEL_INFO,
	NUM_IS_NOTIFY_LEVELS,
} IsNotifyLevel;

gboolean is_notify_init(void);
void is_notify_uninit(void);
NotifyNotification *is_notify(IsNotifyLevel level,
                              const gchar *title,
                              const gchar *format,
                              ...) G_GNUC_PRINTF(3, 4);
NotifyNotification *is_notifyv(IsNotifyLevel level,
                               const gchar *title,
                               const gchar *format,
                               va_list args);
G_END_DECLS

#endif /* __IS_LOG_H__ */
