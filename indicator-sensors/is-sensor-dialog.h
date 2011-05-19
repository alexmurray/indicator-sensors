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

#ifndef __IS_SENSOR_DIALOG_H__
#define __IS_SENSOR_DIALOG_H__

#include <gtk/gtk.h>
#include "is-sensor.h"

G_BEGIN_DECLS

#define IS_TYPE_SENSOR_DIALOG		\
	(is_sensor_dialog_get_type())
#define IS_SENSOR_DIALOG(obj)				\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),			\
				    IS_TYPE_SENSOR_DIALOG,	\
				    IsSensorDialog))
#define IS_SENSOR_DIALOG_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),			\
				 IS_TYPE_SENSOR_DIALOG,	\
				 IsSensorDialogClass))
#define IS_IS_SENSOR_DIALOG(obj)                                   \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),				\
				    IS_TYPE_SENSOR_DIALOG))
#define IS_IS_SENSOR_DIALOG_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),			\
				 IS_TYPE_SENSOR_DIALOG))
#define IS_SENSOR_DIALOG_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),			\
				   IS_TYPE_SENSOR_DIALOG,	\
				   IsSensorDialogClass))

typedef struct _IsSensorDialog      IsSensorDialog;
typedef struct _IsSensorDialogClass IsSensorDialogClass;
typedef struct _IsSensorDialogPrivate IsSensorDialogPrivate;

struct _IsSensorDialogClass
{
	GtkDialogClass parent_class;
};

struct _IsSensorDialog
{
	GtkDialog parent;
	IsSensorDialogPrivate *priv;
};

GType is_sensor_dialog_get_type(void) G_GNUC_CONST;
GtkWidget *is_sensor_dialog_new(IsSensor *sensor);

G_END_DECLS

#endif /* __IS_SENSOR_DIALOG_H__ */
