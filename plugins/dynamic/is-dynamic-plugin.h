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

#ifndef __IS_DYNAMIC_PLUGIN_H__
#define __IS_DYNAMIC_PLUGIN_H__

#include <libpeas/peas.h>


G_BEGIN_DECLS

#define IS_TYPE_DYNAMIC_PLUGIN   \
  (is_dynamic_plugin_get_type())
#define IS_DYNAMIC_PLUGIN(obj)       \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),      \
                              IS_TYPE_DYNAMIC_PLUGIN,  \
                              IsDynamicPlugin))
#define IS_DYNAMIC_PLUGIN_CLASS(klass)     \
  (G_TYPE_CHECK_CLASS_CAST((klass),     \
                           IS_TYPE_DYNAMIC_PLUGIN, \
                           IsDynamicPluginClass))
#define IS_IS_DYNAMIC_PLUGIN(obj)        \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),      \
                              IS_TYPE_DYNAMIC_PLUGIN))
#define IS_IS_DYNAMIC_PLUGIN_CLASS(klass)      \
  (G_TYPE_CHECK_CLASS_TYPE((klass),     \
                           IS_TYPE_DYNAMIC_PLUGIN))
#define IS_DYNAMIC_PLUGIN_GET_CLASS(obj)     \
  (G_TYPE_INSTANCE_GET_CLASS((obj),     \
                             IS_TYPE_DYNAMIC_PLUGIN, \
                             IsDynamicPluginClass))

typedef struct _IsDynamicPlugin      IsDynamicPlugin;
typedef struct _IsDynamicPluginClass IsDynamicPluginClass;
typedef struct _IsDynamicPluginPrivate IsDynamicPluginPrivate;

struct _IsDynamicPluginClass
{
  PeasExtensionBaseClass parent_class;
};

struct _IsDynamicPlugin
{
  PeasExtensionBase parent;
  IsDynamicPluginPrivate *priv;
};

GType is_dynamic_plugin_get_type(void) G_GNUC_CONST;
G_MODULE_EXPORT void peas_register_types(PeasObjectModule *module);

G_END_DECLS

#endif /* __IS_DYNAMIC_PLUGIN_H__ */
