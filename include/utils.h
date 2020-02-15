#ifndef __LUPUS_UTILS_H__
#define __LUPUS_UTILS_H__

#include "toxcore/tox.h"
#include <gtk/gtk.h>

void lupus_strcpy(char const *, char const **);

void tox_save(Tox *, char const *, char const *, GtkWindow *, bool);

#endif