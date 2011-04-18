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

#ifndef __IS_LIBNOTIFY_PLUGIN_H__
#define __IS_LIBNOTIFY_PLUGIN_H__

#include <libpeas/peas.h>


G_BEGIN_DECLS

#define IS_TYPE_LIBNOTIFY_PLUGIN		\
	(is_libnotify_plugin_get_type())
#define IS_LIBNOTIFY_PLUGIN(obj)				\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),			\
				    IS_TYPE_LIBNOTIFY_PLUGIN,	\
				    IsLibnotifyPlugin))
#define IS_LIBNOTIFY_PLUGIN_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),			\
				 IS_TYPE_LIBNOTIFY_PLUGIN,	\
				 IsLibnotifyPluginClass))
#define IS_IS_LIBNOTIFY_PLUGIN(obj)				\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),			\
				    IS_TYPE_LIBNOTIFY_PLUGIN))
#define IS_IS_LIBNOTIFY_PLUGIN_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),			\
				 IS_TYPE_LIBNOTIFY_PLUGIN))
#define IS_LIBNOTIFY_PLUGIN_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),			\
				   IS_TYPE_LIBNOTIFY_PLUGIN,	\
				   IsLibnotifyPluginClass))

typedef struct _IsLibnotifyPlugin      IsLibnotifyPlugin;
typedef struct _IsLibnotifyPluginClass IsLibnotifyPluginClass;
typedef struct _IsLibnotifyPluginPrivate IsLibnotifyPluginPrivate;

struct _IsLibnotifyPluginClass
{
	PeasExtensionBaseClass parent_class;
};

struct _IsLibnotifyPlugin
{
	PeasExtensionBase parent;
	IsLibnotifyPluginPrivate *priv;
};

GType is_libnotify_plugin_get_type(void) G_GNUC_CONST;
G_MODULE_EXPORT void peas_register_types(PeasObjectModule *module);

G_END_DECLS

#endif /* __IS_LIBNOTIFY_PLUGIN_H__ */
