#include <gtk/gtk.h>
#include "is-indicator.h"
#include "config.h"

int main(int argc, char **argv)
{
	IsIndicator *indicator;

	gtk_init(&argc, &argv);

	/* create indicator */
	indicator = is_indicator_new(PACKAGE,
				     PACKAGE);

	/* add sensors to indicator */
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
