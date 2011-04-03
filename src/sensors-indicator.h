/*
 * sensors-indicator.h - Header for SensorsIndicator
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __SENSORS_INDICATOR_H__
#define __SENSORS_INDICATOR_H__

#include <libappindicator/app-indicator.h>


G_BEGIN_DECLS

#define SENSORS_TYPE_INDICATOR			\
	(sensors_indicator_get_type())
#define SENSORS_INDICATOR(obj)					\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),			\
				     SENSORS_TYPE_INDICATOR,	\
				     SensorsIndicator))
#define SENSORS_INDICATOR_CLASS(klass)				\
	(G_TYPE_CHECK_CLASS_CAST((klass),			\
				  SENSORS_TYPE_INDICATOR,	\
				  SensorsIndicatorClass))
#define IS_SENSORS_INDICATOR(obj)				\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),			\
				     SENSORS_TYPE_INDICATOR))
#define IS_SENSORS_INDICATOR_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),			\
				  SENSORS_TYPE_INDICATOR))
#define SENSORS_INDICATOR_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),			\
				    SENSORS_TYPE_INDICATOR,	\
				    SensorsIndicatorClass))

typedef struct _SensorsIndicator      SensorsIndicator;
typedef struct _SensorsIndicatorClass SensorsIndicatorClass;
typedef struct _SensorsIndicatorPrivate SensorsIndicatorPrivate;

struct _SensorsIndicatorClass
{
	AppIndicatorClass parent_class;
};

struct _SensorsIndicator
{
	AppIndicator parent;
	SensorsIndicatorPrivate *priv;
};

GType sensors_indicator_get_type(void) G_GNUC_CONST;
AppIndicator *sensors_indicator_new();

G_END_DECLS

#endif /* __SENSORS_INDICATOR_H__ */
