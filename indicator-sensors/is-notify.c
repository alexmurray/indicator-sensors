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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "is-log.h"
#include "is-notify.h"
#include <gtk/gtk.h>
#include <stdarg.h>
#include <stdio.h>
#include <libnotify/notify.h>
#include <libnotify/notification.h>

static gboolean inited = FALSE;
static gboolean append = FALSE;
static gboolean persistence = FALSE;

gboolean is_notify_init(void)
{
        if (!inited) {
                inited = notify_init(PACKAGE);
                if (inited) {
                        /* look for append capability */
                        GList *caps = notify_get_server_caps();
                        while (caps != NULL) {
                                gchar *cap = (gchar *)caps->data;
                                if (g_strcmp0(cap, "append") == 0) {
                                        append = TRUE;
                                }
                                if (g_strcmp0(cap, "persistence") == 0) {
                                        persistence = TRUE;
                                }
                                g_free(cap);
                                caps = g_list_delete_link(caps, caps);
                        }
                }
        }
        return inited;
}

void is_notify_uninit()
{
        if (inited) {
                notify_uninit();
        }
}

NotifyNotification *
is_notify(IsNotifyLevel level,
          const gchar *title,
          const gchar *format,
          ...)
{
        NotifyNotification *notification = NULL;
	va_list args;

        g_return_val_if_fail(inited, NULL);

	va_start(args, format);
	notification = is_notifyv(level, title, format, args);
	va_end(args);
        return notification;
}

static const gchar *
is_notify_level_to_icon(IsNotifyLevel level)
{
	const gchar *icon = NULL;

	g_return_val_if_fail((int)level >= IS_NOTIFY_LEVEL_ERROR &&
			     level < NUM_IS_NOTIFY_LEVELS, NULL);

	switch (level) {
	case IS_NOTIFY_LEVEL_ERROR:
		icon = GTK_STOCK_DIALOG_ERROR;
		break;

	case IS_NOTIFY_LEVEL_WARNING:
		icon = GTK_STOCK_DIALOG_WARNING;
		break;

	case IS_NOTIFY_LEVEL_INFO:
		icon = GTK_STOCK_DIALOG_INFO;
		break;

	case NUM_IS_NOTIFY_LEVELS:
	default:
		g_assert_not_reached();
	}

	return icon;
}

NotifyNotification *
is_notifyv(IsNotifyLevel level,
           const gchar *title,
           const gchar *format,
           va_list args)
{
        NotifyNotification *notification;
        gchar *body;

        g_return_val_if_fail(inited, NULL);

	body = g_strdup_vprintf(format, args);
        is_debug("notify", "Notify: %s - %s", title, body);
        notification = notify_notification_new(title,
                                               body,
                                               is_notify_level_to_icon(level));
        /* set app-name */
        g_object_set(G_OBJECT(notification), "app-name", PACKAGE, NULL);
        /* use default timeout and urgency */
        notify_notification_set_timeout(notification, NOTIFY_EXPIRES_DEFAULT);
        notify_notification_set_urgency(notification, NOTIFY_URGENCY_NORMAL);
        g_free(body);
        if (append) {
                is_debug("notify", "setting x-canonical-append TRUE");
                notify_notification_set_hint(notification, "x-canonical-append",
                                             g_variant_new_string(""));
        }
        if (persistence) {
                is_debug("notify", "setting resident TRUE and transient FALSE");
                notify_notification_set_hint(notification, "resident",
                                             g_variant_new_boolean(TRUE));
                notify_notification_set_hint(notification, "transient",
                                             g_variant_new_boolean(FALSE));
        }
        /* set desktop entry hint for gnome-shell */
        notify_notification_set_hint(notification, "desktop-entry",
                                     g_variant_new_string(PACKAGE));
        notify_notification_show(notification, NULL);
        return notification;
}
