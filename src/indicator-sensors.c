#include <gtk/gtk.h>
#include <libpeas/peas.h>
#include "is-indicator.h"
#include "config.h"

int main(int argc, char **argv)
{
	IsIndicator *indicator;
	gchar *plugin_dir;
	PeasEngine *engine;

	gtk_init(&argc, &argv);

	engine = peas_engine_get_default();
	/* add home dir to search path */
	plugin_dir = g_build_filename(g_get_user_config_dir(), PACKAGE "/plugins", NULL);
	peas_engine_add_search_path(engine, plugin_dir, plugin_dir);
	g_free(plugin_dir);

	/* add system path to search path */
	peas_engine_add_search_path(engine,
				    LIBDIR "/" PACKAGE "/plugins/",
				    PREFIX "/share/" PACKAGE "/plugins");

	/* create extension set and set indicator as object */

	/* create indicator */
	indicator = is_indicator_new(PACKAGE,
				     PACKAGE);

	/* load plugins which should then add found sensors to indicator */


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
