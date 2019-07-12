#ifndef LUPUS_LUPUS_MAIN_H
#define LUPUS_LUPUS_MAIN_H

#include <gtk/gtk.h>
#include "toxcore/tox.h"
#include "lupus_headerbar.h"
#include "lupus_profile.h"

#define LUPUS_TYPE_MAIN lupus_main_get_type()

G_DECLARE_FINAL_TYPE(LupusMain, lupus_main, LUPUS, MAIN, GtkApplicationWindow)

LupusMain *lupus_main_new(GtkApplication *application, Tox *tox);

#endif