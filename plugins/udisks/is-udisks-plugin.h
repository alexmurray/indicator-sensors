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

#ifndef __IS_UDISKS_PLUGIN_H__
#define __IS_UDISKS_PLUGIN_H__

#include <libpeas/peas.h>


G_BEGIN_DECLS

#define IS_TYPE_UDISKS_PLUGIN		\
	(is_udisks_plugin_get_type())
#define IS_UDISKS_PLUGIN(obj)				\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),			\
				    IS_TYPE_UDISKS_PLUGIN,	\
				    IsUdisksPlugin))
#define IS_UDISKS_PLUGIN_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),			\
				 IS_TYPE_UDISKS_PLUGIN,	\
				 IsUdisksPluginClass))
#define IS_IS_UDISKS_PLUGIN(obj)				\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),			\
				    IS_TYPE_UDISKS_PLUGIN))
#define IS_IS_UDISKS_PLUGIN_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),			\
				 IS_TYPE_UDISKS_PLUGIN))
#define IS_UDISKS_PLUGIN_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),			\
				   IS_TYPE_UDISKS_PLUGIN,	\
				   IsUdisksPluginClass))

typedef struct _IsUdisksPlugin      IsUdisksPlugin;
typedef struct _IsUdisksPluginClass IsUdisksPluginClass;
typedef struct _IsUdisksPluginPrivate IsUdisksPluginPrivate;

struct _IsUdisksPluginClass
{
	PeasExtensionBaseClass parent_class;
};

struct _IsUdisksPlugin
{
	PeasExtensionBase parent;
	IsUdisksPluginPrivate *priv;
};

GType is_udisks_plugin_get_type(void) G_GNUC_CONST;
G_MODULE_EXPORT void peas_register_types(PeasObjectModule *module);

G_END_DECLS

#endif /* __IS_UDISKS_PLUGIN_H__ */
