/*
 * Copyright (C) 2011-2025 Alex Murray <murray.alex@gmail.com>
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

#include "config.h"

#include "is-application.h"
#include "is-manager.h"
#include "is-preferences-dialog.h"
#include <gio/gio.h>
#include <gtk/gtk.h>

typedef struct
{
  IsManager *manager;
  IsApplication *application;
} Fixture;

static void
fixture_setup(Fixture *fixture, gconstpointer user_data)
{
  fixture->manager = g_object_ref_sink(is_manager_new());
  fixture->application =
      g_object_new(IS_TYPE_APPLICATION, "manager", fixture->manager, NULL);
}

static void
fixture_teardown(Fixture *fixture, gconstpointer user_data)
{
  /* disposing the application destroys any preferences dialog it created */
  g_clear_object(&fixture->application);
  g_clear_object(&fixture->manager);
}

/* find the preferences dialog amongst the toplevels rather than reaching
 * into IsApplication's private state, so this exercises exactly what
 * on_preferences_action() (indicator-sensors/is-application.c) does */
static GtkWidget *
find_preferences_dialog(void)
{
  GList *toplevels, *list;
  GtkWidget *dialog = NULL;

  toplevels = gtk_window_list_toplevels();
  for (list = toplevels; list != NULL; list = list->next)
  {
    if (IS_IS_PREFERENCES_DIALOG(list->data))
    {
      dialog = GTK_WIDGET(list->data);
      break;
    }
  }
  g_list_free(toplevels);
  return dialog;
}

/* the no-sensors-enabled and indicator-not-shown notifications both rely
 * on an app.preferences action existing so that activating them opens
 * Preferences - see indicator-sensors/is-application.c and
 * indicator-sensors/is-indicator.c.
 *
 * This looks the action up via GActionMap and activates it via
 * g_action_activate() directly on the GAction, rather than going through
 * GApplication's own GActionGroup wrapper (g_action_group_has_action() /
 * g_action_group_activate_action()) - those refuse to work until the
 * application is registered (g_application_register()), which for
 * IsApplication also runs the startup vfunc and constructs a real
 * AppIndicator. That library is known to hang on shutdown in a headless
 * test environment, which is unrelated to what this test is trying to
 * verify, so registration is deliberately avoided here. */
static void
test_application_preferences_action(Fixture *fixture, gconstpointer user_data)
{
  GAction *action;

  action =
      g_action_map_lookup_action(G_ACTION_MAP(fixture->application),
                                 "preferences");
  g_assert_nonnull(action);
  g_assert_true(g_action_get_enabled(action));

  g_assert_null(find_preferences_dialog());

  g_action_activate(action, NULL);

  g_assert_nonnull(find_preferences_dialog());
}

int
main(int argc, char *argv[])
{
  gtk_test_init(&argc, &argv, NULL);

  g_test_add("/application/preferences-action", Fixture, NULL, fixture_setup,
            test_application_preferences_action, fixture_teardown);

  return g_test_run();
}
