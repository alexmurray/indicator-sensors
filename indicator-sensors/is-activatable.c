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

#include "is-activatable.h"
#include "is-application.h"

G_DEFINE_INTERFACE(IsActivatable, is_activatable, G_TYPE_OBJECT)

void
is_activatable_default_init(IsActivatableInterface *iface)
{
  GParamSpec *pspec;

  pspec = g_param_spec_object("application", "Application",
                              "The application instance", IS_TYPE_APPLICATION,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_interface_install_property(iface, pspec);
  g_param_spec_unref(pspec);
}

void
is_activatable_activate(IsActivatable *activatable)
{
  IsActivatableInterface *iface;

  g_return_if_fail(IS_IS_ACTIVATABLE(activatable));

  iface = IS_ACTIVATABLE_GET_IFACE(activatable);
  if (iface->activate)
    iface->activate(activatable);
}

void
is_activatable_deactivate(IsActivatable *activatable)
{
  IsActivatableInterface *iface;

  g_return_if_fail(IS_IS_ACTIVATABLE(activatable));

  iface = IS_ACTIVATABLE_GET_IFACE(activatable);
  if (iface->deactivate)
    iface->deactivate(activatable);
}
