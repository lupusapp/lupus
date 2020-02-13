#ifndef __LUPUS_LUPUS_MAIN_H__
#define __LUPUS_LUPUS_MAIN_H__

#include "toxcore/tox.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_MAIN lupus_main_get_type()

G_DECLARE_FINAL_TYPE(LupusMain, lupus_main, LUPUS, MAIN, GtkApplicationWindow)

LupusMain *lupus_main_new(GtkApplication *, Tox *, char const *, char const *);

#endif