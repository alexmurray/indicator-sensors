#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>

static void activate_action(GtkAction *action);

static GtkActionEntry entries[] = {
  { "Preferences",      "application-preferences", "_Preferences", NULL,
    "Preferences...", G_CALLBACK(activate_action) },
  { "Quit",     "application-exit", "_Quit", "<control>Q",
    "Exit indicator-sensors", G_CALLBACK(gtk_main_quit) },
};
static guint n_entries = G_N_ELEMENTS(entries);

static const gchar *ui_info =
"<ui>"
"  <popup name='Indicator'>"
"    <menuitem action='Preferences' />"
"  </popup>"
"</ui>";

static void activate_action(GtkAction *action)
{
	g_debug("activated action %s", gtk_action_get_name(action));
}

int main(int argc, char **argv)
{
	AppIndicator *indicator;
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GtkWidget *menu;
	GError *error = NULL;

	gtk_init(&argc, &argv);

	indicator = app_indicator_new("indicator-sensors",
				      "indicator-sensors",
				      APP_INDICATOR_CATEGORY_HARDWARE);

	action_group = gtk_action_group_new("AppActions");
	gtk_action_group_add_actions(action_group,
				     entries, n_entries,
				     indicator);

	ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
	if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_info, -1, &error)) {
		g_error("Failed to build menus: %s\n", error->message);
	}

	menu = gtk_ui_manager_get_widget(ui_manager, "/ui/Indicator");

	app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_attention_icon(indicator, "indicator-sensors");
	app_indicator_set_label(indicator, "indicator-sensors", "indicator-sensors");
	app_indicator_set_menu(indicator, GTK_MENU(menu));
	gtk_main ();

	return 0;
}
