#ifndef GIMPXX_PROJECT_GIMP_C_INCLUDES_H
#define GIMPXX_PROJECT_GIMP_C_INCLUDES_H

// The single include boundary of the GIMPController component: the GIMP C
// headers the former main.c of GIMP included. The unit tests replace the
// whole header with the mocks of the GIMP C functions.

// The GIMP headers depend on the order the former main.c included them in.
// clang-format off
#include "config.h"

#include <gio/gio.h>
#include <glib/gstdio.h>

#ifndef GIMP_CONSOLE_COMPILATION
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#else
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include <babl/babl.h>

extern "C" {
#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase-private.h"

#include "pdb/pdb-types.h"

#include "config/gimpearlyrc.h"
#include "config/gimpconfig-dump.h"

#include "core/gimp.h"
#include "core/gimpbacktrace.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"
#include "pdb/internal-procs.h"

#include "about.h"
#include "app.h"
#include "language.h"
#include "sanity.h"
#include "signals.h"
#include "unique.h"

#include "gimp-log.h"
#include "gimp-intl.h"
#include "gimp-version.h"
}
// clang-format on

#endif  // GIMPXX_PROJECT_GIMP_C_INCLUDES_H
