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

#ifndef __IS_GSETTINGS_PLUGIN_H__
#define __IS_GSETTINGS_PLUGIN_H__

#include <libpeas/peas.h>


G_BEGIN_DECLS

#define IS_TYPE_GSETTINGS_PLUGIN		\
	(is_gsettings_plugin_get_type())
#define IS_GSETTINGS_PLUGIN(obj)				\
	(G_TYPE_CHECK_INSTANCE_CAST((obj),			\
				    IS_TYPE_GSETTINGS_PLUGIN,	\
				    IsGSettingsPlugin))
#define IS_GSETTINGS_PLUGIN_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST((klass),			\
				 IS_TYPE_GSETTINGS_PLUGIN,	\
				 IsGSettingsPluginClass))
#define IS_IS_GSETTINGS_PLUGIN(obj)				\
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),			\
				    IS_TYPE_GSETTINGS_PLUGIN))
#define IS_IS_GSETTINGS_PLUGIN_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE((klass),			\
				 IS_TYPE_GSETTINGS_PLUGIN))
#define IS_GSETTINGS_PLUGIN_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS((obj),			\
				   IS_TYPE_GSETTINGS_PLUGIN,	\
				   IsGSettingsPluginClass))

typedef struct _IsGSettingsPlugin      IsGSettingsPlugin;
typedef struct _IsGSettingsPluginClass IsGSettingsPluginClass;
typedef struct _IsGSettingsPluginPrivate IsGSettingsPluginPrivate;

struct _IsGSettingsPluginClass
{
	PeasExtensionBaseClass parent_class;
};

struct _IsGSettingsPlugin
{
	PeasExtensionBase parent;
	IsGSettingsPluginPrivate *priv;
};

GType is_gsettings_plugin_get_type(void) G_GNUC_CONST;
G_MODULE_EXPORT void peas_register_types(PeasObjectModule *module);

G_END_DECLS

#endif /* __IS_GSETTINGS_PLUGIN_H__ */
