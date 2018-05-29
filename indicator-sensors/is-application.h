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

#ifndef __IS_APPLICATION_H__
#define __IS_APPLICATION_H__

#include <glib.h>
#include "is-manager.h"

G_BEGIN_DECLS

#define IS_TYPE_APPLICATION     \
  (is_application_get_type())
#define IS_APPLICATION(obj)       \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),    \
                              IS_TYPE_APPLICATION,  \
                              IsApplication))
#define IS_APPLICATION_CLASS(klass)     \
  (G_TYPE_CHECK_CLASS_CAST((klass),   \
                           IS_TYPE_APPLICATION, \
                           IsApplicationClass))
#define IS_IS_APPLICATION(obj)        \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),    \
                              IS_TYPE_APPLICATION))
#define IS_IS_APPLICATION_CLASS(klass)      \
  (G_TYPE_CHECK_CLASS_TYPE((klass),   \
                           IS_TYPE_APPLICATION))
#define IS_APPLICATION_GET_CLASS(obj)     \
  (G_TYPE_INSTANCE_GET_CLASS((obj),   \
                             IS_TYPE_APPLICATION, \
                             IsApplicationClass))

typedef struct _IsApplication      IsApplication;
typedef struct _IsApplicationClass IsApplicationClass;
typedef struct _IsApplicationPrivate IsApplicationPrivate;

struct _IsApplicationClass
{
  GObjectClass parent_class;
};

struct _IsApplication
{
  GObject parent;
  IsApplicationPrivate *priv;
};

GType is_application_get_type(void) G_GNUC_CONST;
IsApplication *is_application_new(void);
IsManager *is_application_get_manager(IsApplication *self);
void is_application_set_show_indicator(IsApplication *self,
                                       gboolean show_indicator);
gboolean is_application_get_show_indicator(IsApplication *self);
guint is_application_get_poll_timeout(IsApplication *self);
void is_application_set_poll_timeout(IsApplication *self, guint poll_timeout);
IsTemperatureSensorScale is_application_get_temperature_scale(IsApplication *self);
void is_application_set_temperature_scale(IsApplication *self,
    IsTemperatureSensorScale scale);
void is_application_show_preferences(IsApplication *self);
void is_application_show_about(IsApplication *self);
void is_application_quit(IsApplication *self);

G_END_DECLS

#endif /* __IS_APPLICATION_H__ */
