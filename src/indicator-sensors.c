#include <gtk/gtk.h>
#include "sensors-indicator.h"
#include "config.h"

int main(int argc, char **argv)
{
	AppIndicator *indicator;

	gtk_init(&argc, &argv);

	/* create indicator */
	indicator = sensors_indicator_new(PACKAGE,
					  PACKAGE);

	/* add sensors to indicator */

	/* start */
	gtk_main ();

	return 0;
}
