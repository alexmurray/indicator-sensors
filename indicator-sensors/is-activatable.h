/*
 * Copyright (C) 2025 Alex Murray <murray.alex@gmail.com>
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

#ifndef __IS_ACTIVATABLE_H__
#define __IS_ACTIVATABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define IS_TYPE_ACTIVATABLE (is_activatable_get_type())
G_DECLARE_INTERFACE(IsActivatable, is_activatable, IS, ACTIVATABLE, GObject)

struct _IsActivatableInterface
{
  GTypeInterface g_iface;

  void (*activate)(IsActivatable *activatable);
  void (*deactivate)(IsActivatable *activatable);
};

/**
 * is_activatable_activate:
 * @activatable: A #IsActivatable.
 *
 * Activates the extension.
 */
void is_activatable_activate(IsActivatable *activatable);

/**
 * is_activatable_deactivate:
 * @activatable: A #IsActivatable.
 *
 * Deactivates the extension.
 */
void is_activatable_deactivate(IsActivatable *activatable);

G_END_DECLS

#endif
