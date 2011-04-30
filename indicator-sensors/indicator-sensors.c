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
	IsIndicator *indicator;
	IsManager *manager;
	gchar *plugin_dir;
	PeasEngine *engine;
	PeasExtensionSet *set;
	GError *error = NULL;

	gtk_init(&argc, &argv);

	if (!g_irepository_require(g_irepository_get_default(), "Peas", "1.0",
				   0, &error))
	{
		g_warning("Could not load Peas repository: %s", error->message);
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

	manager = is_manager_new();
	/* create indicator which uses this manager */
	indicator = is_indicator_new(manager);

	/* create extension set and set manager as object */
	set = peas_extension_set_new(engine, PEAS_TYPE_ACTIVATABLE,
				     "object", manager, NULL);

	/* activate all activatable extensions */
	peas_extension_set_call(set, "activate");

	/* and make sure to activate any ones which are found in the future */
	g_signal_connect(set, "extension-added",
			 G_CALLBACK(on_extension_added), indicator);
	g_signal_connect(set, "extension-removed",
			  G_CALLBACK(on_extension_removed), indicator);

	/* start */
	gtk_main();

	return 0;
}
