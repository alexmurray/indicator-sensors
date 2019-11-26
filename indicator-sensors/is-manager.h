/*
 * Copyright (C) 2011-2019 Alex Murray <murray.alex@gmail.com>
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

#ifndef __IS_MANAGER_H__
#define __IS_MANAGER_H__

#include <gtk/gtk.h>
#include "is-store.h"
#include "is-temperature-sensor.h"

G_BEGIN_DECLS

#define IS_TYPE_MANAGER     \
  (is_manager_get_type())
#define IS_MANAGER(obj)         \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),      \
                              IS_TYPE_MANAGER,  \
                              IsManager))
#define IS_MANAGER_CLASS(klass)       \
  (G_TYPE_CHECK_CLASS_CAST((klass),     \
                           IS_TYPE_MANAGER, \
                           IsManagerClass))
#define IS_IS_MANAGER(obj)        \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),      \
                              IS_TYPE_MANAGER))
#define IS_IS_MANAGER_CLASS(klass)      \
  (G_TYPE_CHECK_CLASS_TYPE((klass),     \
                           IS_TYPE_MANAGER))
#define IS_MANAGER_GET_CLASS(obj)     \
  (G_TYPE_INSTANCE_GET_CLASS((obj),     \
                             IS_TYPE_MANAGER, \
                             IsManagerClass))

typedef struct _IsManager      IsManager;
typedef struct _IsManagerClass IsManagerClass;
typedef struct _IsManagerPrivate IsManagerPrivate;

struct _IsManagerClass
{
  GtkTreeViewClass parent_class;
};

struct _IsManager
{
  GtkTreeView parent;
  IsManagerPrivate *priv;
};

GType is_manager_get_type(void) G_GNUC_CONST;
IsManager *is_manager_new(void);
gboolean is_manager_add_sensor(IsManager *self,
                               IsSensor *sensor);
gboolean is_manager_remove_paths_with_prefix(IsManager *self,
                                             const gchar *prefix);
gboolean is_manager_remove_path(IsManager *self,
                                const gchar *path);
IsSensor *is_manager_get_sensor(IsManager *self,
                                const gchar *path);
GSList *is_manager_get_all_sensors_list(IsManager *self);
GSList *is_manager_get_enabled_sensors_list(IsManager *self);
guint is_manager_get_num_enabled_sensors(IsManager *self);
gchar **is_manager_get_enabled_sensors(IsManager *self);
gboolean is_manager_set_enabled_sensors(IsManager *self,
                                        const gchar **enabled_sensors);
IsSensor *is_manager_get_selected_sensor(IsManager *self);

G_END_DECLS

#endif /* __IS_MANAGER_H__ */
