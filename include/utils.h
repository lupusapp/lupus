#ifndef __LUPUS_UTILS_H__
#define __LUPUS_UTILS_H__

#include "toxcore/tox.h"
#include <gtk/gtk.h>

void tox_save(Tox *, gchar const *, gchar const *, GtkWindow *, gboolean);

#endif