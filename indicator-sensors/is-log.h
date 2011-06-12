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

#ifndef __IS_LOG_H__
#define __IS_LOG_H__

#include <glib.h>
#include <stdarg.h>

G_BEGIN_DECLS

typedef enum
{
	IS_LOG_LEVEL_ERROR = 0,
	IS_LOG_LEVEL_CRITICAL,
	IS_LOG_LEVEL_WARNING,
	IS_LOG_LEVEL_MESSAGE,
	IS_LOG_LEVEL_DEBUG,
	NUM_IS_LOG_LEVELS,
} IsLogLevel;

void is_log(const gchar *source,
	    IsLogLevel level,
	    const gchar *format,
	    ...) G_GNUC_PRINTF(3, 4);
void is_logv(const gchar *source,
	     IsLogLevel level,
	     const gchar *format,
	     va_list args);
#define is_error(source, ...) is_log(source, IS_LOG_LEVEL_ERROR, __VA_ARGS__)
#define is_critical(source, ...) is_log(source, IS_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#define is_warning(source, ...) is_log(source, IS_LOG_LEVEL_WARNING, __VA_ARGS__)
#define is_message(source, ...) is_log(source, IS_LOG_LEVEL_MESSAGE, __VA_ARGS__)
#define is_debug(source, ...) is_log(source, IS_LOG_LEVEL_DEBUG, __VA_ARGS__)

G_END_DECLS

#endif /* __IS_LOG_H__ */
