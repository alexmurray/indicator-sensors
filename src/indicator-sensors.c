#include <gtk/gtk.h>
#include <libpeas/peas.h>
#include "is-indicator.h"
#include "config.h"

static void
on_extension_added(PeasExtensionSet *set,
		   PeasPluginInfo *info,
		   PeasExtension *exten,
		   IsIndicator *indicator)
{
	g_debug("Activating plugin: %s", peas_plugin_info_get_name(info));
	peas_extension_call(exten, "activate", indicator);
}

static void
on_extension_removed(PeasExtensionSet *set,
		     PeasPluginInfo *info,
		     PeasExtension *exten,
		     IsIndicator *indicator)
{
	g_debug("Deactivating plugin: %s", peas_plugin_info_get_name(info));
	peas_extension_call(exten, "deactivate", indicator);
}

static void on_plugin_loaded(PeasEngine *engine,
			     PeasPluginInfo *info,
			     gpointer user_data)
{
	g_debug("Loaded plugin: %s", peas_plugin_info_get_name(info));
}


static void on_plugin_unloaded(PeasEngine *engine,
			       PeasPluginInfo *info,
			       gpointer user_data)
{
	g_debug("Unloaded plugin: %s", peas_plugin_info_get_name(info));
}

int main(int argc, char **argv)
{
	IsIndicator *indicator;
	gchar *plugin_dir;
	PeasEngine *engine;
	PeasExtensionSet *set;

	gtk_init(&argc, &argv);

	engine = peas_engine_get_default();
	g_signal_connect(engine, "load-plugin", G_CALLBACK(on_plugin_loaded), NULL);
	g_signal_connect(engine, "unload-plugin", G_CALLBACK(on_plugin_unloaded), NULL);

	/* add home dir to search path */
	plugin_dir = g_build_filename(g_get_user_config_dir(), PACKAGE "/plugins", NULL);
	peas_engine_add_search_path(engine, plugin_dir, plugin_dir);
	g_free(plugin_dir);

	/* add system path to search path */
	peas_engine_add_search_path(engine,
				    LIBDIR "/" PACKAGE "/plugins/",
				    NULL);

	/* load plugins which should then add found sensors to indicator */
	peas_engine_rescan_plugins(engine);

	{
		gchar **plugins = peas_engine_get_loaded_plugins(engine);
		gchar **plugin;
		for (plugin = plugins; *plugin != NULL; plugin++) {
			g_debug("loaded plugin: %s", *plugin);
		}
		g_strfreev(plugins);
	}

	/* create indicator */
	indicator = is_indicator_new(PACKAGE,
				     PACKAGE);

	/* create extension set and set indicator as object */
	set = peas_extension_set_new(engine, PEAS_TYPE_ACTIVATABLE,
				     "object", indicator, NULL);
	peas_extension_set_call(set, "activate");
	g_signal_connect(set, "extension-added",
			 G_CALLBACK(on_extension_added), indicator);
	g_signal_connect(set, "extension-removed",
			  G_CALLBACK(on_extension_removed), indicator);

	/* TODO: remove this hack!! add sensors to indicator */
	is_indicator_add_sensor(indicator,
				is_sensor_new("test-family", "test"));
	is_indicator_add_sensor(indicator,
				is_sensor_new("test-family", "tester"));
	is_indicator_add_sensor(indicator,
				is_sensor_new("test-family", "tester"));

	/* start */
	gtk_main();

	return 0;
}
