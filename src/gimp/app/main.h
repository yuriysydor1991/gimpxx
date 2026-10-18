/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/* The start-up steps of the former main() of GIMP. The C++ application
 * layer of gimpxx (src/app) runs them in their original order, the code
 * which is specific to a platform or to the console binary stays here.
 */

typedef struct _GimpMainOptions GimpMainOptions;

/* The command line options the start-up decides on. The GIMP option
 * entries point into it.
 */
struct _GimpMainOptions
{
  gboolean      be_verbose;
  gboolean      no_interface;
  gboolean      new_instance;
  gboolean      console_messages;
  const gchar **batch_commands;
};

GimpMainOptions * gimp_main_options             (void);

void              gimp_main_init_platform       (gint          *argc,
                                                 gchar       ***argv);
gchar           * gimp_main_init_signals        (void);
gchar          ** gimp_main_init_process        (gchar        **argv);

void              gimp_main_attach_console      (void);
void              gimp_main_show_version        (void);

GOptionContext  * gimp_main_option_context_new  (void);
void              gimp_main_report_parse_error  (GError        *error);

gboolean          gimp_main_set_flatpak_prgname (void);
gboolean          gimp_main_run_remote          (gint          *exit_status);
void              gimp_main_check_sanity        (void);
gint              gimp_main_run                 (const gchar   *full_prog_name,
                                                 const gchar   *backtrace_file);

G_END_DECLS
