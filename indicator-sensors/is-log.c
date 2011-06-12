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
#include <stdarg.h>
#include <stdio.h>

void is_log(const gchar *source,
	    IsLogLevel level,
	    const gchar *format,
	    ...)
{
	va_list args;

	va_start(args, format);
	is_logv(source, level, format, args);
	va_end(args);
}

static const gchar *
is_log_level_to_string(IsLogLevel level)
{
	const gchar *str_level = NULL;

	g_return_val_if_fail((int)level >= IS_LOG_LEVEL_ERROR &&
			     level < NUM_IS_LOG_LEVELS, NULL);

	switch (level) {
	case IS_LOG_LEVEL_ERROR:
		str_level = "ERROR";
		break;

	case IS_LOG_LEVEL_CRITICAL:
		str_level = "CRITICAL";
		break;

	case IS_LOG_LEVEL_WARNING:
		str_level = "WARNING";
		break;

	case IS_LOG_LEVEL_MESSAGE:
		str_level = "MESSAGE";
		break;

	case IS_LOG_LEVEL_DEBUG:
		str_level = "DEBUG";
		break;

	case NUM_IS_LOG_LEVELS:
	default:
		g_assert_not_reached();
	}

	return str_level;
}

void is_logv(const gchar *source,
	     IsLogLevel level,
	     const gchar *format,
	     va_list args)
{
	GLogLevelFlags flags;
	gchar *fmt, *output;

	switch (level) {
	case IS_LOG_LEVEL_ERROR:
		flags = G_LOG_LEVEL_ERROR;
		break;

	case IS_LOG_LEVEL_CRITICAL:
		flags = G_LOG_LEVEL_CRITICAL;
		break;

	case IS_LOG_LEVEL_WARNING:
		flags = G_LOG_LEVEL_WARNING;
		break;

	case IS_LOG_LEVEL_MESSAGE:
		flags = G_LOG_LEVEL_MESSAGE;
		break;

	case IS_LOG_LEVEL_DEBUG:
		flags = G_LOG_LEVEL_DEBUG;
		break;

	case NUM_IS_LOG_LEVELS:
	default:
		g_assert_not_reached();
		return;
	}

	/* TODO: perhaps add a more elaborate logger */
	fmt = g_strdup_printf("[%s] %s: %s", source,
			      is_log_level_to_string(level), format);
	output = g_strdup_vprintf(fmt, args);
	printf("%s\n", output);
	g_free(output);
	g_free(fmt);
}
