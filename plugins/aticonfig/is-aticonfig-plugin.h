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

#ifndef __IS_ATICONFIG_PLUGIN_H__
#define __IS_ATICONFIG_PLUGIN_H__

#include <libpeas/peas.h>


G_BEGIN_DECLS

#define IS_TYPE_ATICONFIG_PLUGIN		\
	(is_aticonfig_plugin_get_type())
#define IS_ATICONFIG_PLUGIN(obj)				\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),			\
				    IS_TYPE_ATICONFIG_PLUGIN,	\
				    IsATIConfigPlugin))
#define IS_ATICONFIG_PLUGIN_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),			\
				 IS_TYPE_ATICONFIG_PLUGIN,	\
				 IsATIConfigPluginClass))
#define IS_IS_ATICONFIG_PLUGIN(obj)				\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),			\
				    IS_TYPE_ATICONFIG_PLUGIN))
#define IS_IS_ATICONFIG_PLUGIN_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),			\
				 IS_TYPE_ATICONFIG_PLUGIN))
#define IS_ATICONFIG_PLUGIN_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),			\
				   IS_TYPE_ATICONFIG_PLUGIN,	\
				   IsATIConfigPluginClass))

typedef struct _IsATIConfigPlugin      IsATIConfigPlugin;
typedef struct _IsATIConfigPluginClass IsATIConfigPluginClass;
typedef struct _IsATIConfigPluginPrivate IsATIConfigPluginPrivate;

struct _IsATIConfigPluginClass
{
	PeasExtensionBaseClass parent_class;
};

struct _IsATIConfigPlugin
{
	PeasExtensionBase parent;
	IsATIConfigPluginPrivate *priv;
};

GType is_aticonfig_plugin_get_type(void) G_GNUC_CONST;
G_MODULE_EXPORT void peas_register_types(PeasObjectModule *module);

G_END_DECLS

#endif /* __IS_ATICONFIG_PLUGIN_H__ */
