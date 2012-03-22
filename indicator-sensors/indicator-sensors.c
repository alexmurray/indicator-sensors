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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "is-log.h"
#include "is-notify.h"
#include "is-application.h"
#include "is-indicator.h"
#include <gtk/gtk.h>
#include <libpeas/peas.h>
#include <glib/gi18n.h>

static void
on_extension_added(PeasExtensionSet *set,
		   PeasPluginInfo *info,
		   PeasExtension *exten,
		   IsIndicator *indicator)
{
	is_debug("main", "Activating plugin: %s", peas_plugin_info_get_name(info));
	peas_extension_call(exten, "activate", indicator);
}

static void
on_extension_removed(PeasExtensionSet *set,
		     PeasPluginInfo *info,
		     PeasExtension *exten,
		     IsIndicator *indicator)
{
	is_debug("main", "Deactivating plugin: %s", peas_plugin_info_get_name(info));
	peas_extension_call(exten, "deactivate", indicator);
}

static void
on_plugin_list_notify(PeasEngine *engine,
		      GParamSpec *pspec,
		      gpointer user_data)
{
	const GList *plugins = peas_engine_get_plugin_list(engine);
	while (plugins != NULL) {
		PeasPluginInfo *info = PEAS_PLUGIN_INFO(plugins->data);
		peas_engine_load_plugin(engine, info);
		plugins = plugins->next;
	}
}

int main(int argc, char **argv)
{
	IsApplication *application;
        GSettings *settings;
        IsManager *manager;
	gchar *plugin_dir;
	PeasEngine *engine;
	PeasExtensionSet *set;
	GError *error = NULL;

	gtk_init(&argc, &argv);

	if (!g_irepository_require(g_irepository_get_default(), "Peas", "1.0",
				   0, &error))
	{
		is_warning("main", "Could not load Peas repository: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	engine = peas_engine_get_default();
	g_signal_connect(engine, "notify::plugin-list",
			 G_CALLBACK(on_plugin_list_notify), NULL);

	/* add home dir to search path */
	plugin_dir = g_build_filename(g_get_user_config_dir(), PACKAGE,
				      "plugins", NULL);
	peas_engine_add_search_path(engine, plugin_dir, NULL);
	g_free(plugin_dir);

	/* add system path to search path */
	plugin_dir = g_build_filename(LIBDIR, PACKAGE, "plugins", NULL);
	peas_engine_add_search_path(engine, plugin_dir, NULL);
	g_free(plugin_dir);

        /* init notifications */
        is_notify_init();
	application = is_application_new();
        settings = g_settings_new("indicator-sensors.application");
        g_settings_bind(settings, "temperature-scale",
                        application, "temperature-scale",
                        G_SETTINGS_BIND_DEFAULT);

	/* create extension set and set manager as object */
	set = peas_extension_set_new(engine, PEAS_TYPE_ACTIVATABLE,
				     "object", application, NULL);

	/* activate all activatable extensions */
	peas_extension_set_call(set, "activate");

	/* and make sure to activate any ones which are found in the future */
	g_signal_connect(set, "extension-added",
			 G_CALLBACK(on_extension_added), application);
	g_signal_connect(set, "extension-removed",
			  G_CALLBACK(on_extension_removed), application);

        /* since all plugins are now inited show a notification if we detected
         * sensors but none are enabled - TODO: perhaps just open the pref's
         * dialog?? */
        manager = is_application_get_manager(application);
        GSList *sensors = is_manager_get_all_sensors_list(manager);
        if (sensors) {
                gchar **enabled_sensors = is_manager_get_enabled_sensors(manager);
                if (!g_strv_length(enabled_sensors)) {
                        is_notify(IS_NOTIFY_LEVEL_INFO,
                                  _("No Sensors Enabled For Monitoring"),
                                  _("Sensors detected but none are enabled for monitoring. To enable monitoring of sensors open the Preferences window and select the sensors to monitor"));
                }
                g_strfreev(enabled_sensors);
                g_slist_foreach(sensors, (GFunc)g_object_unref, NULL);
                g_slist_free(sensors);
        }

	gtk_main();

        g_object_unref(application);
        is_notify_uninit();

	return 0;
}
