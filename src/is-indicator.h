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

#ifndef __IS_INDICATOR_H__
#define __IS_INDICATOR_H__

#include <libappindicator/app-indicator.h>
#include "is-manager.h"
#include "is-sensor.h"

G_BEGIN_DECLS

#define IS_TYPE_INDICATOR			\
	(is_indicator_get_type())
#define IS_INDICATOR(obj)				\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),		\
				    IS_TYPE_INDICATOR,	\
				    IsIndicator))
#define IS_INDICATOR_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),		\
				 IS_TYPE_INDICATOR,	\
				 IsIndicatorClass))
#define IS_IS_INDICATOR(obj)				\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),		\
				    IS_TYPE_INDICATOR))
#define IS_IS_INDICATOR_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),		\
				 IS_TYPE_INDICATOR))
#define IS_INDICATOR_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),		\
				   IS_TYPE_INDICATOR,	\
				   IsIndicatorClass))

typedef struct _IsIndicator      IsIndicator;
typedef struct _IsIndicatorClass IsIndicatorClass;
typedef struct _IsIndicatorPrivate IsIndicatorPrivate;

struct _IsIndicatorClass
{
	AppIndicatorClass parent_class;
};

struct _IsIndicator
{
	AppIndicator parent;
	IsIndicatorPrivate *priv;
};

GType is_indicator_get_type(void) G_GNUC_CONST;
IsIndicator *is_indicator_new(const gchar *id, const gchar *icon_name,
			      IsManager *manager);

G_END_DECLS

#endif /* __IS_INDICATOR_H__ */
