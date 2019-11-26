/*
 * Copyright (C) 2011-2019 Alex Murray <murray.alex@gmail.com>
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

#ifndef __IS_PREFERENCES_DIALOG_H__
#define __IS_PREFERENCES_DIALOG_H__

#include <gtk/gtk.h>
#include "is-application.h"

G_BEGIN_DECLS

/* response id returned when clicking on properties button */
#define IS_PREFERENCES_DIALOG_RESPONSE_SENSOR_PROPERTIES 1

#define IS_TYPE_PREFERENCES_DIALOG    \
  (is_preferences_dialog_get_type())
#define IS_PREFERENCES_DIALOG(obj)        \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),      \
                              IS_TYPE_PREFERENCES_DIALOG, \
                              IsPreferencesDialog))
#define IS_PREFERENCES_DIALOG_CLASS(klass)      \
  (G_TYPE_CHECK_CLASS_CAST((klass),     \
                           IS_TYPE_PREFERENCES_DIALOG,  \
                           IsPreferencesDialogClass))
#define IS_IS_PREFERENCES_DIALOG(obj)                                   \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),        \
                              IS_TYPE_PREFERENCES_DIALOG))
#define IS_IS_PREFERENCES_DIALOG_CLASS(klass)     \
  (G_TYPE_CHECK_CLASS_TYPE((klass),     \
                           IS_TYPE_PREFERENCES_DIALOG))
#define IS_PREFERENCES_DIALOG_GET_CLASS(obj)      \
  (G_TYPE_INSTANCE_GET_CLASS((obj),     \
                             IS_TYPE_PREFERENCES_DIALOG,  \
                             IsPreferencesDialogClass))

typedef struct _IsPreferencesDialog      IsPreferencesDialog;
typedef struct _IsPreferencesDialogClass IsPreferencesDialogClass;
typedef struct _IsPreferencesDialogPrivate IsPreferencesDialogPrivate;

struct _IsPreferencesDialogClass
{
  GtkDialogClass parent_class;
};

struct _IsPreferencesDialog
{
  GtkDialog parent;
  IsPreferencesDialogPrivate *priv;
};

GType is_preferences_dialog_get_type(void) G_GNUC_CONST;
GtkWidget *is_preferences_dialog_new(IsApplication *application);

G_END_DECLS

#endif /* __IS_PREFERENCES_DIALOG_H__ */
