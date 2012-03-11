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

void is_notify(IsNotifyLevel level,
               const gchar *title,
               const gchar *format,
               ...)
{
	va_list args;

        g_return_if_fail(inited);

	va_start(args, format);
	is_notifyv(level, title, format, args);
	va_end(args);
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

void is_notifyv(IsNotifyLevel level,
                const gchar *title,
                const gchar *format,
                va_list args)
{
        NotifyNotification *notification;
        gchar *body;

        g_return_if_fail(inited);

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
                GVariant *cap = g_variant_new("s", "");
                notify_notification_set_hint(notification, "x-canonical-append", cap);
        }
        notify_notification_show(notification, NULL);
        g_object_unref(notification);
}
